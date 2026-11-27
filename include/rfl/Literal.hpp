#ifndef RFL_LITERAL_HPP_
#define RFL_LITERAL_HPP_

#include <compare>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Result.hpp"
#include "Tuple.hpp"
#include "internal/StringLiteral.hpp"
#include "internal/find_index.hpp"
#include "internal/no_duplicate_field_names.hpp"

namespace rfl {

/// Helper struct for Literal - stores a compile-time string name.
template <internal::StringLiteral _name>
struct LiteralHelper {
  constexpr static internal::StringLiteral name_ = _name;
};

/// A type-safe representation of a fixed set of string values known at compile
/// time. Similar to an enum but with string values. Useful for representing
/// discriminators, status values, or any fixed set of string constants with
/// compile-time validation.
/// @tparam fields_ The compile-time string literals that this Literal can
/// represent
template <internal::StringLiteral... fields_>
class Literal {
  using FieldsType = rfl::Tuple<LiteralHelper<fields_>...>;

 public:
  using ValueType = std::conditional_t<sizeof...(fields_) <= 255, std::uint8_t,
                                       std::uint16_t>;

  /// The number of different fields or different options that the literal
  /// can assume.
  static constexpr ValueType num_fields_ = sizeof...(fields_);

  using ReflectionType = std::string;

  /// Copy constructor - constructs a Literal from another literal with the same
  /// fields.
  /// @param _other The literal to copy from
  constexpr Literal(const Literal<fields_...>& _other) = default;

  /// Move constructor - constructs a Literal from another literal with the same
  /// fields.
  /// @param _other The literal to move from
  constexpr Literal(Literal<fields_...>&& _other) noexcept = default;

  /// Constructs a Literal from a string value.
  /// @param _str The string representing one of the literal's allowed values
  /// @throws std::bad_optional_access if the string is not one of the allowed
  /// values
  Literal(const std::string& _str) : value_(find_value(_str).value()) {}

  /// Default constructor - initializes to the first value (index 0).
  constexpr Literal() : value_(0) {}

  /// Destructor.
  ~Literal() = default;

  /// Constructs a new Literal from a compile-time string name.
  /// @tparam _name The compile-time string literal to create a Literal for
  /// @return A Literal representing the specified name
  template <internal::StringLiteral _name>
  static Literal<fields_...> make() {
    return Literal(Literal<fields_...>::template value_of<_name>());
  }

  /// Constructs a new Literal from a compile-time string name.
  /// Equivalent to make(), provided for consistency with from_value().
  /// @tparam _name The compile-time string literal to create a Literal for
  /// @return A Literal representing the specified name
  template <internal::StringLiteral _name>
  static Literal<fields_...> from_name() {
    return Literal<fields_...>::template make<_name>();
  }

  /// Constructs a new Literal from a compile-time value index.
  /// @tparam _value The index of the value (must be less than num_fields_)
  /// @return A Literal representing the value at the specified index
  template <ValueType _value>
  static constexpr Literal<fields_...> from_value() {
    static_assert(_value < num_fields_,
                  "Value cannot exceed number of fields.");
    return Literal<fields_...>(_value);
  }

  /// Constructs a new Literal from a runtime value index.
  /// @param _value The index of the value
  /// @return Result containing the Literal or an error if the value is out of
  /// range
  static Result<Literal<fields_...>> from_value(ValueType _value) {
    if (_value >= num_fields_) {
      return error("Value cannot exceed number of fields.");
    }
    return Literal<fields_...>(_value);
  }

  /// Determines whether the literal contains the string (runtime check).
  /// @param _str The string to check for
  /// @return true if the string is one of the allowed values, false otherwise
  static bool contains(const std::string& _str) {
    bool found = false;
    has_value(_str, &found);
    return found;
  }

  /// Determines whether the literal contains the string at compile time.
  /// @tparam _name The compile-time string literal to check for
  /// @return true if the string is one of the allowed values, false otherwise
  template <internal::StringLiteral _name>
  static constexpr bool contains() {
    return find_value_of<_name>() != -1;
  }

  /// Determines whether the literal contains any of the strings in the other
  /// literal at compile time.
  /// @tparam OtherLiteralType Another Literal type to check against
  /// @return true if there is any overlap between the two literal types
  template <class OtherLiteralType>
  static constexpr bool contains_any() {
    return []<int... _is>(const std::integer_sequence<int, _is...>&) {
      return (false || ... ||
              OtherLiteralType::template contains<
                  find_name_within_own_fields<_is>()>());
    }(std::make_integer_sequence<int, num_fields_>());
  }

  /// Determines whether the literal contains all of the strings in the other
  /// literal at compile time.
  /// @tparam OtherLiteralType Another Literal type to check against
  /// @return true if this literal contains all values from the other literal
  template <class OtherLiteralType>
  static constexpr bool contains_all() {
    return []<int... _is>(const std::integer_sequence<int, _is...>&) {
      return (true && ... &&
              OtherLiteralType::template contains<
                  find_name_within_own_fields<_is>()>());
    }(std::make_integer_sequence<int, num_fields_>());
  }

  /// Determines whether the literal has duplicate strings at compile time.
  /// This is useful for checking collections of strings in other contexts.
  /// @return true if there are duplicate values in the literal
  static constexpr bool has_duplicates() {
    return !internal::no_duplicate_field_names<FieldsType>();
  }

  /// Constructs a Literal from a string. Returns an error if the string
  /// cannot be found.
  /// @param _str The string to convert to a Literal
  /// @return Result containing the Literal or an error if the string is not
  /// recognized
  static Result<Literal> from_string(const std::string& _str) {
    const auto to_literal = [](const auto& _v) {
      return Literal<fields_...>(_v);
    };
    return find_value(_str).transform(to_literal);
  }

  /// The name defined by the Literal (returns the string representation).
  /// @return The string value of the current literal
  std::string name() const { return find_name(); }

  /// Returns all possible values of the literal as a std::vector<std::string>.
  /// @return A vector containing all allowed string values
  static std::vector<std::string> names() {
    return allowed_strings_vec(std::make_integer_sequence<int, num_fields_>());
  }

  /// Helper function to retrieve a name at compile time.
  /// @tparam _value The index of the value to retrieve
  /// @return A Literal containing just the name at the specified index
  template <int _value>
  constexpr static auto name_of() {
    constexpr auto name = find_name_within_own_fields<_value>();
    return Literal<name>();
  }

  /// Copy assignment operator - assigns from another literal.
  /// @param _other The literal to copy from
  /// @return Reference to this literal
  Literal<fields_...>& operator=(const Literal<fields_...>& _other) = default;

  /// Move assignment operator - assigns from another literal.
  /// @param _other The literal to move from
  /// @return Reference to this literal
  Literal<fields_...>& operator=(Literal<fields_...>&& _other) noexcept =
      default;

  /// Assigns the literal from a string.
  /// @param _str The string to assign (must be one of the allowed values)
  /// @return Reference to this literal
  /// @throws std::bad_optional_access if the string is not one of the allowed
  /// values
  Literal<fields_...>& operator=(const std::string& _str) {
    value_ = find_value(_str).value();
    return *this;
  }

  /// Three-way comparison operator for other Literals with the same fields.
  /// Compares by internal value index.
  /// @param _other The other literal to compare with
  /// @return The ordering relationship between the two literals
  auto operator<=>(const Literal<fields_...>& _other) const {
    return value() <=> _other.value();
  }

  /// Three-way comparison operator for other Literals with different fields.
  /// Compares by string name lexicographically.
  /// @tparam _fields The fields of the other Literal type
  /// @param _l2 The other literal to compare with
  /// @return The ordering relationship between the string values
  template <internal::StringLiteral... _fields>
  inline auto operator<=>(const Literal<_fields...>& _l2) const {
    return name() <=> _l2.name();
  }

  /// Three-way comparison operator for strings.
  /// @param _str The string to compare with
  /// @return The ordering relationship between the literal's value and the
  /// string
  inline auto operator<=>(const std::string& _str) const {
#if __cpp_lib_three_way_comparison >= 201907L
    return name() <=> _str;
#else
    auto const& const_name = name();
    if (const_name < _str) {
      return std::strong_ordering::less;
    }
    if (const_name == _str) {
      return std::strong_ordering::equal;
    }
    return std::strong_ordering::greater;
#endif
  }

  /// Three-way comparison operator for const char*.
  /// @tparam other_fields Template parameter for compatibility
  /// @param _str The C-string to compare with
  /// @return The ordering relationship between the literal's value and the
  /// string
  inline auto operator<=>(const char* _str) const {
#if __cpp_lib_three_way_comparison >= 201907L
    return name() <=> _str;
#else
    auto const& const_name = name();
    if (const_name < _str) {
      return std::strong_ordering::less;
    }
    if (const_name == _str) {
      return std::strong_ordering::equal;
    }
    return std::strong_ordering::greater;
#endif
  }

  /// Equality operator - works with Literals, strings, and other comparable
  /// types.
  /// @tparam Other The type to compare with
  /// @param _other The value to compare with
  /// @return true if the values are equal
  template <class Other>
  bool operator==(const Other& _other) const {
    return (*this <=> _other) == 0;
  }

  /// Alias for .name() - returns the string representation for reflection.
  /// @return The string value of the current literal
  std::string reflection() const { return name(); }

  /// Returns the number of fields in the Literal.
  /// @return The total number of possible values this literal can hold
  static constexpr size_t size() { return num_fields_; }

  /// Alias for .name() - returns the string representation.
  /// @return The string value of the current literal
  std::string str() const { return name(); }

  /// Alias for .names() - returns all possible string values.
  /// @return A vector containing all allowed string values
  static std::vector<std::string> strings() {
    return allowed_strings_vec(std::make_integer_sequence<int, num_fields_>());
  }

  /// Returns the value actually contained in the Literal (the internal index).
  /// @return The index representing the current value
  ValueType value() const { return value_; }

  /// Returns the value index of the string literal in the template at compile
  /// time.
  /// @tparam _name The compile-time string literal to get the index for
  /// @return The index of the specified name
  template <internal::StringLiteral _name>
  static constexpr ValueType value_of() {
    constexpr auto value = find_value_of<_name>();
    static_assert(value >= 0, "String not supported.");
    return value;
  }

 private:
  /// Only the static methods are allowed to access this constructor.
  /// @param _value The internal value index
  Literal(const ValueType _value) : value_(_value) {}

  /// Returns all of the allowed fields.
  static std::string allowed_strings() {
    const auto vec =
        allowed_strings_vec(std::make_integer_sequence<int, num_fields_>());
    std::string str;
    for (size_t i = 0; i < vec.size(); ++i) {
      const auto head = "'" + vec[i] + "'";
      str += i == 0 ? head : (", " + head);
    }
    return str;
  }

  /// Returns all of the allowed fields.
  template <int... _is>
  static std::vector<std::string> allowed_strings_vec(
      std::integer_sequence<int, _is...>) {
    std::vector<std::string> values;
    (allowed_strings_vec_add_one<_is>(&values), ...);
    return values;
  }

  template <int _i>
  static void allowed_strings_vec_add_one(std::vector<std::string>* _values) {
    using FieldType = tuple_element_t<_i, FieldsType>;
    _values->emplace_back(FieldType::name_.str());
  }

  /// Finds the correct index associated with
  /// the string at run time.
  std::string find_name() const {
    return find_name_set_str(std::make_integer_sequence<int, num_fields_>());
  }

  template <int... _is>
  std::string find_name_set_str(std::integer_sequence<int, _is...>) const {
    std::string name;
    (find_name_set_if_matches<_is>(&name), ...);
    return name;
  }

  template <int _i>
  void find_name_set_if_matches(std::string* _name) const {
    if (_i == value_) {
      using FieldType = tuple_element_t<_i, FieldsType>;
      *_name = FieldType::name_.str();
    }
  }

  /// Finds the correct index associated with
  /// the string at compile time within the Literal's own fields.
  template <int _i>
  constexpr static auto find_name_within_own_fields() {
    return tuple_element_t<_i, FieldsType>::name_;
  }

  /// Finds the correct value associated with
  /// the string at run time.
  static Result<int> find_value(const std::string& _str) {
    bool found = false;
    const auto idx = find_value_set_idx(
        _str, &found, std::make_integer_sequence<int, num_fields_>());
    if (!found) {
      return error(
          "Literal does not support string '" + _str +
          "'. The following strings are supported: " + allowed_strings() + ".");
    }
    return idx;
  }

  template <int... _is>
  static int find_value_set_idx(const std::string& _str, bool* _found,
                                std::integer_sequence<int, _is...>) {
    int idx = 0;
    (find_value_set_if_matches<_is>(_str, _found, &idx), ...);
    return idx;
  }

  template <int _i>
  static void find_value_set_if_matches(const std::string& _str, bool* _found,
                                        int* _idx) {
    using FieldType = tuple_element_t<_i, FieldsType>;
    if (!*_found && FieldType::name_.string_view() == _str) {
      *_idx = _i;
      *_found = true;
    }
  }

  /// Finds the value of a string literal at compile time.
  template <internal::StringLiteral _name>
  static constexpr int find_value_of() {
    return internal::find_index_or_minus_one<_name, FieldsType>();
  }

  /// Whether the literal contains this string.
  static void has_value(const std::string& _str, bool* _found) {
    find_value_set_idx(_str, _found,
                       std::make_integer_sequence<int, num_fields_>());
  }

  static_assert(sizeof...(fields_) <= std::numeric_limits<ValueType>::max(),
                "Too many fields.");

 private:
  /// The underlying value.
  ValueType value_;
};

/// Helper function to retrieve a name at compile time.
template <class LiteralType, int _value>
inline constexpr auto name_of() {
  return LiteralType::template name_of<_value>();
}

/// Helper function to retrieve a value at compile time.
template <class LiteralType, internal::StringLiteral _name>
inline constexpr auto value_of() {
  return LiteralType::template value_of<_name>();
}

}  // namespace rfl

namespace std {
template <rfl::internal::StringLiteral... fields>
struct hash<rfl::Literal<fields...>> {
  size_t operator()(const rfl::Literal<fields...>& _l) const {
    return hash<int>()(static_cast<int>(_l.value()));
  }
};

}  // namespace std

#endif  // RFL_LITERAL_HPP_
