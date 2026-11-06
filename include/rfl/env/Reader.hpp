#ifndef RFL_ENV_READER_HPP_
#define RFL_ENV_READER_HPP_

#include <locale.h>

#include <charconv>
#include <clocale>
#include <concepts>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Result.hpp"

#if defined(__APPLE__)
#include <crt_externs.h>
inline char** get_environ() noexcept { return *_NSGetEnviron(); }
#elif defined(_WIN32)
#include <stdlib.h>
inline char** get_environ() noexcept { return _environ; }
#else
extern char** environ;
inline char** get_environ() noexcept { return environ; }
#endif

namespace rfl::env {

/// Represents a ENV variable that can be a direct value or a path in the
/// argument map. The `path` represents the hierarchical key (e.g.,
/// "DATABASE_HOST").
struct InputEnvVarType {
  std::string path;
};

/// Represents a ENV object with a prefix path for accessing nested fields.
/// All child fields will be prefixed with this path.
struct InputEnvObjectType {
  std::string prefix;
};

/// Represents a ENV array containing multiple string values.
struct InputEnvArrayType {
  std::string prefix;
};

// --- Constrained overloads for string-to-type parsing ---

/// Parses a string value to std::string (identity function).
/// @tparam T Must be std::string
/// @param _str The string to parse
/// @param _path The ENV argument path (unused for strings)
/// @return The string unchanged
template <class T>
  requires std::same_as<T, std::string>
rfl::Result<T> parse_value(const std::string& _str,
                           const std::string&) noexcept {
  return _str;
}

/// Parses a string value to boolean.
/// Accepts: empty/"true"/"1" → true, "false"/"0" → false.
/// @tparam T Must be bool
/// @param _str The string to parse
/// @param _path The ENV argument path (for error messages)
/// @return A Result containing the boolean value or an error
template <class T>
  requires std::same_as<T, bool>
rfl::Result<T> parse_value(const std::string& _str,
                           const std::string& _path) noexcept {
  if (_str.empty() || _str == "true" || _str == "1") {
    return true;
  }
  if (_str == "false" || _str == "0") {
    return false;
  }
  return error("Could not cast '" + _str + "' to boolean for key '" + _path +
               "'.");
}

/// Parses a string value to a floating-point number.
/// Uses locale-independent parsing (C locale) to ensure "3.14" works
/// consistently.
/// @tparam T Must be a floating-point type (float, double, long double)
/// @param _str The string to parse
/// @param _path The ENV argument path (for error messages)
/// @return A Result containing the parsed number or an error
// std::from_chars for float/double is unavailable in Apple libc++.
// std::strtod depends on the C locale (LC_NUMERIC), so "3.14" can fail
// under locales that use comma as decimal separator.
// Use strtod_l with an explicit "C" locale on all platforms for consistency.
template <class T>
  requires(std::is_floating_point_v<T>)
rfl::Result<T> parse_value(const std::string& _str,
                           const std::string& _path) noexcept {
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
    return error("Could not cast '" + _str + "' to floating point for key '" +
                 _path + "'.");
  }
  return static_cast<T>(value);
}

/// Parses a string value to an integral type (excluding bool).
/// Uses std::from_chars for efficient and locale-independent parsing.
/// @tparam T Must be an integral type other than bool
/// @param _str The string to parse
/// @param _path The ENV argument path (for error messages)
/// @return A Result containing the parsed integer or an error
template <class T>
  requires(std::is_integral_v<T> && !std::same_as<T, bool>)
rfl::Result<T> parse_value(const std::string& _str,
                           const std::string& _path) noexcept {
  T value;
  const auto [ptr, ec] =
      std::from_chars(_str.data(), _str.data() + _str.size(), value);
  if (ec != std::errc() || ptr != _str.data() + _str.size()) {
    return error("Could not cast '" + _str + "' to integer for key '" + _path +
                 "'.");
  }
  return value;
}

/// Parser for environment variables. Provides methods to read fields from ENV
/// arrays and objects, check for empty variables, and convert string values to
/// basic C++ types.
struct Reader {
  using InputArrayType = InputEnvArrayType;
  using InputObjectType = InputEnvObjectType;
  using InputVarType = InputEnvVarType;

  template <class T>
  static constexpr bool has_custom_constructor = false;

  /// Gets a specific element from a ENV array by index.
  /// @param _idx The index of the element to retrieve
  /// @param _arr The ENV array
  /// @return A Result containing the element as a CliVarType or an error if out
  /// of bounds
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    return InputVarType{.path = _arr.prefix + std::to_string(_idx)};
  }

  /// Gets a specific field from a ENV object by name.
  /// Constructs a child path by appending the field name to the object's
  /// prefix.
  /// @param _name The field name
  /// @param _obj The ENV object
  /// @return A Result containing a CliVarType for accessing the field
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    const auto child_path = _obj.prefix.empty() ? _name : _obj.prefix + _name;
    return InputVarType{.path = child_path};
  }

  /// Checks if a ENV variable is empty (has no value).
  /// A variable is empty if there's no direct value and no matching key in the
  /// argument map.
  /// @param _var The ENV variable to check
  /// @return true if the variable is empty, false otherwise
  bool is_empty(const InputVarType& _var) const noexcept {
    const auto str = std::getenv(_var.path.c_str());
    if (str != nullptr) {
      return false;
    }
    if (_var.path.empty()) {
      return true;
    }
    const auto prefix = _var.path.empty() ? std::string("") : _var.path + "_";
    for (char** env = get_environ(); *env != nullptr; ++env) {
      const std::string env_entry(*env);
      if (env_entry.size() <= prefix.size()) {
        continue;
      }
      if (env_entry.compare(0, prefix.size(), prefix) == 0) {
        return false;
      }
    }
    return true;
  }

  /// Reads all elements from a ENV array using the provided array reader.
  /// @tparam ArrayReader The type of reader that processes individual array
  /// elements
  /// @param _array_reader The reader object that processes each element
  /// @param _arr The ENV array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    size_t idx = 0;
    while (true) {
      const auto var = InputVarType{.path = _arr.prefix + std::to_string(idx)};
      if (is_empty(var)) {
        break;
      }
      auto err = _array_reader.read(var);
      if (err) {
        return err;
      }
      ++idx;
    }
    return std::nullopt;
  }

  /// Reads all fields from a ENV object using the provided object reader.
  /// Iterates through all arguments with the object's prefix and extracts child
  /// field names.
  /// @tparam ObjectReader The type of reader that processes individual object
  /// fields
  /// @param _object_reader The reader object that processes each field
  /// @param _obj The ENV object to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    constexpr bool has_view_type =
        requires { typename std::remove_cvref_t<ObjectReader>::ViewType; };

    if constexpr (has_view_type) {
      using ViewType = typename std::remove_cvref_t<ObjectReader>::ViewType;
      using NamesType = typename ViewType::Names;
      const auto names = NamesType::names();
      for (const auto& name : names) {
        const auto child_path = _obj.prefix.empty() ? name : _obj.prefix + name;
        const auto var = InputVarType{.path = child_path};
        if (!is_empty(var)) {
          _object_reader.read(std::string_view(name), var);
        }
      }

    } else {
      for (char** env = get_environ(); *env != nullptr; ++env) {
        const std::string env_entry(*env);
        if (env_entry.size() <= _obj.prefix.size()) {
          continue;
        }
        if (env_entry.compare(0, _obj.prefix.size(), _obj.prefix) == 0) {
          const auto pos_eq = env_entry.find('=');
          if (pos_eq == std::string::npos) {
            continue;
          }
          const auto name =
              env_entry.substr(_obj.prefix.size(), pos_eq - _obj.prefix.size());
          const auto var = InputVarType{.path = env_entry.substr(0, pos_eq)};
          _object_reader.read(std::string_view(name), var);
        }
      }
    }

    return std::nullopt;
  }

  /// Converts a ENV variable to a basic C++ type by parsing the string value.
  /// @tparam T The target type to convert to
  /// @param _var The ENV variable containing the string value
  /// @return A Result containing the converted value or an error
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    const auto str = get_value(_var);
    if (!str) {
      return error("No value for key '" + _var.path + "'.");
    }
    return parse_value<T>(*str, _var.path);
  }

  /// Converts a ENV variable to an array by splitting the value on commas.
  /// @param _var The ENV variable containing a comma-delimited string
  /// @return A Result containing a CliArrayType with the split values
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    const auto prefix = _var.path.empty() ? std::string("") : _var.path + "_";
    return InputArrayType{.prefix = prefix};
  }

  /// Converts a ENV variable to an object for reading nested fields.
  /// Creates a EnvObjectType with an appropriate prefix for child field access.
  /// @param _var The ENV variable representing the object
  /// @return A Result containing a CliObjectType or an error
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    const auto prefix = _var.path.empty() ? std::string("") : _var.path + "_";
    return InputObjectType{.prefix = prefix};
  }

  /// Custom constructors are not supported for ENV parsing.
  /// @tparam T The type to construct (unused)
  /// @param _var The ENV variable (unused)
  /// @return Always returns an error
  template <class T>
  rfl::Result<T> use_custom_constructor(const InputVarType&) const noexcept {
    return error("Custom constructors are not supported for ENV parsing.");
  }

 private:
  static std::optional<std::string> get_value(
      const InputVarType& _var) noexcept {
    const char* env_value = std::getenv(_var.path.c_str());
    if (env_value) {
      return std::string(env_value);
    } else {
      return std::nullopt;
    }
  }
};

}  // namespace rfl::env

#endif
