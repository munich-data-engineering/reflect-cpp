#ifndef RFL_MSGPACK_READER_HPP_
#define RFL_MSGPACK_READER_HPP_

#include <msgpack.h>

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <string_view>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../concepts.hpp"
#include "../internal/ptr_cast.hpp"

namespace rfl {
namespace msgpack {

struct Reader {
  using InputArrayType = msgpack_object_array;
  using InputObjectType = msgpack_object_map;
  using InputVarType = msgpack_object;

  /**
   * @brief Checks if type T has a custom constructor from a msgpack object.
   * @tparam T The type to check.
   */
  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_msgpack_obj(var); });

  /**
   * @brief Retrieves a field from a msgpack array by index.
   * @param _idx The index of the field to retrieve.
   * @param _arr The msgpack array.
   * @return The field at the given index or an error if out of bounds.
   */
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType _arr) const noexcept {
    if (_idx >= _arr.size) {
      return error("Index " + std::to_string(_idx) + " of of bounds.");
    }
    return _arr.ptr[_idx];
  }

  /**
   * @brief Retrieves a field from a msgpack object by name.
   * @param _name The name of the field to retrieve.
   * @param _obj The msgpack object.
   * @return The value of the field or an error if not found.
   */
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    for (uint32_t i = 0; i < _obj.size; ++i) {
      const auto& key = _obj.ptr[i].key;
      if (key.type != MSGPACK_OBJECT_STR) {
        return error("Key in element " + std::to_string(i) +
                     " was not a string.");
      }
      const auto current_name =
          std::string_view(key.via.str.ptr, key.via.str.size);
      if (_name == current_name) {
        return _obj.ptr[i].val;
      }
    }
    return error("No field named '" + _name + "' was found.");
  }

  /**
   * @brief Checks if a msgpack object is empty (nil).
   * @param _var The msgpack object to check.
   * @return True if the object is nil, false otherwise.
   */
  bool is_empty(const InputVarType& _var) const noexcept {
    return _var.type == MSGPACK_OBJECT_NIL;
  }

  /**
   * @brief Converts a msgpack object to a basic C++ type.
   * Basic types include strings, byte containers, booleans,
   * floating-point numbers, integers, and literal types (enums).
   * @tparam T The target type.
   * @param _var The msgpack object to convert.
   * @return The converted value or an error if conversion fails.
   */
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    const auto type = _var.type;
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      if (type != MSGPACK_OBJECT_STR) {
        return error("Could not cast to string.");
      }
      const auto str = _var.via.str;
      return std::string(str.ptr, str.size);

    } else if constexpr (concepts::MutableContiguousByteContainer<
                             std::remove_cvref_t<T>>) {
      using VectorType = std::remove_cvref_t<T>;
      using ValueType = typename VectorType::value_type;
      if (type != MSGPACK_OBJECT_BIN) {
        return error("Could not cast to bytestring.");
      }
      const auto bin = _var.via.bin;
      const auto data = internal::ptr_cast<const ValueType*>(bin.ptr);
      return VectorType(data, data + bin.size);

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (type != MSGPACK_OBJECT_BOOLEAN) {
        return error("Could not cast to boolean.");
      }
      return _var.via.boolean;

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (type == MSGPACK_OBJECT_FLOAT32 || type == MSGPACK_OBJECT_FLOAT64 ||
          type == MSGPACK_OBJECT_FLOAT) {
        return static_cast<T>(_var.via.f64);
      }
      return error(
          "Could not cast to numeric value. The type must be float "
          "or double.");

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if (type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
        return static_cast<T>(_var.via.u64);
      } else if (type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
        return static_cast<T>(_var.via.i64);
      }
      return error(
          "Could not cast to numeric value. The type must be integral, float "
          "or double.");
    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  /**
   * @brief Converts a msgpack object to an array type.
   * @param _var The msgpack object to convert.
   * @return The array or an error if conversion fails.
   */
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (_var.type != MSGPACK_OBJECT_ARRAY) {
      return error("Could not cast to an array.");
    }
    return _var.via.array;
  }

  /**
   * @brief Converts a msgpack object to a map/object type.
   * @param _var The msgpack object to convert.
   * @return The object or an error if conversion fails.
   */
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (_var.type != MSGPACK_OBJECT_MAP) {
      return error("Could not cast to a map.");
    }
    return _var.via.map;
  }

  /**
   * @brief Reads all elements of a msgpack array using a provided reader.
   * @tparam ArrayReader The type of the array reader.
   * @param _array_reader The reader to use for each element.
   * @param _arr The msgpack array to read.
   * @return An optional error if any element fails to be read.
   */
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    if constexpr (requires { _array_reader.reserve(_arr.size); }) {
      if (const auto err = _array_reader.reserve(_arr.size)) {
        return err;
      }
    }
    for (uint32_t i = 0; i < _arr.size; ++i) {
      const auto err = _array_reader.read(_arr.ptr[i]);
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads native map entries without converting keys to field names.
  template <class MapReader>
  std::optional<Error> read_typed_map(
      const MapReader& _reader, const InputObjectType& _map) const noexcept {
    for (uint32_t i = 0; i < _map.size; ++i) {
      if (const auto err = _reader.read(_map.ptr[i].key, _map.ptr[i].val)) {
        return err;
      }
    }
    return std::nullopt;
  }

  /**
   * @brief Reads all fields of a msgpack object using a provided reader.
   * @tparam ObjectReader The type of the object reader.
   * @param _object_reader The reader to use for each field.
   * @param _obj The msgpack object to read.
   * @return An optional error if any field fails to be read.
   */
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (uint32_t i = 0; i < _obj.size; ++i) {
      const auto& key = _obj.ptr[i].key;
      const auto& val = _obj.ptr[i].val;
      if (key.type != MSGPACK_OBJECT_STR) {
        return rfl::Error("Key in element " + std::to_string(i) +
                          " was not a string.");
      }
      const auto name = std::string_view(key.via.str.ptr, key.via.str.size);
      _object_reader.read(name, val);
    }
    return std::nullopt;
  }

  /**
   * @brief Uses a custom constructor to convert a msgpack object to type T.
   * @tparam T The target type.
   * @param _var The msgpack object to convert.
   * @return The constructed object or an error if construction fails.
   */
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_msgpack_obj(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

}  // namespace msgpack
}  // namespace rfl

#endif
