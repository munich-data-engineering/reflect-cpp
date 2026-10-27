#ifndef RFL_AVRO_READER_HPP_
#define RFL_AVRO_READER_HPP_

#include <avro.h>

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../concepts.hpp"
#include "../internal/is_literal.hpp"
#include "../parsing/schemaful/IsSchemafulReader.hpp"

namespace rfl::avro {

struct Reader {
  struct AVROInputArray {
    const avro_value_t* val_;
  };

  struct AVROInputObject {
    const avro_value_t* val_;
  };

  struct AVROInputMap {
    const avro_value_t* val_;
  };

  struct AVROInputUnion {
    const avro_value_t* val_;
  };

  struct AVROInputVar {
    const avro_value_t* val_;
  };

  using InputArrayType = AVROInputArray;
  using InputObjectType = AVROInputObject;
  using InputMapType = AVROInputMap;
  using InputUnionType = AVROInputUnion;
  using InputVarType = AVROInputVar;

  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_avro_obj(var); });

  /// Checks if the given Avro value is empty (null type).
  /// @param _var The Avro value to check
  /// @return true if the value is of type AVRO_NULL, false otherwise
  bool is_empty(const InputVarType& _var) const noexcept {
    return avro_value_get_type(_var.val_) == AVRO_NULL;
  }

  /// Converts an Avro value to a basic C++ type.
  /// Supports strings, byte containers, booleans, floating-point numbers, 
  /// integers, and literal types (enums).
  /// @tparam T The target C++ type to convert to
  /// @param _var The Avro value to convert
  /// @return A Result containing the converted value or an error
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    const auto type = avro_value_get_type(_var.val_);
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      const char* c_str = nullptr;
      size_t size = 0;
      const auto err = avro_value_get_string(_var.val_, &c_str, &size);
      if (err) {
        return error("Could not cast to string.");
      }
      if (size == 0) {
        return std::string("");
      }
      return std::string(c_str, size - 1);
    } else if constexpr (concepts::MutableContiguousByteContainer<
                             std::remove_cvref_t<T>>) {
      using VectorType = std::remove_cvref_t<T>;
      using ValueType = typename VectorType::value_type;
      const void* ptr = nullptr;
      size_t size = 0;
      const auto err = avro_value_get_bytes(_var.val_, &ptr, &size);
      if (err) {
        return error("Could not cast to bytestring.");
      }
      if (size == 0) {
        return VectorType{};
      }
      const auto data = internal::ptr_cast<const ValueType*>(ptr);
      return VectorType(data, data + size);
    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (type != AVRO_BOOLEAN) {
        return rfl::error("Could not cast to boolean.");
      }
      int result_value = 0;
      int result = avro_value_get_boolean(_var.val_, &result_value);
      if (result != 0) {
        return error(std::string(__FUNCTION__) + " error(" +
                     std::to_string(result) + "): " + avro_strerror());
      }
      return (result_value != 0);

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (type == AVRO_DOUBLE) {
        double result = 0.0;
        const auto err = avro_value_get_double(_var.val_, &result);
        if (err) {
          return error("Could not cast to double.");
        }
        return static_cast<T>(result);
      } else if (type == AVRO_FLOAT) {
        float result = 0.0;
        const auto err = avro_value_get_float(_var.val_, &result);
        if (err) {
          return error("Could not cast to float.");
        }
        return static_cast<T>(result);
      } else {
        return rfl::error(
            "Could not cast to numeric value. The type must be float "
            "or double.");
      }

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if (type == AVRO_INT32) {
        int32_t result = 0;
        const auto err = avro_value_get_int(_var.val_, &result);
        if (err) {
          return error("Could not cast to int32.");
        }
        return static_cast<T>(result);
      } else if (type == AVRO_INT64) {
        int64_t result = 0;
        const auto err = avro_value_get_long(_var.val_, &result);
        if (err) {
          return error("Could not cast to int64.");
        }
        return static_cast<T>(result);
      } else {
        return rfl::error(
            "Could not cast to numeric value. The type must be integral.");
      }

    } else if constexpr (internal::is_literal_v<T>) {
      int value = 0;
      const auto err = avro_value_get_enum(_var.val_, &value);
      if (err) {
        return error("Could not cast to enum.");
      }
      return std::remove_cvref_t<T>::from_value(
          static_cast<typename std::remove_cvref_t<T>::ValueType>(value));

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  /// Converts an Avro value to an array type.
  /// @param _var The Avro value to convert
  /// @return A Result containing an InputArrayType or an error if the value is not an array
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (avro_value_get_type(_var.val_) != AVRO_ARRAY) {
      return error("Could not cast to an array.");
    }
    return InputArrayType{_var.val_};
  }

  /// Converts an Avro value to an object (record) type.
  /// In Avro, objects are represented as records with named fields.
  /// @param _var The Avro value to convert
  /// @return A Result containing an InputObjectType or an error if the value is not a record
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    const auto type = avro_value_get_type(_var.val_);
    if (type != AVRO_RECORD) {
      return error("Could not cast to an object.");
    }
    return InputObjectType{_var.val_};
  }

  /// Converts an Avro value to a map type.
  /// Avro maps are key-value containers where keys are strings.
  /// @param _var The Avro value to convert
  /// @return A Result containing an InputMapType or an error if the value is not a map
  rfl::Result<InputMapType> to_map(const InputVarType& _var) const noexcept {
    const auto type = avro_value_get_type(_var.val_);
    if (type != AVRO_MAP) {
      return error("Could not cast to a map.");
    }
    return InputMapType{_var.val_};
  }

  /// Converts an Avro value to a union type.
  /// Avro unions allow a value to be one of several types.
  /// @param _var The Avro value to convert
  /// @return A Result containing an InputUnionType or an error if the value is not a union
  rfl::Result<InputUnionType> to_union(
      const InputVarType& _var) const noexcept {
    const auto type = avro_value_get_type(_var.val_);
    if (type != AVRO_UNION) {
      return error("Could not cast to a union.");
    }
    return InputUnionType{_var.val_};
  }

  /// Reads all elements from an Avro array using the provided array reader.
  /// @tparam ArrayReader The type of reader that processes individual array elements
  /// @param _array_reader The reader object that processes each element
  /// @param _arr The Avro array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    size_t size = 0;
    int res = avro_value_get_size(_arr.val_, &size);
    if (res) {
      return Error(std::string(__FUNCTION__) + " error(" + std::to_string(res) +
                   "): " + avro_strerror());
    }
    if constexpr (requires { _array_reader.reserve(size); }) {
      if (const auto err = _array_reader.reserve(size)) {
        return err;
      }
    }
    for (size_t ix = 0; ix < size; ++ix) {
      avro_value_t element;
      res = avro_value_get_by_index(_arr.val_, ix, &element, nullptr);
      if (res) {
        return Error(std::string(__FUNCTION__) + " error(" +
                     std::to_string(res) + "): " + avro_strerror());
      }
      const auto err = _array_reader.read(InputVarType{&element});
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads all key-value pairs from an Avro map using the provided map reader.
  /// @tparam MapReader The type of reader that processes individual map entries
  /// @param _map_reader The reader object that processes each key-value pair
  /// @param _map The Avro map to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class MapReader>
  std::optional<Error> read_map(const MapReader& _map_reader,
                                const InputMapType& _map) const noexcept {
    size_t size = 0;
    int res = avro_value_get_size(_map.val_, &size);
    if (res != 0) {
      return Error(std::string(__FUNCTION__) + " error(" + std::to_string(res) +
                   "): " + avro_strerror());
    }
    for (size_t ix = 0; ix < size; ++ix) {
      avro_value_t element;
      const char* key = nullptr;
      res = avro_value_get_by_index(_map.val_, ix, &element, &key);
      if (res != 0) {
        return Error(std::string(__FUNCTION__) + " error(" +
                     std::to_string(res) + "): " + avro_strerror());
      }
      _map_reader.read(std::string_view(key), InputVarType{&element});
    }
    return std::nullopt;
  }

  /// Reads all fields from an Avro record (object) using the provided object reader.
  /// Fields are accessed by their index in the schema definition.
  /// @tparam ObjectReader The type of reader that processes individual object fields
  /// @param _object_reader The reader object that processes each field
  /// @param _obj The Avro record to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    size_t size = 0;
    int res = avro_value_get_size(_obj.val_, &size);
    if (res != 0) {
      return Error(std::string(__FUNCTION__) + " error(" + std::to_string(res) +
                   "): " + avro_strerror());
    }
    if constexpr (requires {
                    std::make_integer_sequence<int, ObjectReader::size()>{};
                  }) {
      if (size == ObjectReader::size()) {
        std::optional<Error> err;
        const auto read_field =
            [&]<int i>(std::integral_constant<int, i> index) {
              if (err) {
                return;
              }
              avro_value_t element;
              const int result =
                  avro_value_get_by_index(_obj.val_, i, &element, nullptr);
              if (result != 0) {
                err = Error(std::string("read_object error(") +
                            std::to_string(result) + "): " + avro_strerror());
                return;
              }
              _object_reader.read(index, InputVarType{&element});
            };
        [&]<int... Is>(std::integer_sequence<int, Is...>) {
          (read_field(std::integral_constant<int, Is>{}), ...);
        }(std::make_integer_sequence<int, ObjectReader::size()>{});
        return err;
      }
    }
    for (size_t ix = 0; ix < size; ++ix) {
      avro_value_t element;
      res = avro_value_get_by_index(_obj.val_, ix, &element, nullptr);
      if (res != 0) {
        return Error(std::string(__FUNCTION__) + " error(" +
                     std::to_string(res) + "): " + avro_strerror());
      }
      _object_reader.read(static_cast<int>(ix), InputVarType{&element});
    }
    return std::nullopt;
  }

  /// Reads an Avro union value and converts it to the appropriate variant type.
  /// Determines which branch of the union is active and reads that value.
  /// @tparam VariantType The C++ variant type to construct
  /// @tparam UnionReaderType The type of reader that handles union alternatives
  /// @param _union The Avro union to read from
  /// @return A Result containing the variant or an error
  template <class VariantType, class UnionReaderType>
  rfl::Result<VariantType> read_union(
      const InputUnionType& _union) const noexcept {
    int disc = 0;
    auto err = avro_value_get_discriminant(_union.val_, &disc);
    if (err) {
      return error("Could not get the discriminant.");
    }
    avro_value_t value;
    err = avro_value_get_current_branch(_union.val_, &value);
    if (err) {
      return error("Could not cast the union type.");
    }
    return UnionReaderType::read(*this, static_cast<size_t>(disc),
                                 InputVarType{&value});
  }

  /// Uses a custom constructor to create an object from an Avro value.
  /// The type T must provide a static method T::from_avro_obj(InputVarType).
  /// @tparam T The type to construct
  /// @param _var The Avro value to construct from
  /// @return A Result containing the constructed object or an error
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_avro_obj(_var);
    } catch (std::exception& e) {
      return rfl::error(e.what());
    }
  }
};

static_assert(parsing::schemaful::IsSchemafulReader<Reader>,
              "This must be a schemaful reader.");

}  // namespace rfl::avro

#endif
