#ifndef RFL_VALIDATOR_HPP_
#define RFL_VALIDATOR_HPP_

#include <cstddef>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

#include "AllOf.hpp"
#include "Result.hpp"
#include "internal/HasValidation.hpp"

namespace rfl {

/// A wrapper that validates a value against one or more validation rules.
/// The value is checked on construction and assignment, and will throw if
/// validation fails.
/// @tparam T The underlying type to validate
/// @tparam V The primary validator type
/// @tparam Vs Additional validator types (combined with AllOf)
template <class T, class V, class... Vs>
  requires internal::HasValidation<AllOf<V, Vs...>, T>
struct Validator {
 public:
  using ReflectionType = T;
  using ValidationType =
      std::conditional_t<sizeof...(Vs) == 0, V, AllOf<V, Vs...>>;

  /// Exception-free validation from a value.
  /// @param _value The value to validate
  /// @return Result containing a Validator if validation succeeds, or an error
  /// if it fails
  static Result<Validator> from_value(const T& _value) noexcept {
    try {
      return Validator(_value);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }

  /// Default constructor - validates a default-constructed T.
  /// @throws std::exception if validation of the default value fails
  Validator() : value_(ValidationType::validate(T()).value()) {}

  /// Move constructor.
  /// @param _other The validator to move from
  Validator(Validator&& _other) noexcept = default;

  /// Copy constructor.
  /// @param _other The validator to copy from
  Validator(const Validator& _other) = default;

  /// Constructs from an rvalue of T (validates the value).
  /// @param _value The value to validate and wrap
  /// @throws std::exception if validation fails
  Validator(T&& _value)
      : value_(ValidationType::validate(std::move(_value)).value()) {}

  /// Constructs from a const reference to T (validates the value).
  /// @param _value The value to validate and wrap
  /// @throws std::exception if validation fails
  Validator(const T& _value)
      : value_(ValidationType::validate(_value).value()) {}

  /// Constructs from a type convertible to T (validates after conversion).
  /// @tparam U Type convertible to T
  /// @param _value The value to convert, validate, and wrap
  /// @throws std::exception if validation fails
  template <class U>
    requires std::is_convertible_v<U, T>
  Validator(U&& _value)
      : value_(ValidationType::validate(T(std::forward<U>(_value))).value()) {}

  /// Constructs from a const reference to a type convertible to T.
  /// @tparam U Type convertible to T
  /// @param _value The value to convert, validate, and wrap
  /// @throws std::exception if validation fails
  template <class U>
    requires std::is_convertible_v<U, T>
  Validator(const U& _value)
      : value_(ValidationType::validate(T(_value)).value()) {}

  /// Destructor.
  ~Validator() = default;

  /// Assigns a new value (validates before assigning).
  /// @param _value The value to validate and assign
  /// @return Reference to this validator
  /// @throws std::exception if validation fails
  auto& operator=(const T& _value) {
    value_ = ValidationType::validate(_value).value();
    return *this;
  }

  /// Assigns a new value (move version, validates before assigning).
  /// @param _value The value to validate and assign
  /// @return Reference to this validator
  /// @throws std::exception if validation fails
  auto& operator=(T&& _value) {
    value_ = ValidationType::validate(std::forward<T>(_value)).value();
    return *this;
  }

  /// Copy assignment operator.
  /// @param _other The validator to copy from
  /// @return Reference to this validator
  Validator& operator=(const Validator& _other) = default;

  /// Move assignment operator.
  /// @param _other The validator to move from
  /// @return Reference to this validator
  Validator& operator=(Validator&& _other) noexcept = default;

  /// Assigns from a convertible type (validates after conversion).
  /// @tparam U Type convertible to T
  /// @param _value The value to convert, validate, and assign
  /// @return Reference to this validator
  /// @throws std::exception if validation fails
  template <class U>
    requires std::is_convertible_v<U, T>
  auto& operator=(U&& _value) {
    value_ = ValidationType::validate(T(std::forward<U>(_value))).value();
    return *this;
  }

  /// Assigns from a const reference to a convertible type.
  /// @tparam U Type convertible to T
  /// @param _value The value to convert, validate, and assign
  /// @return Reference to this validator
  /// @throws std::exception if validation fails
  template <class U>
    requires std::is_convertible_v<U, T>
  auto& operator=(const U& _value) {
    value_ = ValidationType::validate(T(_value)).value();
    return *this;
  }

  /// Equality operator for comparing with other Validators.
  /// @param _other The other validator to compare with
  /// @return true if the underlying values are equal
  bool operator==(const Validator& _other) const {
    return value() == _other.value();
  }

  /// Returns the underlying object.
  const T& get() const noexcept { return value_; }

  /// Returns the underlying object.
  T& get() noexcept { return value_; }

  /// Returns the underlying object.
  T& operator*() noexcept { return value_; }

  /// Returns the underlying object.
  const T& operator*() const noexcept { return value_; }

  /// Returns the underlying object.
  T& operator()() noexcept { return value_; }

  /// Returns the underlying object.
  const T& operator()() const noexcept { return value_; }

  /// Pointer to the underlying value.
  /// @return Pointer to the stored value
  T* operator->() noexcept { return &value_; }

  /// Pointer to the underlying value (const).
  /// @return Const pointer to the stored value
  const T* operator->() const noexcept { return &value_; }

  /// Exposes the underlying value.
  T& value() noexcept { return value_; }

  /// Exposes the underlying value.
  const T& value() const noexcept { return value_; }

  /// Necessary for the serialization to work.
  const T& reflection() const { return value_; }

 private:
  /// The underlying value.
  T value_;
};

/// Three-way comparison operator for validators with the same validation rules.
/// @tparam T The underlying type
/// @tparam V The primary validator type
/// @tparam Vs Additional validator types
/// @param _v1 The first validator to compare
/// @param _v2 The second validator to compare
/// @return The ordering relationship between the underlying values
template <class T, class V, class... Vs>
inline auto operator<=>(const Validator<T, V, Vs...>& _v1,
                        const Validator<T, V, Vs...>& _v2) {
  return _v1.value() <=> _v2.value();
}

/// Three-way comparison operator for comparing a validator with a raw value.
/// @tparam T The underlying type
/// @tparam V The primary validator type
/// @tparam Vs Additional validator types
/// @param _v The validator to compare
/// @param _t The raw value to compare with
/// @return The ordering relationship between the validator's value and the raw
/// value
template <class T, class V, class... Vs>
inline auto operator<=>(const Validator<T, V, Vs...>& _v, const T& _t) {
  return _v.value() <=> _t;
}

}  // namespace rfl

namespace std {

/// Specialization of std::hash for Validator.
/// Allows Validator to be used in unordered containers.
template <class T, class V, class... Vs>
struct hash<rfl::Validator<T, V, Vs...>> {
  /// Computes hash of the validator by hashing its underlying value.
  /// @param _v The validator to hash
  /// @return The hash value
  size_t operator()(const rfl::Validator<T, V, Vs...>& _v) const {
    return hash<T>()(_v.value());
  }
};

}  // namespace std

#endif
