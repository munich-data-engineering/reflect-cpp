#ifndef RFL_GENERIC_HPP_
#define RFL_GENERIC_HPP_

#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <cstdint>

#include "Object.hpp"
#include "Result.hpp"
#include "common.hpp"

namespace rfl {

/// This is the declaration of the `Generic` class, which serves as a
/// type-agnostic container for various data types. It wraps a `std::variant`
/// that can hold primitive types (bool, int, double, string), null, nested
/// objects, or arrays. This class is typically used in
/// serialization/deserialization libraries to represent JSON or other formats
/// where types are not known at compile time. The class provides constructors
/// to initialize from various types and accessors to retrieve the underlying
/// variant. It mimics the behavior of a `std::optional` or `std::expected` by
/// providing methods like `get()`, `value()`, and `is_null()`. The
/// `VariantType` alias defines the specific set of types this container can
/// hold. The `ReflectionType` alias allows for optional initialization, useful
/// when a value might be absent. Public member functions include constructors,
/// destructor, assignment operators, and accessors. The class is marked with
/// `RFL_API` for DLL export/import support.
class RFL_API Generic {
 public:
  constexpr static std::nullopt_t Null = std::nullopt;

  using Array = std::vector<Generic>;
  using Object = rfl::Object<Generic>;
  using VariantType = std::variant<bool, int64_t, double, std::string, Object,
                                   Array, std::nullopt_t>;
  using ReflectionType = std::optional<
      std::variant<bool, int64_t, double, std::string, Object, Array>>;

  /// Default constructor - creates a Generic with null value.
  Generic();

  /// Move constructor.
  /// @param _other The Generic to move from
  Generic(Generic&& _other) noexcept;

  /// Copy constructor.
  /// @param _other The Generic to copy from
  Generic(const Generic& _other);

  /// Constructs from a variant (copy).
  /// @param _value The variant value to wrap
  Generic(const VariantType& _value);

  /// Constructs from a variant (move).
  /// @param _value The variant value to wrap (will be moved)
  Generic(VariantType&& _value) noexcept;

  /// Constructs from a reflection type.
  /// @param _value The reflection type value (optional variant)
  Generic(const ReflectionType& _value);

  /// Constructs from any type convertible to VariantType (copy).
  /// @tparam T The type to convert from
  /// @param _value The value to convert and wrap
  template <class T>
    requires std::is_convertible_v<T, VariantType>
  Generic(const T& _value) {
    value_ = _value;
  }

  /// Constructs from any type convertible to VariantType (move).
  /// @tparam T The type to convert from
  /// @param _value The value to convert and wrap (will be moved)
  template <class T>
    requires std::is_convertible_v<T, VariantType>
  Generic(T&& _value) noexcept : value_(std::forward<T>(_value)) {}

  /// Destructor.
  ~Generic();

  /// Returns the underlying object.
  const VariantType& get() const noexcept { return value_; }

  /// Returns the underlying object.
  VariantType& get() noexcept { return value_; }

  /// Returns the underlying object.
  VariantType& operator*() noexcept { return value_; }

  /// Returns the underlying object.
  const VariantType& operator*() const noexcept { return value_; }

  /// Returns the underlying object.
  VariantType& operator()() noexcept { return value_; }

  /// Returns the underlying object.
  const VariantType& operator()() const noexcept { return value_; }

  /// Returns the underlying object.
  VariantType& value() noexcept { return value_; }

  /// Returns the underlying object.
  const VariantType& value() const noexcept { return value_; }

  /// Whether the object contains the null value.
  bool is_null() const noexcept;

  /// Assigns the underlying object.
  /// @param _value The variant value to assign
  /// @return Reference to this object
  Generic& operator=(const VariantType& _value);

  /// Assigns the underlying object (move version).
  /// @param _value The variant value to assign (will be moved)
  /// @return Reference to this object
  Generic& operator=(VariantType&& _value) noexcept;

  /// Assigns from any type convertible to VariantType.
  /// Handles special conversions for numeric types to ensure proper variant
  /// alternative selection.
  /// @tparam T The type to convert from
  /// @param _value The value to assign
  /// @return Reference to this object
  template <class T>
    requires std::is_convertible_v<T, VariantType>
  auto& operator=(const T& _value) {
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, bool>) {
      value_.emplace<0>(_value);
    } else if constexpr (std::is_integral_v<Type>) {
      value_.emplace<1>(static_cast<int64_t>(_value));
    } else if constexpr (std::is_floating_point_v<Type>) {
      value_.emplace<2>(static_cast<double>(_value));
    } else {
      value_ = _value;
    }
    return *this;
  }

  /// Copy assignment operator.
  /// @param _other The Generic to copy from
  /// @return Reference to this object
  Generic& operator=(const Generic& _other);

  /// Move assignment operator.
  /// @param _other The Generic to move from
  /// @return Reference to this object
  Generic& operator=(Generic&& _other);

  /// Returns the underlying object, necessary for the serialization to work.
  ReflectionType reflection() const noexcept;

  /// Casts the underlying value to an rfl::Generic::Array or returns an
  /// rfl::Error, if the underlying value is not an rfl::Generic::Array.
  Result<Array> to_array() const noexcept {
    if (auto* ptr = std::get_if<Array>(&value_)) return *ptr;
    return error(
        "rfl::Generic: Could not cast the underlying value to an "
        "rfl::Generic::Array.");
  }

  /// Casts the underlying value to a boolean or returns an rfl::Error, if the
  /// underlying value is not a boolean.
  Result<bool> to_bool() const noexcept {
    if (auto* ptr = std::get_if<bool>(&value_)) return *ptr;
    return error(
        "rfl::Generic: Could not cast the underlying value to a boolean.");
  }

  /// Casts the underlying value to a double or returns an rfl::Error, if the
  /// underlying value is not a number or the conversion would result in loss of
  /// precision.
  Result<double> to_double() const noexcept;

  /// Casts the underlying value to an integer or returns an rfl::Error, if the
  /// underlying value is not an integer.
  Result<int> to_int() const noexcept;

  /// Casts the underlying value to an int64 or returns an rfl::Error, if the
  /// underlying value is not an integer.
  Result<int64_t> to_int64() const noexcept;

  /// Casts the underlying value to an rfl::Generic::Object or returns an
  /// rfl::Error, if the underlying value is not an rfl::Generic::Object.
  Result<Object> to_object() const noexcept;

  /// Casts the underlying value to rfl::Generic::Null or returns an
  /// rfl::Error, if the underlying value is not rfl::Generic::Null.
  Result<std::nullopt_t> to_null() const noexcept;

  /// Casts the underlying value to a string or returns an rfl::Error, if the
  /// underlying value is not a string.
  Result<std::string> to_string() const noexcept;

  /// Returns the underlying variant.
  VariantType& variant() noexcept { return value_; }

  /// Returns the underlying variant.
  const VariantType& variant() const noexcept { return value_; }

  /// Equality comparison. Two Generics are equal if they hold the same
  /// alternative and the underlying values compare equal.
  /// @param _lhs The left-hand side Generic
  /// @param _rhs The right-hand side Generic
  /// @return true if both Generics hold equal values
  friend bool operator==(const Generic& _lhs, const Generic& _rhs) {
    return is_same(_lhs, _rhs);
  }

 private:
  /// Converts a ReflectionType to a VariantType.
  /// @param _r The reflection type (optional variant) to convert
  /// @return The converted variant type
  static VariantType from_reflection_type(const ReflectionType& _r) noexcept;

  /// Checks if two Generic instances are equal by comparing their type indices
  /// and their underlying values. This is used to implement the equality
  /// operator for the Generic class.
  /// @param _lhs The left-hand side Generic instance to compare
  /// @param _rhs The right-hand side Generic instance to compare
  /// @return true if both instances have the same type and equal values, false
  /// otherwise
  static bool is_same(const Generic& _lhs, const Generic& _rhs);

 private:
  VariantType value_;
};

/// Casts the underlying value to an rfl::Generic::Array or returns an
/// rfl::Error, if the underlying value is not an rfl::Generic::Array.
inline Result<Generic::Array> to_array(const Generic& _g) noexcept {
  return _g.to_array();
}

/// Casts the underlying value to a boolean or returns an rfl::Error, if the
/// underlying value is not a boolean.
inline Result<bool> to_bool(const Generic& _g) noexcept { return _g.to_bool(); }

/// Casts the underlying value to a double or returns an rfl::Error, if the
/// underlying value is not a double.
inline Result<double> to_double(const Generic& _g) noexcept {
  return _g.to_double();
}

/// Casts the underlying value to an integer or returns an rfl::Error, if the
/// underlying value is not an integer.
inline Result<int> to_int(const Generic& _g) noexcept { return _g.to_int(); }

/// Casts the underlying value to an int64 or returns an rfl::Error, if the
/// underlying value is not an integer.
inline Result<long> to_int64(const Generic& _g) noexcept {
  return _g.to_int64();
}

/// Casts the underlying value to an rfl::Generic::Object or returns an
/// rfl::Error, if the underlying value is not an rfl::Generic::Object.
inline Result<Generic::Object> to_object(const Generic& _g) noexcept {
  return _g.to_object();
}

/// Casts the underlying value to a double or returns an rfl::Error, if the
/// underlying value is not a double.
inline Result<std::nullopt_t> to_null(const Generic& _g) noexcept {
  return _g.to_null();
}

/// Casts the underlying value to a string or returns an rfl::Error, if the
/// underlying value is not a string.
inline Result<std::string> to_string(const Generic& _g) noexcept {
  return _g.to_string();
}

}  // namespace rfl

#endif
