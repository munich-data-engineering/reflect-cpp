#ifndef GENERIC_READER_HPP_
#define GENERIC_READER_HPP_

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Generic.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl::generic {

/// Reader for the Generic type - a dynamic, format-agnostic data structure.
/// The Generic type can represent any JSON-like structure (objects, arrays, primitives)
/// without being tied to a specific serialization format.
struct Reader {
  using InputArrayType = Generic::Array;
  using InputObjectType = Generic::Object;
  using InputVarType = Generic;

  template <class T>
  static constexpr bool has_custom_constructor = false;

  /// Gets a specific element from a Generic array by index.
  /// @param _idx The index of the element to retrieve
  /// @param _arr The Generic array
  /// @return A Result containing the element or an error if out of bounds
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.size()) {
      return error("Index " + std::to_string(_idx) + " of of bounds.");
    }
    return _arr[_idx];
  }

  /// Gets a specific field from a Generic object by name.
  /// @param _name The field name
  /// @param _obj The Generic object
  /// @return A Result containing the field value or an error if not found
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    return _obj.get(_name);
  }

  /// Checks if the Generic value is empty (null).
  /// @param _var The Generic value to check
  /// @return true if the value is null, false otherwise
  bool is_empty(const InputVarType& _var) const noexcept {
    return _var.is_null();
  }

  /// Converts a Generic value to a basic C++ type.
  /// Supports strings, booleans, floating-point numbers, and integers.
  /// @tparam T The target C++ type to convert to
  /// @param _var The Generic value to convert
  /// @return A Result containing the converted value or an error
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      return _var.to_string();
    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      return _var.to_bool();
    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      return _var.to_double().transform(
          [](const auto& _v) { return static_cast<T>(_v); });
    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if constexpr (sizeof(T) > sizeof(int)) {
        return _var.to_int64().transform(
            [](const auto& _v) { return static_cast<T>(_v); });
      } else {
        return _var.to_int().transform(
            [](const auto& _v) { return static_cast<T>(_v); });
      }
    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  /// Reads all elements from a Generic array using the provided array reader.
  /// @tparam ArrayReader The type of reader that processes individual array elements
  /// @param _array_reader The reader object that processes each element
  /// @param _arr The Generic array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    if constexpr (requires { _array_reader.reserve(_arr.size()); }) {
      if (auto err = _array_reader.reserve(_arr.size())) {
        return err;
      }
    }
    for (const auto& v : _arr) {
      auto err = _array_reader.read(InputVarType(v));
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads all key-value pairs from a Generic object using the provided object reader.
  /// @tparam ObjectReader The type of reader that processes individual object fields
  /// @param _object_reader The reader object that processes each field
  /// @param _obj The Generic object to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (const auto& [k, v] : _obj) {
      _object_reader.read(std::string_view(k), v);
    }
    return std::nullopt;
  }

  /// Converts a Generic value to an array type.
  /// @param _var The Generic value to convert
  /// @return A Result containing a Generic::Array or an error if not an array
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    return _var.to_array();
  }

  /// Converts a Generic value to an object type.
  /// @param _var The Generic value to convert
  /// @return A Result containing a Generic::Object or an error if not an object
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    return _var.to_object();
  }

  /// Custom constructors are not supported for generic types.
  /// @tparam T The type to construct (unused)
  /// @param _var The Generic value (unused)
  /// @return Always returns an error
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType /*_var*/) const noexcept {
    return error("Not supported for generic types");
  }
};

}  // namespace rfl::generic

#endif
