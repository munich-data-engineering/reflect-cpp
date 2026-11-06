#ifndef RFL_CLI_READER_HPP_
#define RFL_CLI_READER_HPP_

#include <charconv>
#include <concepts>
#include <clocale>
#include <cstdlib>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl::cli {

/// Character used to separate nested field names in CLI arguments.
/// Example: `--database.host` for nested field `database::host`.
static constexpr char path_separator = '.';

/// Character used to delimit array elements in CLI argument values.
/// Example: `--ports=8080,8081,8082` for an array of ports.
static constexpr char array_delimiter = ',';

/// Represents a CLI variable that can be a direct value or a path in the argument map.
/// The `path` represents the hierarchical key (e.g., "database.host").
/// The `direct_value` is used when parsing array elements directly.
struct CliVarType {
  const std::map<std::string, std::string>* const args = nullptr;
  const std::string path;
  const std::optional<std::string> direct_value;
};

/// Represents a CLI object with a prefix path for accessing nested fields.
/// All child fields will be prefixed with this path.
struct CliObjectType {
  const std::map<std::string, std::string>* const args = nullptr;
  const std::string prefix;
};

/// Represents a CLI array containing multiple string values.
/// Typically created by splitting a comma-delimited argument value.
struct CliArrayType {
  const std::vector<std::string> values;
};

// --- Constrained overloads for string-to-type parsing ---

/// Parses a string value to std::string (identity function).
/// @tparam T Must be std::string
/// @param _str The string to parse
/// @param _path The CLI argument path (unused for strings)
/// @return The string unchanged
template <class T> requires std::same_as<T, std::string>
rfl::Result<T> parse_value(
    const std::string& _str, const std::string&
) noexcept {
  return _str;
}

/// Parses a string value to boolean.
/// Accepts: empty/"true"/"1" → true, "false"/"0" → false.
/// @tparam T Must be bool
/// @param _str The string to parse
/// @param _path The CLI argument path (for error messages)
/// @return A Result containing the boolean value or an error
template <class T> requires std::same_as<T, bool>
rfl::Result<T> parse_value(
    const std::string& _str, const std::string& _path
) noexcept {
  if (_str.empty() || _str == "true" || _str == "1") {
    return true;
  }
  if (_str == "false" || _str == "0") {
    return false;
  }
  return error(
      "Could not cast '" + _str + "' to boolean for key '" + _path + "'.");
}

/// Parses a string value to a floating-point number.
/// Uses locale-independent parsing (C locale) to ensure "3.14" works consistently.
/// @tparam T Must be a floating-point type (float, double, long double)
/// @param _str The string to parse
/// @param _path The CLI argument path (for error messages)
/// @return A Result containing the parsed number or an error
// std::from_chars for float/double is unavailable in Apple libc++.
// std::strtod depends on the C locale (LC_NUMERIC), so "3.14" can fail
// under locales that use comma as decimal separator.
// Use strtod_l with an explicit "C" locale on all platforms for consistency.
template <class T> requires (std::is_floating_point_v<T>)
rfl::Result<T> parse_value(
    const std::string& _str, const std::string& _path
) noexcept {
  char* end = nullptr;
#ifdef _WIN32
  const auto c_locale = _create_locale(LC_NUMERIC, "C");
  const double value = _strtod_l(_str.c_str(), &end, c_locale);
  _free_locale(c_locale);
#else
  const auto c_locale = newlocale(LC_NUMERIC_MASK, "C", nullptr);
  const double value = strtod_l(_str.c_str(), &end, c_locale);
  freelocale(c_locale);
#endif
  if (end != _str.c_str() + _str.size()) {
    return error(
        "Could not cast '" + _str + "' to floating point for key '" + _path + "'.");
  }
  return static_cast<T>(value);
}

/// Parses a string value to an integral type (excluding bool).
/// Uses std::from_chars for efficient and locale-independent parsing.
/// @tparam T Must be an integral type other than bool
/// @param _str The string to parse
/// @param _path The CLI argument path (for error messages)
/// @return A Result containing the parsed integer or an error
template <class T> requires (std::is_integral_v<T> && !std::same_as<T, bool>)
rfl::Result<T> parse_value(
    const std::string& _str, const std::string& _path
) noexcept {
  T value;
  const auto [ptr, ec] =
      std::from_chars(_str.data(), _str.data() + _str.size(), value);
  if (ec != std::errc() || ptr != _str.data() + _str.size()) {
    return error(
        "Could not cast '" + _str + "' to integer for key '" + _path + "'.");
  }
  return value;
}

/// Reader for command-line interface arguments.
/// Parses hierarchical key-value pairs from CLI arguments (e.g., --database.host=localhost).
/// Supports arrays through comma-delimited values (e.g., --ports=8080,8081).
struct Reader {
  using InputArrayType = CliArrayType;
  using InputObjectType = CliObjectType;
  using InputVarType = CliVarType;

  template <class T>
  static constexpr bool has_custom_constructor = false;

  /// Gets a specific element from a CLI array by index.
  /// @param _idx The index of the element to retrieve
  /// @param _arr The CLI array
  /// @return A Result containing the element as a CliVarType or an error if out of bounds
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.values.size()) {
      return error(
          std::string("Index ") + std::to_string(_idx) + " out of bounds.");
    }
    return InputVarType{nullptr, "", _arr.values[_idx]};
  }

  /// Gets a specific field from a CLI object by name.
  /// Constructs a child path by appending the field name to the object's prefix.
  /// @param _name The field name
  /// @param _obj The CLI object
  /// @return A Result containing a CliVarType for accessing the field
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    const auto child_path = _obj.prefix.empty()
        ? _name
        : _obj.prefix + _name;
    return InputVarType{_obj.args, child_path, std::nullopt};
  }

  /// Checks if a CLI variable is empty (has no value).
  /// A variable is empty if there's no direct value and no matching key in the argument map.
  /// @param _var The CLI variable to check
  /// @return true if the variable is empty, false otherwise
  bool is_empty(const InputVarType& _var) const noexcept {
    if (_var.direct_value) {
      return false;
    }
    if (!_var.args) {
      return true;
    }
    if (_var.args->count(_var.path)) {
      return false;
    }
    const auto prefix = _var.path + path_separator;
    const auto it = _var.args->lower_bound(prefix);
    return it == _var.args->end()
           || it->first.substr(0, prefix.size()) != prefix;
  }

  /// Reads all elements from a CLI array using the provided array reader.
  /// @tparam ArrayReader The type of reader that processes individual array elements
  /// @param _array_reader The reader object that processes each element
  /// @param _arr The CLI array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(
      const ArrayReader& _array_reader,
      const InputArrayType& _arr
  ) const noexcept {
    for (const auto& val : _arr.values) {
      auto err = _array_reader.read(
          InputVarType{nullptr, "", val});
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads all fields from a CLI object using the provided object reader.
  /// Iterates through all arguments with the object's prefix and extracts child field names.
  /// @tparam ObjectReader The type of reader that processes individual object fields
  /// @param _object_reader The reader object that processes each field
  /// @param _obj The CLI object to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(
      const ObjectReader& _object_reader,
      const InputObjectType& _obj
  ) const noexcept {
    std::set<std::string> seen;
    auto it = _obj.prefix.empty()
        ? _obj.args->begin()
        : _obj.args->lower_bound(_obj.prefix);
    while (it != _obj.args->end()) {
      if (!_obj.prefix.empty()
          && it->first.substr(0, _obj.prefix.size()) != _obj.prefix) {
        break;
      }
      const auto rest = std::string_view(it->first).substr(_obj.prefix.size());
      const auto separator_pos = rest.find(path_separator);
      const auto child = std::string(
          separator_pos == std::string_view::npos
              ? rest
              : rest.substr(0, separator_pos));
      if (!child.empty() && seen.insert(child).second) {
        const auto child_path = _obj.prefix + child;
        _object_reader.read(
            std::string_view(child),
            InputVarType{_obj.args, child_path, std::nullopt});
      }
      ++it;
    }
    return std::nullopt;
  }

  /// Converts a CLI variable to a basic C++ type by parsing the string value.
  /// @tparam T The target type to convert to
  /// @param _var The CLI variable containing the string value
  /// @return A Result containing the converted value or an error
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    const auto str = get_value(_var);
    if (!str) {
      return error("No value for key '" + _var.path + "'.");
    }
    return parse_value<T>(*str, _var.path);
  }

  /// Converts a CLI variable to an array by splitting the value on commas.
  /// @param _var The CLI variable containing a comma-delimited string
  /// @return A Result containing a CliArrayType with the split values
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    const auto str = get_value(_var);
    if (!str) {
      return InputArrayType{{}};
    }
    return InputArrayType{split(*str, array_delimiter)};
  }

  /// Converts a CLI variable to an object for reading nested fields.
  /// Creates a CliObjectType with an appropriate prefix for child field access.
  /// @param _var The CLI variable representing the object
  /// @return A Result containing a CliObjectType or an error
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.args) {
      return error("Cannot convert to object: no argument map available"
                   + (_var.path.empty()
                          ? std::string(".")
                          : " for key '" + _var.path + "'."));
    }
    const auto prefix = _var.path.empty()
        ? std::string("")
        : _var.path + path_separator;
    return InputObjectType{_var.args, prefix};
  }

  /// Custom constructors are not supported for CLI parsing.
  /// @tparam T The type to construct (unused)
  /// @param _var The CLI variable (unused)
  /// @return Always returns an error
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType&
  ) const noexcept {
    return error("Custom constructors are not supported for CLI parsing.");
  }

 private:
  static std::optional<std::string> get_value(
      const InputVarType& _var
  ) noexcept {
    if (_var.direct_value) {
      return *_var.direct_value;
    }
    if (!_var.args) {
      return std::nullopt;
    }
    const auto it = _var.args->find(_var.path);
    if (it == _var.args->end()) {
      return std::nullopt;
    }
    return it->second;
  }

  static std::vector<std::string> split(
      const std::string& _str, char _delim
  ) {
    std::vector<std::string> result;
    if (_str.empty()) {
      return result;
    }
    size_t start = 0;
    while (true) {
      const auto pos = _str.find(_delim, start);
      if (pos == std::string::npos) {
        if (start < _str.size()) {
          result.emplace_back(_str.substr(start));
        }
        break;
      }
      if (pos > start) {
        result.emplace_back(_str.substr(start, pos - start));
      }
      start = pos + 1;
    }
    return result;
  }

};

}  // namespace rfl::cli

#endif
