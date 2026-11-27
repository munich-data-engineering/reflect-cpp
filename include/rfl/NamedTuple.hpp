#ifndef RFL_NAMEDTUPLE_HPP_
#define RFL_NAMEDTUPLE_HPP_

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Field.hpp"
#include "Literal.hpp"
#include "Tuple.hpp"
#include "apply.hpp"
#include "get.hpp"
#include "internal/StringLiteral.hpp"
#include "internal/find_index.hpp"
#include "internal/is_extra_fields.hpp"
#include "make_field.hpp"
#include "make_from_tuple.hpp"
#include "tuple_cat.hpp"

namespace rfl {

/// A named tuple behaves like a tuple,
/// but the fields have explicit names, which
/// allows for reflection.
/// It is one of the core types in the library, and is used as the underlying
/// type for structs. When we reflect a struct, we get a named tuple where each
/// field corresponds to a field in the struct, and the field names are the same
/// as the struct's field names. IMPORTANT: We have two template
/// specializations. One with fields, one without fields.
template <class... FieldTypes>
class NamedTuple;

// ----------------------------------------------------------------------------

template <class... FieldTypes>
class NamedTuple {
  template <int _i>
  struct Index {};

  static constexpr auto seq_ =
      std::make_integer_sequence<int, sizeof...(FieldTypes)>();

 public:
  using Fields = rfl::Tuple<std::remove_cvref_t<FieldTypes>...>;
  using Names = Literal<std::remove_cvref_t<FieldTypes>::name_...>;
  using Values = rfl::Tuple<typename std::remove_cvref_t<FieldTypes>::Type...>;

 public:
  /// @brief Construct NamedTuple from values of each field type.
  /// @param _values Rvalue references to the values for each field.
  NamedTuple(typename std::remove_cvref<FieldTypes>::type::Type&&... _values)
      : values_(
            std::forward<typename std::remove_cvref<FieldTypes>::type::Type>(
                _values)...) {}

  /// @brief Construct NamedTuple from values of each field type.
  /// @param _values Const lvalue references to the values for each field.
  NamedTuple(
      const typename std::remove_cvref<FieldTypes>::type::Type&... _values)
      : values_(rfl::make_tuple(_values...)) {}

  /// @brief Construct NamedTuple from fields (rvalue).
  /// @param _fields Rvalue references to fields.
  NamedTuple(FieldTypes&&... _fields)
      : values_(rfl::make_tuple(std::move(_fields.value_)...)) {}

  /// @brief Construct NamedTuple from fields (const lvalue).
  /// @param _fields Const lvalue references to fields.
  NamedTuple(const FieldTypes&... _fields)
      : values_(rfl::make_tuple(_fields.value_...)) {}

  /// @brief Construct NamedTuple from a tuple containing fields (rvalue).
  /// @param _tup Tuple of fields.
  NamedTuple(rfl::Tuple<FieldTypes...>&& _tup)
      : NamedTuple(rfl::make_from_tuple<NamedTuple<FieldTypes...>>(
            std::forward<rfl::Tuple<FieldTypes...>>(_tup))) {}

  /// @brief Construct NamedTuple from a tuple containing fields (const lvalue).
  /// @param _tup Tuple of fields.
  NamedTuple(const rfl::Tuple<FieldTypes...>& _tup)
      : NamedTuple(rfl::make_from_tuple<NamedTuple<FieldTypes...>>(_tup)) {}

  /// @brief Copy constructor.
  /// @param _other Another NamedTuple to copy from.
  NamedTuple(const NamedTuple<FieldTypes...>& _other) = default;

  /// @brief Move constructor.
  /// @param _other Another NamedTuple to move from.
  NamedTuple(NamedTuple<FieldTypes...>&& _other) = default;

  /// @brief Copy constructor from another NamedTuple with possibly different
  /// field types.
  /// @param _other Another NamedTuple to copy from.
  template <class... OtherFieldTypes>
  NamedTuple(const NamedTuple<OtherFieldTypes...>& _other)
      : NamedTuple(retrieve_fields(_other.fields(), seq_)) {}

  /// @brief Move constructor from another NamedTuple with possibly different
  /// field types.
  /// @param _other Another NamedTuple to move from.
  template <class... OtherFieldTypes>
  NamedTuple(NamedTuple<OtherFieldTypes...>&& _other)
      : NamedTuple(retrieve_fields(
            std::forward<NamedTuple<OtherFieldTypes...>>(_other).fields(),
            seq_)) {}

  /// @brief Destructor.
  ~NamedTuple() = default;

  /// @brief Returns a new NamedTuple with additional fields (rvalue overload).
  /// @tparam _name Name of the field to add.
  /// @tparam FType Type of the field to add.
  /// @tparam Tail Additional fields to add.
  /// @param _head Field to add.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <internal::StringLiteral _name, class FType, class... Tail>
  auto add(Field<_name, FType>&& _head, Tail&&... _tail) && {
    using Head = Field<_name, FType>;
    if constexpr (sizeof...(Tail) > 0) {
      return NamedTuple<FieldTypes..., std::remove_cvref_t<Head>>(
                 std::move(*this).make_fields(seq_, std::forward<Head>(_head)))
          .add(std::forward<Tail>(_tail)...);
    } else {
      return NamedTuple<FieldTypes..., std::remove_cvref_t<Head>>(
          std::move(*this).make_fields(seq_, std::forward<Head>(_head)));
    }
  }

  /// @brief Returns a new NamedTuple with additional fields (const lvalue
  /// overload).
  /// @tparam _name Name of the field to add.
  /// @tparam FType Type of the field to add.
  /// @tparam Tail Additional fields to add.
  /// @param _head Field to add.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <internal::StringLiteral _name, class FType, class... Tail>
  auto add(Field<_name, FType> _head, const Tail&... _tail) const& {
    using Head = Field<_name, FType>;
    if constexpr (sizeof...(Tail) > 0) {
      return NamedTuple<FieldTypes..., std::remove_cvref_t<Head>>(
                 make_fields(seq_, _head))
          .add(_tail...);
    } else {
      return NamedTuple<FieldTypes..., std::remove_cvref_t<Head>>(
          make_fields(seq_, _head));
    }
  }

  /// @brief Add fields from a tuple (rvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @tparam Tail Additional fields to add.
  /// @param _tuple Tuple of fields to add.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent, class... Tail>
  auto add(rfl::Tuple<TupContent...>&& _tuple, Tail&&... _tail) && {
    if constexpr (sizeof...(Tail) > 0) {
      return std::move(*this)
          .add_tuple(std::forward<rfl::Tuple<TupContent...>>(_tuple))
          .add(std::forward<Tail>(_tail)...);
    } else {
      return std::move(*this).add_tuple(
          std::forward<rfl::Tuple<TupContent...>>(_tuple));
    }
  }

  /// @brief Add fields from a tuple (const lvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @tparam Tail Additional fields to add.
  /// @param _tuple Tuple of fields to add.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent, class... Tail>
  auto add(rfl::Tuple<TupContent...> _tuple, const Tail&... _tail) const& {
    if constexpr (sizeof...(Tail) > 0) {
      return add_tuple(std::move(_tuple)).add(_tail...);
    } else {
      return add_tuple(std::move(_tuple));
    }
  }

  /// @brief Add fields from another NamedTuple (rvalue overload).
  /// @tparam TupContent Types of the NamedTuple fields.
  /// @tparam Tail Additional fields to add.
  /// @param _named_tuple NamedTuple to add fields from.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent, class... Tail>
  auto add(NamedTuple<TupContent...>&& _named_tuple, Tail&&... _tail) && {
    return std::move(*this).add(
        std::forward<rfl::Tuple<TupContent...>>(
            std::forward<NamedTuple<TupContent...>>(_named_tuple).fields()),
        std::forward<Tail>(_tail)...);
  }

  /// @brief Add fields from another NamedTuple (const lvalue overload).
  /// @tparam TupContent Types of the NamedTuple fields.
  /// @tparam Tail Additional fields to add.
  /// @param _named_tuple NamedTuple to add fields from.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent, class... Tail>
  auto add(NamedTuple<TupContent...> _named_tuple,
           const Tail&... _tail) const& {
    return add(_named_tuple.fields(), _tail...);
  }

  /// @brief Applies a function to each field, returning a new NamedTuple with
  /// the results (rvalue overload).
  /// @tparam F Callable type.
  /// @param _f Function to apply to each field. Must return a NamedTuple.
  /// @return New NamedTuple with fields from the results of _f.
  template <typename F>
  auto and_then(const F& _f) && {
    const auto transform_field = [&_f](auto... _fields) {
      return rfl::tuple_cat(_f(std::move(_fields)).fields()...);
    };
    const auto to_nt = []<class... NewFields>(rfl::Tuple<NewFields...>&& _tup) {
      return NamedTuple<NewFields...>(_tup);
    };
    auto new_fields = rfl::apply(transform_field, std::move(*this).fields());
    return to_nt(std::move(new_fields));
  }

  /// @brief Applies a function to each field, returning a new NamedTuple with
  /// the results (const lvalue overload).
  /// @tparam F Callable type.
  /// @param _f Function to apply to each field. Must return a NamedTuple.
  /// @return New NamedTuple with fields from the results of _f.
  template <typename F>
  auto and_then(const F& _f) const& {
    const auto transform_field = [&_f](auto... _fields) {
      return rfl::tuple_cat(_f(std::move(_fields)).fields()...);
    };
    const auto to_nt = []<class... NewFields>(rfl::Tuple<NewFields...>&& _tup) {
      return NamedTuple<NewFields...>(_tup);
    };
    auto new_fields = rfl::apply(transform_field, std::move(fields()));
    return to_nt(std::move(new_fields));
  }

  /// @brief Invokes a callable object once for each field in order.
  /// @tparam F Callable type.
  /// @param _f Function to apply to each field.
  template <typename F>
  void apply(F&& _f) const& {
    const auto apply_to_field = [&_f](const auto&... fields) {
      ((_f(fields)), ...);
    };
    rfl::apply(apply_to_field, fields());
  }

  /// @brief Returns a tuple containing the fields (rvalue overload).
  /// @return Tuple of fields.
  Fields fields() && { return std::move(*this).make_fields(seq_); }

  /// @brief Returns a tuple containing the fields (const lvalue overload).
  /// @return Tuple of fields.
  Fields fields() const& { return make_fields(seq_); }

  /// @brief Gets a field by index (mutable).
  /// @tparam _index Index of the field.
  /// @return Reference to the field.
  template <int _index>
  auto& get() {
    return rfl::get<_index>(*this);
  }

  /// @brief Gets a field by name (mutable).
  /// @tparam _field_name Name of the field.
  /// @return Reference to the field.
  template <internal::StringLiteral _field_name>
  auto& get() {
    return rfl::get<_field_name>(*this);
  }

  /// @brief Gets a field by the field type (mutable).
  /// @tparam Field Field type.
  /// @return Reference to the field.
  template <class Field>
  auto& get() {
    return rfl::get<Field>(*this);
  }

  /// @brief Gets a field by index (const).
  /// @tparam _index Index of the field.
  /// @return Const reference to the field.
  template <int _index>
  const auto& get() const {
    return rfl::get<_index>(*this);
  }

  /// @brief Gets a field by name (const).
  /// @tparam _field_name Name of the field.
  /// @return Const reference to the field.
  template <internal::StringLiteral _field_name>
  const auto& get() const {
    return rfl::get<_field_name>(*this);
  }

  /// @brief Gets a field by the field type (const).
  /// @tparam Field Field type.
  /// @return Const reference to the field.
  template <class Field>
  const auto& get() const {
    return rfl::get<Field>(*this);
  }

  /// @brief Returns the results wrapped in a field.
  /// @tparam _field_name Name of the field.
  /// @return Field containing the value.
  template <internal::StringLiteral _field_name>
  auto get_field() const {
    return rfl::make_field<_field_name>(rfl::get<_field_name>(*this));
  }

  /// @brief Copy assignment operator.
  /// @param _other NamedTuple to copy from.
  /// @return Reference to this NamedTuple.
  NamedTuple<FieldTypes...>& operator=(
      const NamedTuple<FieldTypes...>& _other) = default;

  /// @brief Move assignment operator.
  /// @param _other NamedTuple to move from.
  /// @return Reference to this NamedTuple.
  NamedTuple<FieldTypes...>& operator=(
      NamedTuple<FieldTypes...>&& _other) noexcept = default;

  /// @brief Equality operator.
  /// @param _other NamedTuple to compare with.
  /// @return True if equal, false otherwise.
  inline auto operator==(const rfl::NamedTuple<FieldTypes...>& _other) const {
    return values() == _other.values();
  }

  /// @brief Three-way comparison operator.
  /// @param _other NamedTuple to compare with.
  /// @return Comparison result.
  inline auto operator<=>(const rfl::NamedTuple<FieldTypes...>& _other) const {
    return values() <=> _other.values();
  }

  /// @brief Returns the number of fields, counting ExtraFields individually.
  /// @return Number of fields.
  size_t num_fields() const {
    if constexpr (pos_extra_fields() == -1) {
      return size();
    } else {
      return calc_num_fields<pos_extra_fields()>();
    }
  }

  /// @brief Returns the position of the extra fields, or -1 if there aren't
  /// any.
  /// @return Position of extra fields or -1.
  constexpr static int pos_extra_fields() { return pos_extra_fields_; }

  /// @brief Replaces one or several fields, returning a new version with the
  /// non-replaced fields left unchanged (rvalue overload).
  /// @tparam _name Name of the field to replace.
  /// @tparam FType Type of the field to replace.
  /// @tparam OtherRFields Other fields to replace.
  /// @param _field Field to replace.
  /// @param _other_fields Other fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <internal::StringLiteral _name, class FType, class... OtherRFields>
  auto replace(Field<_name, FType>&& _field,
               OtherRFields&&... _other_fields) && {
    using RField = Field<_name, FType>;
    constexpr auto num_other_fields = sizeof...(OtherRFields);
    if constexpr (num_other_fields == 0) {
      return std::move(*this).template replace_value<RField>(_field.value_);
    } else {
      return std::move(*this)
          .template replace_value<RField>(_field.value_)
          .replace(std::forward<OtherRFields>(_other_fields)...);
    }
  }

  /// @brief Replaces one or several fields, returning a new version with the
  /// non-replaced fields left unchanged (const lvalue overload).
  /// @tparam _name Name of the field to replace.
  /// @tparam FType Type of the field to replace.
  /// @tparam OtherRFields Other fields to replace.
  /// @param _field Field to replace.
  /// @param _other_fields Other fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <internal::StringLiteral _name, class FType, class... OtherRFields>
  auto replace(Field<_name, FType> _field,
               const OtherRFields&... _other_fields) const& {
    using RField = Field<_name, FType>;
    constexpr auto num_other_fields = sizeof...(OtherRFields);
    if constexpr (num_other_fields == 0) {
      return replace_value<RField>(std::move(_field.value_));
    } else {
      return replace_value<RField>(std::move(_field.value_))
          .replace(_other_fields...);
    }
  }

  /// @brief Replace fields from a tuple (rvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @tparam Tail Additional fields to replace.
  /// @param _tuple Tuple of fields to replace.
  /// @param _tail Additional fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <class... TupContent, class... Tail>
  auto replace(rfl::Tuple<TupContent...>&& _tuple, Tail&&... _tail) && {
    if constexpr (sizeof...(Tail) > 0) {
      return std::move(*this)
          .replace_tuple(std::forward<rfl::Tuple<TupContent...>>(_tuple))
          .replace(std::forward<Tail>(_tail)...);
    } else {
      return std::move(*this).replace_tuple(
          std::forward<rfl::Tuple<TupContent...>>(_tuple));
    }
  }

  /// @brief Replace fields from a tuple (const lvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @tparam Tail Additional fields to replace.
  /// @param _tuple Tuple of fields to replace.
  /// @param _tail Additional fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <class... TupContent, class... Tail>
  auto replace(rfl::Tuple<TupContent...> _tuple, const Tail&... _tail) const& {
    if constexpr (sizeof...(Tail) > 0) {
      return replace_tuple(std::move(_tuple)).replace(_tail...);
    } else {
      return replace_tuple(std::move(_tuple));
    }
  }

  /// @brief Replace fields from another NamedTuple (rvalue overload).
  /// @tparam TupContent Types of the NamedTuple fields.
  /// @tparam Tail Additional fields to replace.
  /// @param _named_tuple NamedTuple to replace fields from.
  /// @param _tail Additional fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <class... TupContent, class... Tail>
  auto replace(NamedTuple<TupContent...>&& _named_tuple, Tail&&... _tail) && {
    return std::move(*this).replace(
        std::forward<NamedTuple<TupContent...>>(_named_tuple).fields(),
        std::forward<Tail>(_tail)...);
  }

  /// @brief Replace fields from another NamedTuple (const lvalue overload).
  /// @tparam TupContent Types of the NamedTuple fields.
  /// @tparam Tail Additional fields to replace.
  /// @param _named_tuple NamedTuple to replace fields from.
  /// @param _tail Additional fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <class... TupContent, class... Tail>
  auto replace(NamedTuple<TupContent...> _named_tuple,
               const Tail&... _tail) const& {
    return replace(_named_tuple.fields(), _tail...);
  }

  /// @brief Returns the size of the NamedTuple (number of fields, not counting
  /// ExtraFields individually).
  /// @return Number of fields.
  static constexpr size_t size() { return rfl::tuple_size_v<Values>; }

  /// @brief Applies a function to every field, returning a new NamedTuple with
  /// the results (rvalue overload).
  /// @tparam F Callable type.
  /// @param _f Function to apply to each field.
  /// @return New NamedTuple with transformed fields.
  template <typename F>
  auto transform(const F& _f) && {
    const auto transform_field = [&_f](auto... fields) {
      return rfl::make_tuple(_f(std::move(fields))...);
    };
    const auto to_nt = []<class... NewFields>(rfl::Tuple<NewFields...>&& _tup) {
      return NamedTuple<NewFields...>(_tup);
    };
    auto new_fields = rfl::apply(transform_field, std::move(*this).fields());
    return to_nt(std::move(new_fields));
  }

  /// @brief Applies a function to every field, returning a new NamedTuple with
  /// the results (const lvalue overload).
  /// @tparam F Callable type.
  /// @param _f Function to apply to each field.
  /// @return New NamedTuple with transformed fields.
  template <typename F>
  auto transform(const F& _f) const& {
    const auto transform_field = [&_f](auto... fields) {
      return rfl::make_tuple(_f(std::move(fields))...);
    };
    const auto to_nt = []<class... NewFields>(rfl::Tuple<NewFields...>&& _tup) {
      return NamedTuple<NewFields...>(_tup);
    };
    auto new_fields = rfl::apply(transform_field, std::move(fields()));
    return to_nt(std::move(new_fields));
  }

  /// @brief Returns the underlying rfl::Tuple of values (mutable).
  /// @return Reference to the tuple of values.
  Values& values() { return values_; }

  /// @brief Returns the underlying rfl::Tuple of values (const).
  /// @return Const reference to the tuple of values.
  const Values& values() const { return values_; }

 private:
  /// @brief Adds the elements of a tuple to a newly created NamedTuple (rvalue
  /// overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @param _tuple Tuple of fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent>
  constexpr auto add_tuple(rfl::Tuple<TupContent...>&& _tuple) && {
    const auto a = [this](auto&&... _fields) {
      return std::move(*this).add(std::forward<TupContent>(_fields)...);
    };
    return rfl::apply(a, std::forward<rfl::Tuple<TupContent...>>(_tuple));
  }

  /// @brief Adds the elements of a tuple to a newly created NamedTuple (const
  /// lvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @param _tuple Tuple of fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent>
  constexpr auto add_tuple(rfl::Tuple<TupContent...>&& _tuple) const& {
    const auto a = [this](auto&&... _fields) {
      return this->add(std::forward<TupContent>(_fields)...);
    };
    return rfl::apply(a, std::forward<rfl::Tuple<TupContent...>>(_tuple));
  }

  /// @brief Calculates the number of fields, counting ExtraFields individually.
  /// @tparam _pos Position of ExtraFields.
  /// @return Number of fields.
  template <int _pos>
  size_t calc_num_fields() const {
    const auto& extra_fields = get<_pos>();
    if constexpr (std::is_pointer_v<
                      std::remove_cvref_t<decltype(extra_fields)>>) {
      return size() + extra_fields->size() - 1;
    } else {
      return size() + extra_fields.size() - 1;
    }
  }

  /// @brief Finds the position of the extra fields, or -1 if there aren't any.
  /// @tparam _i Current index.
  /// @tparam _idx Index of ExtraFields found so far.
  /// @return Position of ExtraFields or -1.
  template <int _i = 0, int _idx = -1>
  constexpr static int find_extra_fields() {
    if constexpr (_i == size()) {
      return _idx;
    } else {
      using FieldType = internal::nth_element_t<_i, FieldTypes...>;
      constexpr bool is_extra_fields =
          internal::is_extra_fields_v<typename FieldType::Type>;
      static_assert(_idx == -1 || !is_extra_fields,
                    "There can only be one rfl::ExtraFields in any struct or "
                    "named tuple.");
      if constexpr (is_extra_fields) {
        return find_extra_fields<_i + 1, _i>();
      } else {
        return find_extra_fields<_i + 1, _idx>();
      }
    }
  }

  /// @brief Generates the fields for the NamedTuple (rvalue overload).
  /// @tparam _is Indices of the fields.
  /// @tparam AdditionalArgs Additional arguments to append.
  /// @param _args Additional fields to append.
  /// @return Tuple of fields.
  template <int... _is, class... AdditionalArgs>
  auto make_fields(std::integer_sequence<int, _is...>,
                   AdditionalArgs&&... _args) && {
    const auto wrap = [this]<int _i>(Index<_i>) {
      using FieldType = internal::nth_element_t<_i, FieldTypes...>;
      return FieldType(std::move(rfl::get<_i>(values_)));
    };
    return rfl::make_tuple(wrap(Index<_is>{})...,
                           std::forward<AdditionalArgs>(_args)...);
  }

  /// @brief Generates the fields for the NamedTuple (const lvalue overload).
  /// @tparam _is Indices of the fields.
  /// @tparam AdditionalArgs Additional arguments to append.
  /// @param _args Additional fields to append.
  /// @return Tuple of fields.
  template <int... _is, class... AdditionalArgs>
  auto make_fields(std::integer_sequence<int, _is...>,
                   AdditionalArgs... _args) const& {
    const auto wrap = [this]<int _i>(Index<_i>) {
      using FieldType = internal::nth_element_t<_i, FieldTypes...>;
      return FieldType(rfl::get<_i>(values_));
    };
    return rfl::make_tuple(wrap(Index<_is>{})..., _args...);
  }

  /// @brief Generates a new NamedTuple with one value replaced with a new
  /// value.
  /// @tparam _index Index of the field to replace.
  /// @tparam V Tuple of values.
  /// @tparam T Value to replace with.
  /// @tparam _is Indices of the fields.
  /// @param _values Tuple of values.
  /// @param _val Value to replace with.
  /// @return New NamedTuple with replaced value.
  template <int _index, class V, class T, int... _is>
  auto make_replaced(V&& _values, T&& _val,
                     std::integer_sequence<int, _is...>) const {
    const auto wrap = [&]<int _i>(Index<_i>) {
      if constexpr (_i == _index) {
        return std::forward<T>(_val);
      } else {
        using FieldType = internal::nth_element_t<_i, FieldTypes...>;
        using U = typename FieldType::Type;
        return FieldType(std::forward<U>(rfl::get<_i>(_values)));
      }
    };
    return NamedTuple<FieldTypes...>(wrap(Index<_is>{})...);
  }

  /// @brief Replaces the field signified by the field type (rvalue overload).
  /// @tparam Field Field type to replace.
  /// @tparam T Value to replace with.
  /// @param _val Value to replace with.
  /// @return New NamedTuple with replaced value.
  template <class Field, class T>
  NamedTuple<FieldTypes...> replace_value(T&& _val) && {
    using FieldType = std::remove_cvref_t<Field>;
    constexpr auto index = internal::find_index<FieldType::name_, Fields>();
    return make_replaced<index>(std::move(values_), std::forward<T>(_val),
                                seq_);
  }

  /// @brief Replaces the field signified by the field type (const lvalue
  /// overload).
  /// @tparam Field Field type to replace.
  /// @tparam T Value to replace with.
  /// @param _val Value to replace with.
  /// @return New NamedTuple with replaced value.
  template <class Field, class T>
  NamedTuple<FieldTypes...> replace_value(T&& _val) const& {
    using FieldType = std::remove_cvref_t<Field>;
    constexpr auto index = internal::find_index<FieldType::name_, Fields>();
    auto values = values_;
    return make_replaced<index>(std::move(values), std::forward<T>(_val), seq_);
  }

  /// @brief Replaces fields from a tuple (rvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @param _tuple Tuple of fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <class... TupContent>
  auto replace_tuple(rfl::Tuple<TupContent...>&& _tuple) && {
    const auto r = [this](auto&&... _fields) {
      return std::move(*this).replace(std::forward<TupContent>(_fields)...);
    };
    return rfl::apply(r, std::forward<rfl::Tuple<TupContent...>>(_tuple));
  }

  /// @brief Replaces fields from a tuple (const lvalue overload).
  /// @tparam TupContent Types of the tuple contents.
  /// @param _tuple Tuple of fields to replace.
  /// @return New NamedTuple with replaced fields.
  template <class... TupContent>
  auto replace_tuple(rfl::Tuple<TupContent...>&& _tuple) const& {
    const auto r = [this](auto&&... _fields) {
      return this->replace(std::forward<TupContent>(_fields)...);
    };
    return rfl::apply(r, std::forward<rfl::Tuple<TupContent...>>(_tuple));
  }

  /// @brief Retrieves the fields from another tuple.
  /// @tparam OtherFieldTypes Types of the other tuple's fields.
  /// @tparam _is Indices of the fields.
  /// @param _other_fields Tuple of fields to retrieve from.
  /// @return Tuple of fields matching the current NamedTuple's field types.
  template <class... OtherFieldTypes, int... _is>
  constexpr static Fields retrieve_fields(
      rfl::Tuple<OtherFieldTypes...>&& _other_fields,
      std::integer_sequence<int, _is...>) {
    const auto get_field = [&]<int _i>(Index<_i>) {
      constexpr auto field_name =
          internal::nth_element_t<_i, FieldTypes...>::name_;
      constexpr auto index =
          internal::find_index<field_name, rfl::Tuple<OtherFieldTypes...>>();
      using FieldType = internal::nth_element_t<_i, FieldTypes...>;
      using T = std::remove_cvref_t<typename FieldType::Type>;
      return FieldType(std::forward<T>(rfl::get<index>(_other_fields).value_));
    };
    return rfl::make_tuple(get_field(Index<_is>{})...);
  }

 private:
  /// @brief The values actually contained in the NamedTuple.
  Values values_;

  /// @brief The position of rfl::ExtraFields, or -1 if there aren't any.
  constexpr static int pos_extra_fields_ = find_extra_fields();
};

// ----------------------------------------------------------------------------

/// @brief Specialization for empty NamedTuple.
template <>
class NamedTuple<> {
 public:
  using Fields = rfl::Tuple<>;
  using Names = Literal<>;
  using Values = rfl::Tuple<>;

  /// @brief Default constructor for empty NamedTuple.
  NamedTuple() {}

  /// @brief Destructor.
  ~NamedTuple() = default;

  /// @brief Returns a new NamedTuple with additional fields.
  /// @tparam _name Name of the field to add.
  /// @tparam FType Type of the field to add.
  /// @tparam Tail Additional fields to add.
  /// @param _head Field to add.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <internal::StringLiteral _name, class FType, class... Tail>
  auto add(Field<_name, FType> _head, const Tail&... _tail) const {
    if constexpr (sizeof...(Tail) > 0) {
      return NamedTuple<Field<_name, FType>>(std::move(_head)).add(_tail...);
    } else {
      return NamedTuple<Field<_name, FType>>(std::move(_head));
    }
  }

  /// @brief Add fields from a tuple.
  /// @tparam TupContent Types of the tuple contents.
  /// @tparam Tail Additional fields to add.
  /// @param _tuple Tuple of fields to add.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent, class... Tail>
  auto add(rfl::Tuple<TupContent...> _tuple, const Tail&... _tail) const {
    if constexpr (sizeof...(Tail) > 0) {
      return NamedTuple<TupContent...>(std::move(_tuple)).add(_tail...);
    } else {
      return NamedTuple<TupContent...>(std::move(_tuple));
    }
  }

  /// @brief Add fields from another NamedTuple.
  /// @tparam TupContent Types of the NamedTuple fields.
  /// @tparam Tail Additional fields to add.
  /// @param _named_tuple NamedTuple to add fields from.
  /// @param _tail Additional fields to add.
  /// @return New NamedTuple with added fields.
  template <class... TupContent, class... Tail>
  auto add(NamedTuple<TupContent...> _named_tuple, const Tail&... _tail) const {
    return add(_named_tuple.fields(), _tail...);
  }

  /// @brief Applies a function to each field, returning a new NamedTuple
  /// (always empty).
  /// @tparam F Callable type.
  /// @param _f Function to apply.
  /// @return Empty NamedTuple.
  template <typename F>
  auto and_then(const F&) const {
    return NamedTuple<>();
  }

  /// @brief Does nothing at all (for empty NamedTuple).
  /// @tparam F Callable type.
  /// @param _f Function to apply.
  template <typename F>
  void apply(F&&) const {}

  /// @brief Returns an empty tuple of fields.
  /// @return Empty tuple.
  auto fields() const { return rfl::Tuple(); }

  /// @brief Must always be 0.
  /// @return 0
  size_t num_fields() const { return 0; }

  /// @brief Must always be -1.
  /// @return -1
  constexpr static int pos_extra_fields() { return -1; }

  /// @brief Must always be 0.
  /// @return 0
  static constexpr size_t size() { return 0; }

  /// @brief Returns an empty NamedTuple.
  /// @tparam F Callable type.
  /// @param _f Function to apply.
  /// @return Empty NamedTuple.
  template <typename F>
  auto transform(const F&) const {
    return NamedTuple<>();
  }

  /// @brief Returns an empty tuple of values.
  /// @return Empty tuple.
  auto values() const { return rfl::Tuple(); }
};

// ----------------------------------------------------------------------------

/// @brief Operator* to combine two fields into a NamedTuple.
/// @tparam _name1 Name of first field.
/// @tparam Type1 Type of first field.
/// @tparam _name2 Name of second field.
/// @tparam Type2 Type of second field.
/// @param _f1 First field.
/// @param _f2 Second field.
/// @return NamedTuple containing both fields.
template <internal::StringLiteral _name1, class Type1,
          internal::StringLiteral _name2, class Type2>
inline auto operator*(const rfl::Field<_name1, Type1>& _f1,
                      const rfl::Field<_name2, Type2>& _f2) {
  return NamedTuple(_f1, _f2);
}

/// @brief Operator* to add a field to a NamedTuple.
/// @tparam _name Name of the field.
/// @tparam Type Type of the field.
/// @tparam FieldTypes Types of the NamedTuple fields.
/// @param _tup NamedTuple.
/// @param _f Field to add.
/// @return New NamedTuple with the field added.
template <internal::StringLiteral _name, class Type, class... FieldTypes>
inline auto operator*(const NamedTuple<FieldTypes...>& _tup,
                      const rfl::Field<_name, Type>& _f) {
  return _tup.add(_f);
}

/// @brief Operator* to add a NamedTuple to a field.
/// @tparam _name Name of the field.
/// @tparam Type Type of the field.
/// @tparam FieldTypes Types of the NamedTuple fields.
/// @param _f Field to add.
/// @param _tup NamedTuple.
/// @return New NamedTuple with the field prepended.
template <internal::StringLiteral _name, class Type, class... FieldTypes>
inline auto operator*(const rfl::Field<_name, Type>& _f,
                      const NamedTuple<FieldTypes...>& _tup) {
  return NamedTuple(_f).add(_tup);
}

/// @brief Operator* to combine two NamedTuples.
/// @tparam FieldTypes1 Types of the first NamedTuple fields.
/// @tparam FieldTypes2 Types of the second NamedTuple fields.
/// @param _tup1 First NamedTuple.
/// @param _tup2 Second NamedTuple.
/// @return New NamedTuple with all fields from both.
template <class... FieldTypes1, class... FieldTypes2>
inline auto operator*(const NamedTuple<FieldTypes1...>& _tup1,
                      const NamedTuple<FieldTypes2...>& _tup2) {
  return _tup1.add(_tup2);
}

/// @brief Operator* to combine two fields into a NamedTuple (rvalue overload).
/// @tparam _name1 Name of first field.
/// @tparam Type1 Type of first field.
/// @tparam _name2 Name of second field.
/// @tparam Type2 Type of second field.
/// @param _f1 First field (rvalue).
/// @param _f2 Second field (rvalue).
/// @return NamedTuple containing both fields.
template <internal::StringLiteral _name1, class Type1,
          internal::StringLiteral _name2, class Type2>
inline auto operator*(rfl::Field<_name1, Type1>&& _f1,
                      rfl::Field<_name2, Type2>&& _f2) {
  return NamedTuple(std::forward<Field<_name1, Type1>>(_f1),
                    std::forward<Field<_name2, Type2>>(_f2));
}

/// @brief Operator* to add a field to a NamedTuple (rvalue overload).
/// @tparam _name Name of the field.
/// @tparam Type Type of the field.
/// @tparam FieldTypes Types of the NamedTuple fields.
/// @param _tup NamedTuple (rvalue).
/// @param _f Field to add (rvalue).
/// @return New NamedTuple with the field added.
template <internal::StringLiteral _name, class Type, class... FieldTypes>
inline auto operator*(NamedTuple<FieldTypes...>&& _tup,
                      rfl::Field<_name, Type>&& _f) {
  return _tup.add(std::forward<Field<_name, Type>>(_f));
}

/// @brief Operator* to add a NamedTuple to a field (rvalue overload).
/// @tparam _name Name of the field.
/// @tparam Type Type of the field.
/// @tparam FieldTypes Types of the NamedTuple fields.
/// @param _f Field to add (rvalue).
/// @param _tup NamedTuple (rvalue).
/// @return New NamedTuple with the field prepended.
template <internal::StringLiteral _name, class Type, class... FieldTypes>
inline auto operator*(rfl::Field<_name, Type>&& _f,
                      NamedTuple<FieldTypes...>&& _tup) {
  return NamedTuple(std::forward<Field<_name, Type>>(_f))
      .add(std::forward<NamedTuple<FieldTypes...>>(_tup));
}

/// @brief Operator* to combine two NamedTuples (rvalue overload).
/// @tparam FieldTypes1 Types of the first NamedTuple fields.
/// @tparam FieldTypes2 Types of the second NamedTuple fields.
/// @param _tup1 First NamedTuple (rvalue).
/// @param _tup2 Second NamedTuple (rvalue).
/// @return New NamedTuple with all fields from both.
template <class... FieldTypes1, class... FieldTypes2>
inline auto operator*(NamedTuple<FieldTypes1...>&& _tup1,
                      NamedTuple<FieldTypes2...>&& _tup2) {
  return _tup1.add(std::forward<NamedTuple<FieldTypes2...>>(_tup2));
}

}  // namespace rfl

#endif  // RFL_NAMEDTUPLE_HPP_
