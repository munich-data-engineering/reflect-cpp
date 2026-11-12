#ifndef RFL_JSON_READER_HPP_
#define RFL_JSON_READER_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"
#include "../internal/base64.hpp"

namespace rfl {
namespace json {

/// Reader class for deserializing JSON (JavaScript Object Notation) data.
/// This class provides the interface for parsing JSON format into C++ objects.
/// Uses the yyjson library for high-performance JSON parsing.
struct Reader {
  /// Represents a JSON array during deserialization.
  /// Wraps a pointer to the underlying yyjson value representing the array.
  struct YYJSONInputArray {
    YYJSONInputArray(yyjson_val* _val) : val_(_val) {}
    yyjson_val* val_;
  };

  /// Represents a JSON object during deserialization.
  /// Wraps a pointer to the underlying yyjson value representing the object.
  struct YYJSONInputObject {
    YYJSONInputObject(yyjson_val* _val) : val_(_val) {}
    yyjson_val* val_;
  };

  /// Represents a variant JSON value during deserialization.
  /// Can hold any JSON type (array, object, string, number, boolean, null).
  /// Wraps a pointer to the underlying yyjson value.
  struct YYJSONInputVar {
    YYJSONInputVar() : val_(nullptr) {}
    YYJSONInputVar(yyjson_val* _val) : val_(_val) {}
    yyjson_val* val_;
  };

  using InputArrayType = YYJSONInputArray;
  using InputObjectType = YYJSONInputObject;
  using InputVarType = YYJSONInputVar;

  /// Compile-time flag indicating whether type T has a custom constructor from JSON.
  /// If true, the type provides a static from_json_obj() method for custom deserialization.
  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_json_obj(var); });

  /// Retrieves an element from a JSON array by index.
  /// @param _idx The zero-based index of the element to retrieve
  /// @param _arr The JSON array to read from
  /// @return Result containing the element value, or an error if index is out of bounds
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType _arr) const noexcept {
    const auto var = InputVarType(yyjson_arr_get(_arr.val_, _idx));
    if (!var.val_) {
      return error("Index " + std::to_string(_idx) + " of of bounds.");
    }
    return var;
  }

  /// Retrieves a field from a JSON object by name.
  /// @param _name The name of the field to retrieve
  /// @param _obj The JSON object to read from
  /// @return Result containing the field value, or an error if field is not found
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType _obj) const noexcept {
    const auto var = InputVarType(yyjson_obj_get(_obj.val_, _name.c_str()));
    if (!var.val_) {
      return error("Object contains no field named '" + _name + "'.");
    }
    return var;
  }

  /// Checks if a JSON value is empty or null.
  /// @param _var The JSON value to check
  /// @return true if the value is null or not set, false otherwise
  bool is_empty(const InputVarType _var) const noexcept {
    return !_var.val_ || yyjson_is_null(_var.val_);
  }

  /// Reads all elements from a JSON array using a provided array reader.
  /// The array reader's read() method is called for each element in the array.
  /// @tparam ArrayReader Type that provides a read() method for processing elements
  /// @param _array_reader The reader object used to process each array element
  /// @param _arr The JSON array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    const auto size = yyjson_arr_size(_arr.val_);
    if constexpr (requires { _array_reader.reserve(size); }) {
      if (auto err = _array_reader.reserve(size)) {
        return err;
      }
    }
    yyjson_val* val;
    yyjson_arr_iter iter;
    yyjson_arr_iter_init(_arr.val_, &iter);
    while ((val = yyjson_arr_iter_next(&iter))) {
      auto err = _array_reader.read(InputVarType(val));
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads all key-value pairs from a JSON object using a provided object reader.
  /// The object reader's read() method is called for each key-value pair.
  /// @tparam ObjectReader Type that provides a read() method for processing key-value pairs
  /// @param _object_reader The reader object used to process each field
  /// @param _obj The JSON object to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    yyjson_obj_iter iter;
    yyjson_obj_iter_init(_obj.val_, &iter);
    yyjson_val* key;
    while ((key = yyjson_obj_iter_next(&iter))) {
      const auto name =
          std::string_view(yyjson_get_str(key), yyjson_get_len(key));
      _object_reader.read(name, InputVarType(yyjson_obj_iter_get_val(key)));
    }
    return std::nullopt;
  }

  /// Converts a JSON value to a basic C++ type.
  /// Supports strings, booleans, floating-point numbers, and integers (signed/unsigned).
  /// @tparam T The target C++ type
  /// @param _var The JSON value to convert
  /// @return Result containing the converted value, or an error if conversion fails
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      const auto r = yyjson_get_str(_var.val_);
      if (r == NULL) {
        return error("Could not cast to string.");
      }
      return std::string(r, yyjson_get_len(_var.val_));

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, Bytestring>()) {
      const auto str = yyjson_get_str(_var.val_);
      if (str == nullptr) {
        return error("Could not cast to a Base64 string.");
      }
      return internal::decode_base64(
          std::string_view(str, yyjson_get_len(_var.val_)));

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (!yyjson_is_bool(_var.val_)) {
        return error("Could not cast to boolean.");
      }
      return yyjson_get_bool(_var.val_);

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (!yyjson_is_num(_var.val_)) {
        return error("Could not cast to double.");
      }
      return static_cast<T>(yyjson_get_num(_var.val_));

    } else if constexpr (std::is_unsigned<std::remove_cvref_t<T>>()) {
      if (!yyjson_is_uint(_var.val_)) {
        return error("Could not cast to uint.");
      }
      const auto value = yyjson_get_uint(_var.val_);
      if (value > static_cast<uint64_t>(std::numeric_limits<std::remove_cvref_t<T>>::max())) {
        return error("Integer is out of range for the target type.");
      }
      return static_cast<T>(value);

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if (!yyjson_is_int(_var.val_)) {
        return error("Could not cast to int.");
      }
      const auto value = yyjson_get_sint(_var.val_);
      if (value < std::numeric_limits<std::remove_cvref_t<T>>::min() ||
          value > std::numeric_limits<std::remove_cvref_t<T>>::max()) {
        return error("Integer is out of range for the target type.");
      }
      return static_cast<T>(value);

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  /// Converts a JSON value to an array.
  /// @param _var The JSON value to convert
  /// @return Result containing the array, or an error if value is not an array
  rfl::Result<InputArrayType> to_array(const InputVarType _var) const noexcept {
    if (!yyjson_is_arr(_var.val_)) {
      return error("Could not cast to array!");
    }
    return InputArrayType(_var.val_);
  }

  /// Converts a JSON value to an object.
  /// @param _var The JSON value to convert
  /// @return Result containing the object, or an error if value is not an object
  rfl::Result<InputObjectType> to_object(
      const InputVarType _var) const noexcept {
    if (!yyjson_is_obj(_var.val_)) {
      return error("Could not cast to object!");
    }
    return InputObjectType(_var.val_);
  }

  /// Uses a type's custom constructor to deserialize from JSON.
  /// Calls the type's static from_json_obj() method for custom deserialization logic.
  /// @tparam T The type to construct, must have a from_json_obj() static method
  /// @param _var The JSON value to deserialize from
  /// @return Result containing the constructed object, or an error if construction fails
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType _var) const noexcept {
    try {
      return T::from_json_obj(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

}  // namespace json
}  // namespace rfl

#endif
