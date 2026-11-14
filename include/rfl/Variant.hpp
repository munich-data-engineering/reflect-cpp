#ifndef RFL_VARIANT_HPP_
#define RFL_VARIANT_HPP_

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "internal/element_index.hpp"
#include "internal/nth_element_t.hpp"
#include "internal/ptr_cast.hpp"
#include "internal/variant/find_max_size.hpp"
#include "internal/variant/is_alternative_type.hpp"
#include "internal/variant/result_t.hpp"

namespace rfl {
template <class... AlternativeTypes>
class Variant {
  static constexpr auto max_size_wrapper_ =
      internal::variant::find_max_size<AlternativeTypes...>();

  static constexpr unsigned long num_bytes_ = max_size_wrapper_.size_;

  using DataType = std::array<unsigned char, num_bytes_>;

  using IndexType =
      std::conditional_t<sizeof...(AlternativeTypes) <=
                             std::numeric_limits<std::uint8_t>::max(),
                         std::uint8_t, std::uint16_t>;

  static constexpr IndexType size_ = sizeof...(AlternativeTypes);

  template <class F>
  using result_t = internal::variant::result_t<F, AlternativeTypes...>;

  template <IndexType _i>
  using Index = std::integral_constant<IndexType, _i>;

  template <class T>
  struct TypeWrapper {};

 public:
  /// @brief Default-constructs the Variant, initializing it with the first
  /// alternative type.
  /// @details The first alternative type is default-constructed in place.
  Variant() : index_(IndexType()), data_(DataType()) {
    using FirstAlternative = internal::nth_element_t<0, AlternativeTypes...>;
    new (data_.data()) FirstAlternative();
  }

  /// @brief Copy-constructs the Variant from another Variant.
  /// @param _other The Variant to copy from.
  /// @details The contained value is copied from the other Variant, preserving
  /// the alternative type.
  Variant(const Variant& _other) : index_(IndexType()), data_(DataType()) {
    copy_from_other(_other);
  }

  /// @brief Move-constructs the Variant from another Variant.
  /// @param _other The Variant to move from.
  /// @details The contained value is moved from the other Variant, preserving
  /// the alternative type.
  Variant(Variant&& _other) noexcept : index_(IndexType()), data_(DataType()) {
    move_from_other(std::move(_other));
  }

  /// @brief Constructs the Variant from a value of an alternative type (copy).
  /// @tparam T The type of the value to store.
  /// @param _t The value to copy into the Variant.
  /// @details Only enabled if T is one of the alternative types.
  template <class T>
    requires internal::variant::is_alternative_type_v<T, AlternativeTypes...>
  Variant(const T& _t) : index_(IndexType()), data_(DataType()) {
    copy_from_type(_t);
  }

  /// @brief Constructs the Variant from a value of an alternative type (move).
  /// @tparam T The type of the value to store.
  /// @param _t The value to move into the Variant.
  /// @details Only enabled if T is one of the alternative types.
  template <class T>
    requires internal::variant::is_alternative_type_v<T, AlternativeTypes...>
  Variant(T&& _t) noexcept : index_(IndexType()), data_(DataType()) {
    move_from_type(std::forward<T>(_t));
  }

  /// @brief Destroys the contained value if necessary.
  /// @details Calls the destructor of the contained alternative type if it is
  /// destructible.
  ~Variant() { destroy_if_necessary(); }

  /// @brief Emplaces a new element of type T into the variant.
  /// @tparam T The type to emplace.
  /// @tparam Args The argument types for T's constructor.
  /// @param _args The arguments to forward to T's constructor.
  /// @return Reference to the newly emplaced value.
  template <class T, class... Args>
  constexpr T& emplace(Args&&... _args) {
    auto t = T{std::forward<Args>(_args)...};
    destroy_if_necessary();
    move_from_type(std::move(t));
    return *internal::ptr_cast<T*>(data_.data());
  }

  /// @brief Emplaces a new element of the N-th alternative type into the
  /// variant.
  /// @tparam _i The index of the alternative type.
  /// @tparam Args The argument types for the constructor.
  /// @param _args The arguments to forward to the constructor.
  /// @return Reference to the newly emplaced value.
  template <int _i, class... Args>
  constexpr auto& emplace(Args&&... _args) {
    using T = internal::nth_element_t<_i, AlternativeTypes...>;
    return emplace<T>(std::move(_args)...);
  }

  /// @brief Returns the index of the currently held alternative.
  /// @return The index of the contained alternative type.
  constexpr int index() const noexcept { return index_; }

  /// @brief Assigns a value of an alternative type to the Variant (copy).
  /// @tparam T The type of the value to assign.
  /// @param _t The value to assign.
  /// @return Reference to this Variant.
  template <class T>
    requires internal::variant::is_alternative_type_v<T, AlternativeTypes...>
  Variant& operator=(const T& _t) {
    auto temp = Variant(_t);
    destroy_if_necessary();
    move_from_other(std::move(temp));
    return *this;
  }

  /// @brief Assigns a value of an alternative type to the Variant (move).
  /// @tparam T The type of the value to assign.
  /// @param _t The value to assign.
  /// @return Reference to this Variant.
  template <class T>
    requires internal::variant::is_alternative_type_v<T, AlternativeTypes...>
  Variant& operator=(T&& _t) noexcept {
    destroy_if_necessary();
    move_from_type(std::forward<T>(_t));
    return *this;
  }

  /// @brief Copy-assigns from another Variant.
  /// @param _other The Variant to copy from.
  /// @return Reference to this Variant.
  Variant& operator=(const Variant& _other) {
    if (this == &_other) {
      return *this;
    }
    auto temp = Variant(_other);
    destroy_if_necessary();
    move_from_other(std::move(temp));
    return *this;
  }

  /// @brief Move-assigns from another Variant.
  /// @param _other The Variant to move from.
  /// @return Reference to this Variant.
  Variant& operator=(Variant&& _other) noexcept {
    if (this == &_other) {
      return *this;
    }
    destroy_if_necessary();
    move_from_other(std::move(_other));
    return *this;
  }

  /// @brief Swaps the contents of this Variant with another.
  /// @param _other The Variant to swap with.
  /// @details Exchanges the contained values and types between the two
  /// Variants.
  void swap(Variant& _other) noexcept {
    if (this == &_other) {
      return;
    }
    auto temp = Variant(std::move(*this));
    move_from_other(std::move(_other));
    _other = std::move(temp);
  }

  /// @brief Visits the contained value with a visitor function.
  /// @tparam F The visitor type.
  /// @param _f The visitor function or functor.
  /// @return The result of the visitor, or void.
  template <class F>
  result_t<F> visit(F&& _f) {
    using ResultType = result_t<F>;
    if constexpr (std::is_same_v<ResultType, void>) {
      bool visited = false;
      do_visit_no_result(std::forward<F>(_f), &visited,
                         std::make_integer_sequence<IndexType, size_>());
    } else if constexpr (std::is_reference_v<ResultType>) {
      std::remove_reference_t<ResultType>* res = nullptr;
      do_visit_with_reference(std::forward<F>(_f), &res,
                              std::make_integer_sequence<IndexType, size_>());
      return *res;
    } else {
      auto res = std::optional<ResultType>();
      do_visit_with_result(std::forward<F>(_f), &res,
                           std::make_integer_sequence<IndexType, size_>());
      return std::move(*res);
    }
  }

  /// @brief Visits the contained value with a visitor function (const
  /// overload).
  /// @tparam F The visitor type.
  /// @param _f The visitor function or functor.
  /// @return The result of the visitor, or void.
  template <class F>
  result_t<F> visit(F&& _f) const {
    using ResultType = result_t<F>;
    if constexpr (std::is_same_v<ResultType, void>) {
      bool visited = false;
      do_visit_no_result(std::forward<F>(_f), &visited,
                         std::make_integer_sequence<IndexType, size_>());
    } else if constexpr (std::is_reference_v<ResultType>) {
      std::remove_reference_t<ResultType>* res = nullptr;
      do_visit_with_reference(std::forward<F>(_f), &res,
                              std::make_integer_sequence<IndexType, size_>());
      return *res;
    } else {
      auto res = std::optional<ResultType>();
      do_visit_with_result(std::forward<F>(_f), &res,
                           std::make_integer_sequence<IndexType, size_>());
      return std::move(*res);
    }
  }

 private:
  /// @brief Copies the value from another Variant.
  /// @param _other The Variant to copy from.
  void copy_from_other(const Variant& _other) {
    const auto copy_one = [this](const auto& _t) { this->copy_from_type(_t); };
    _other.visit(copy_one);
  }

  /// @brief Copies a value of type T from another type.
  /// @tparam T The type to copy from.
  /// @param _t The value to copy.
  /// @details Tries to copy the value from T to each alternative type, and if
  /// successful, stores it in the Variant. Only the first successful copy is
  /// performed.
  template <class T>
  void copy_from_other_type(const T& _t) {
    bool set = false;
    const auto copy_one = [&, this]<class AltType>(const T& _t,
                                                   const TypeWrapper<AltType>) {
      if constexpr (std::is_convertible_v<T, AltType>) {
        if (!set) {
          move_from_type(AltType(_t));
          set = true;
        }
      }
    };
    (copy_one(_t, TypeWrapper<AlternativeTypes>{}), ...);
  }

  /// @brief Copies a value of type T into the Variant.
  /// @tparam T The type to copy from.
  /// @param _t The value to copy.
  template <class T>
  void copy_from_type(const T& _t) noexcept {
    using CurrentType = std::remove_cvref_t<decltype(_t)>;
    index_ =
        internal::element_index<CurrentType,
                                std::remove_cvref_t<AlternativeTypes>...>();
    new (data_.data()) CurrentType(_t);
  }

  /// @brief Destroys the contained value if it is destructible.
  void destroy_if_necessary() {
    const auto destroy_one = [](auto& _t) {
      using T = std::remove_cvref_t<decltype(_t)>;
      if constexpr (std::is_destructible_v<T>) {
        _t.~T();
      }
    };
    visit(destroy_one);
  }

  /// @brief Helper for visiting without result (non-const).
  /// @tparam F The visitor type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _visited Pointer to visitation flag.
  template <class F, IndexType... _is>
  void do_visit_no_result(F& _f, bool* _visited,
                          std::integer_sequence<IndexType, _is...>) {
    auto visit_one = [this]<IndexType _i>(F& _f, bool* _visited, Index<_i>) {
      if (!*_visited && index_ == _i) {
        _f(get_alternative<_i>());
        *_visited = true;
      }
    };
    (visit_one(_f, _visited, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting without result (const).
  /// @tparam F The visitor type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _visited Pointer to visitation flag.
  template <class F, IndexType... _is>
  void do_visit_no_result(F& _f, bool* _visited,
                          std::integer_sequence<IndexType, _is...>) const {
    auto visit_one = [this]<IndexType _i>(F& _f, bool* _visited, Index<_i>) {
      if (!*_visited && index_ == _i) {
        _f(get_alternative<_i>());
        *_visited = true;
      }
    };
    (visit_one(_f, _visited, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting without result (non-const, const visitor).
  /// @tparam F The visitor type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _visited Pointer to visitation flag.
  template <class F, IndexType... _is>
  void do_visit_no_result(const F& _f, bool* _visited,
                          std::integer_sequence<IndexType, _is...>) {
    const auto visit_one = [this]<IndexType _i>(const F& _f, bool* _visited,
                                                Index<_i>) {
      if (!*_visited && index_ == _i) {
        _f(get_alternative<_i>());
        *_visited = true;
      }
    };
    (visit_one(_f, _visited, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting without result (const, const visitor).
  /// @tparam F The visitor type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _visited Pointer to visitation flag.
  template <class F, IndexType... _is>
  void do_visit_no_result(const F& _f, bool* _visited,
                          std::integer_sequence<IndexType, _is...>) const {
    const auto visit_one = [this]<IndexType _i>(const F& _f, bool* _visited,
                                                Index<_i>) {
      if (!*_visited && index_ == _i) {
        _f(get_alternative<_i>());
        *_visited = true;
      }
    };
    (visit_one(_f, _visited, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with result (non-const).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to optional result.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_result(F& _f, std::optional<ResultType>* _result,
                            std::integer_sequence<IndexType, _is...>) {
    auto visit_one = [this]<IndexType _i>(
                         F& _f, std::optional<ResultType>* _result, Index<_i>) {
      if (!*_result && index_ == _i) {
        _result->emplace(_f(get_alternative<_i>()));
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with result (const).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to optional result.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_result(F& _f, std::optional<ResultType>* _result,
                            std::integer_sequence<IndexType, _is...>) const {
    auto visit_one = [this]<IndexType _i>(
                         F& _f, std::optional<ResultType>* _result, Index<_i>) {
      if (!*_result && index_ == _i) {
        _result->emplace(_f(get_alternative<_i>()));
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with result (non-const, const visitor).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to optional result.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_result(const F& _f, std::optional<ResultType>* _result,
                            std::integer_sequence<IndexType, _is...>) {
    const auto visit_one = [this]<IndexType _i>(
                               const F& _f, std::optional<ResultType>* _result,
                               Index<_i>) {
      if (!*_result && index_ == _i) {
        _result->emplace(_f(get_alternative<_i>()));
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with result (const, const visitor).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to optional result.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_result(const F& _f, std::optional<ResultType>* _result,
                            std::integer_sequence<IndexType, _is...>) const {
    const auto visit_one = [this]<IndexType _i>(
                               const F& _f, std::optional<ResultType>* _result,
                               Index<_i>) {
      if (!*_result && index_ == _i) {
        _result->emplace(_f(get_alternative<_i>()));
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with reference result (non-const).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to result pointer.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_reference(F& _f, ResultType** _result,
                               std::integer_sequence<IndexType, _is...>) {
    const auto visit_one = [this]<IndexType _i>(F& _f, ResultType** _result,
                                                Index<_i>) {
      if (!*_result && index_ == _i) {
        *_result = &_f(get_alternative<_i>());
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with reference result (const).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to result pointer.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_reference(F& _f, ResultType** _result,
                               std::integer_sequence<IndexType, _is...>) const {
    const auto visit_one = [this]<IndexType _i>(F& _f, ResultType** _result,
                                                Index<_i>) {
      if (!*_result && index_ == _i) {
        *_result = &_f(get_alternative<_i>());
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with reference result (non-const, const
  /// visitor).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to result pointer.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_reference(const F& _f, ResultType** _result,
                               std::integer_sequence<IndexType, _is...>) {
    const auto visit_one = [this]<IndexType _i>(
                               const F& _f, ResultType** _result, Index<_i>) {
      if (!*_result && index_ == _i) {
        *_result = &_f(get_alternative<_i>());
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Helper for visiting with reference result (const, const visitor).
  /// @tparam F The visitor type.
  /// @tparam ResultType The result type.
  /// @tparam _is Index sequence.
  /// @param _f The visitor.
  /// @param _result Pointer to result pointer.
  template <class F, class ResultType, IndexType... _is>
  void do_visit_with_reference(const F& _f, ResultType** _result,
                               std::integer_sequence<IndexType, _is...>) const {
    const auto visit_one = [this]<IndexType _i>(
                               const F& _f, ResultType** _result, Index<_i>) {
      if (!*_result && index_ == _i) {
        *_result = &_f(get_alternative<_i>());
      }
    };
    (visit_one(_f, _result, Index<_is>{}), ...);
  }

  /// @brief Returns a reference to the contained value of the _i-th alternative
  /// type.
  /// @tparam _i The index of the alternative type.
  /// @return Reference to the contained value.
  template <IndexType _i>
  auto& get_alternative() noexcept {
    using CurrentType = internal::nth_element_t<_i, AlternativeTypes...>;
    return *internal::ptr_cast<CurrentType*>(data_.data());
  }

  /// @brief Returns a const reference to the contained value of the _i-th
  /// alternative type.
  /// @tparam _i The index of the alternative type.
  /// @return Const reference to the contained value.
  template <IndexType _i>
  const auto& get_alternative() const noexcept {
    using CurrentType = internal::nth_element_t<_i, AlternativeTypes...>;
    return *internal::ptr_cast<const CurrentType*>(data_.data());
  }

  /// @brief Moves the value from another Variant into this Variant.
  /// @param _other The Variant to move from.
  void move_from_other(Variant&& _other) noexcept {
    const auto move_one = [this](auto&& _t) {
      this->move_from_type(std::forward<std::remove_cvref_t<decltype(_t)>>(_t));
    };
    std::move(_other).visit(move_one);
  }

  /// @brief Moves a value of type T into the Variant.
  /// @tparam T The type to move from.
  /// @param _t The value to move.
  template <class T>
  void move_from_type(T&& _t) noexcept {
    using CurrentType = std::remove_cvref_t<decltype(_t)>;
    index_ =
        internal::element_index<CurrentType,
                                std::remove_cvref_t<AlternativeTypes>...>();
    new (data_.data()) CurrentType(std::forward<T>(_t));
  }

 private:
  /// Index indicating which of the alternatives is currently contained in the
  /// variant.
  IndexType index_;

  /// The underlying data, can be any of the underlying types.
  alignas(AlternativeTypes...) DataType data_;
};

/// @brief Concept to check if a type is Variant-based.
/// @tparam V The type to check.
template <typename V>
concept VariantBased = requires(std::decay_t<V> v) {
  []<typename... Args>(Variant<Args...> const&) {}(v);
};

/// @brief Returns a pointer to the contained value if it is of type T,
/// otherwise nullptr.
/// @tparam T The type to check for.
/// @tparam Types The alternative types.
/// @param _v Pointer to the Variant.
/// @return Pointer to the contained value or nullptr.
template <class T, class... Types>
constexpr T* get_if(Variant<Types...>* _v) noexcept {
  const auto get = [](auto& _v) -> T* {
    using Type = std::remove_cvref_t<decltype(_v)>;
    if constexpr (std::is_same<Type, std::remove_cvref_t<T>>()) {
      return &_v;
    } else {
      return nullptr;
    }
  };
  return _v->visit(get);
}

/// @brief Returns a const pointer to the contained value if it is of type T,
/// otherwise nullptr.
/// @tparam T The type to check for.
/// @tparam Types The alternative types.
/// @param _v Pointer to the Variant.
/// @return Const pointer to the contained value or nullptr.
template <class T, class... Types>
constexpr const T* get_if(const Variant<Types...>* _v) noexcept {
  const auto get = [](const auto& _v) -> const T* {
    using Type = std::remove_cvref_t<decltype(_v)>;
    if constexpr (std::is_same<Type, std::remove_cvref_t<T>>()) {
      return &_v;
    } else {
      return nullptr;
    }
  };
  return _v->visit(get);
}

/// @brief Returns a pointer to the contained value if it is of the N-th
/// alternative type, otherwise nullptr.
/// @tparam _i The index of the alternative type.
/// @tparam Types The alternative types.
/// @param _v Pointer to the Variant.
/// @return Pointer to the contained value or nullptr.
template <int _i, class... Types>
constexpr auto* get_if(Variant<Types...>* _v) noexcept {
  using T = internal::nth_element_t<_i, Types...>;
  return get_if<T>(_v);
}

/// @brief Returns a const pointer to the contained value if it is of the N-th
/// alternative type, otherwise nullptr.
/// @tparam _i The index of the alternative type.
/// @tparam Types The alternative types.
/// @param _v Pointer to the Variant.
/// @return Const pointer to the contained value or nullptr.
template <int _i, class... Types>
constexpr auto* get_if(const Variant<Types...>* _v) noexcept {
  using T = internal::nth_element_t<_i, Types...>;
  return get_if<T>(_v);
}

/// @brief Returns a reference to the contained value if it is of type T, throws
/// otherwise.
/// @tparam T The type to get.
/// @tparam Types The alternative types.
/// @param _v Reference to the Variant.
/// @return Reference to the contained value.
/// @throws std::runtime_error if the contained value is not of type T.
template <class T, class... Types>
constexpr T& get(Variant<Types...>& _v) {
  auto ptr = get_if<T>(&_v);
  if (!ptr) {
    throw std::runtime_error("Variant does not contain signified type.");
  }
  return *ptr;
}

/// @brief Returns an rvalue reference to the contained value if it is of type
/// T, throws otherwise.
/// @tparam T The type to get.
/// @tparam Types The alternative types.
/// @param _v Rvalue reference to the Variant.
/// @return Rvalue reference to the contained value.
/// @throws std::runtime_error if the contained value is not of type T.
template <class T, class... Types>
constexpr T&& get(Variant<Types...>&& _v) {
  auto ptr = get_if<T>(&_v);
  if (!ptr) {
    throw std::runtime_error("Variant does not contain signified type.");
  }
  return std::move(*ptr);
}

/// @brief Returns a const reference to the contained value if it is of type T,
/// throws otherwise.
/// @tparam T The type to get.
/// @tparam Types The alternative types.
/// @param _v Const reference to the Variant.
/// @return Const reference to the contained value.
/// @throws std::runtime_error if the contained value is not of type T.
template <class T, class... Types>
constexpr const T& get(const Variant<Types...>& _v) {
  auto ptr = get_if<T>(&_v);
  if (!ptr) {
    throw std::runtime_error("Variant does not contain signified type.");
  }
  return *ptr;
}

/// @brief Returns a reference to the contained value if it is of the N-th
/// alternative type, throws otherwise.
/// @tparam _i The index of the alternative type.
/// @tparam Types The alternative types.
/// @param _v Reference to the Variant.
/// @return Reference to the contained value.
/// @throws std::runtime_error if the contained value is not of the N-th type.
template <int _i, class... Types>
constexpr auto& get(Variant<Types...>& _v) {
  auto ptr = get_if<_i>(&_v);
  if (!ptr) {
    throw std::runtime_error("Variant does not contain signified type.");
  }
  return *ptr;
}

/// @brief Returns an rvalue reference to the contained value if it is of the
/// N-th alternative type, throws otherwise.
/// @tparam _i The index of the alternative type.
/// @tparam Types The alternative types.
/// @param _v Rvalue reference to the Variant.
/// @return Rvalue reference to the contained value.
/// @throws std::runtime_error if the contained value is not of the N-th type.
template <int _i, class... Types>
constexpr auto&& get(Variant<Types...>&& _v) {
  auto ptr = get_if<_i>(&_v);
  if (!ptr) {
    throw std::runtime_error("Variant does not contain signified type.");
  }
  return std::move(*ptr);
}

/// @brief Returns a const reference to the contained value if it is of the N-th
/// alternative type, throws otherwise.
/// @tparam _i The index of the alternative type.
/// @tparam Types The alternative types.
/// @param _v Const reference to the Variant.
/// @return Const reference to the contained value.
/// @throws std::runtime_error if the contained value is not of the N-th type.
template <int _i, class... Types>
constexpr const auto& get(const Variant<Types...>& _v) {
  auto ptr = get_if<_i>(&_v);
  if (!ptr) {
    throw std::runtime_error("Variant does not contain signified type.");
  }
  return *ptr;
}

/// @brief Checks if the Variant currently holds a value of type T.
/// @tparam T The type to check for.
/// @tparam Types The alternative types.
/// @param _v Const reference to the Variant.
/// @return True if the Variant holds T, false otherwise.
template <class T, class... Types>
constexpr bool holds_alternative(const Variant<Types...>& _v) noexcept {
  constexpr auto ix = internal::element_index<std::remove_cvref_t<T>,
                                              std::remove_cvref_t<Types>...>();
  static_assert(ix != -1, "Type not supported.");
  return ix == _v.index();
}

/// @brief Provides the type of the N-th alternative in a Variant.
/// @tparam N The index of the alternative.
/// @tparam T The Variant type.
template <int N, class T>
struct variant_alternative;

template <int N, class... Types>
struct variant_alternative<N, Variant<Types...>> {
  using type = internal::nth_element_t<N, Types...>;
};

/// @brief Alias for the type of the N-th alternative in a Variant.
/// @tparam N The index of the alternative.
/// @tparam VariantType The Variant type.
template <int N, class VariantType>
using variant_alternative_t =
    typename variant_alternative<N, std::remove_cvref_t<VariantType>>::type;

/// @brief Provides the number of alternatives in a Variant.
/// @tparam T The Variant type.
template <class T>
struct variant_size;

template <class... Types>
struct variant_size<Variant<Types...>>
    : std::integral_constant<size_t, sizeof...(Types)> {};

/// @brief Alias for the number of alternatives in a Variant.
/// @tparam VariantType The Variant type.
template <class VariantType>
constexpr size_t variant_size_v =
    variant_size<std::remove_cvref_t<VariantType>>();

}  // namespace rfl

namespace std {
/// @brief Swaps two Variants.
/// @tparam Types The alternative types.
/// @param _lhs The first Variant.
/// @param _rhs The second Variant.
template <class... Types>
void swap(rfl::Variant<Types...>& _lhs, rfl::Variant<Types...>& _rhs) noexcept {
  _lhs.swap(_rhs);
};
}  // namespace std

#endif
