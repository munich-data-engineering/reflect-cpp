#ifndef RFL_YAS_READER_HPP_
#define RFL_YAS_READER_HPP_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <yas/binary_iarchive.hpp>
#include <yas/mem_streams.hpp>
#include <yas/std_types.hpp>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../internal/is_literal.hpp"
#include "../parsing/read_array_elements.hpp"
#include "../parsing/schemaful/IsSchemafulReader.hpp"

namespace rfl::yas {

struct Reader {
  using IArchive = ::yas::binary_iarchive<::yas::mem_istream>;

  struct InputVarType {
    IArchive* ar;
  };

  struct InputArrayType {
    IArchive* ar;
  };

  struct InputObjectType {
    IArchive* ar;
  };

  struct InputMapType {
    IArchive* ar;
  };

  struct InputUnionType {
    IArchive* ar;
  };

  /// @brief Indicates whether a custom constructor exists for type T.
  /// @tparam T The type to check for a custom constructor.
  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_yas_obj(var); });

  /// @brief Checks if the given input variable is empty.
  /// @param _var The input variable to check.
  /// @return Always returns false, as emptiness is not supported in this
  /// context.
  bool is_empty(const InputVarType& /*_var*/) const noexcept { return false; }

  /// @brief Deserializes a basic type from the input variable using the YAS
  /// archive.
  /// @tparam T The type to deserialize.
  /// @param _var The input variable containing the archive pointer.
  /// @return The deserialized value or an error if deserialization fails.
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    try {
      if constexpr (internal::is_literal_v<T>) {
        std::string str;
        (*_var.ar) & str;
        return std::remove_cvref_t<T>::from_string(str);
      } else {
        T value;
        (*_var.ar) & value;
        return value;
      }
    } catch (std::exception& e) {
      return error(std::string("yas read error: ") + e.what());
    }
  }

  /// @brief Converts an input variable to an array type for further
  /// deserialization.
  /// @param _var The input variable containing the archive pointer.
  /// @return An InputArrayType wrapping the archive pointer.
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    return InputArrayType{_var.ar};
  }

  /// @brief Converts an input variable to an object type for further
  /// deserialization.
  /// @param _var The input variable containing the archive pointer.
  /// @return An InputObjectType wrapping the archive pointer.
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    return InputObjectType{_var.ar};
  }

  /// @brief Converts an input variable to a map type for further
  /// deserialization.
  /// @param _var The input variable containing the archive pointer.
  /// @return An InputMapType wrapping the archive pointer.
  rfl::Result<InputMapType> to_map(const InputVarType& _var) const noexcept {
    return InputMapType{_var.ar};
  }

  /// @brief Converts an input variable to a union type for further
  /// deserialization.
  /// @param _var The input variable containing the archive pointer.
  /// @return An InputUnionType wrapping the archive pointer.
  rfl::Result<InputUnionType> to_union(
      const InputVarType& _var) const noexcept {
    return InputUnionType{_var.ar};
  }

  /// @brief Reads an array from the archive using the provided ArrayReader.
  /// @tparam ArrayReader The type responsible for reading array elements.
  /// @param _array_reader The reader instance for array elements.
  /// @param _arr The input array type containing the archive pointer.
  /// @return std::nullopt on success, or an Error if reading fails.
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    try {
      size_t size;
      (*_arr.ar) & size;
      return parsing::read_array_elements(_array_reader, InputVarType{_arr.ar},
                                          size);
    } catch (std::exception& e) {
      return Error(std::string("yas array read error: ") + e.what());
    }
  }

  /// Reads exactly the compile-time extent without consuming a length prefix.
  template <size_t _size, class ArrayReader>
  std::optional<Error> read_fixed_array(
      const ArrayReader& _array_reader,
      const InputVarType& _var) const noexcept {
    try {
      return parsing::read_array_elements(_array_reader, _var, _size);
    } catch (std::exception& e) {
      return Error(std::string("yas array read error: ") + e.what());
    }
  }

  /// @brief Reads a map from the archive using the provided MapReader.
  /// @tparam MapReader The type responsible for reading map entries.
  /// @param _map_reader The reader instance for map entries.
  /// @param _map The input map type containing the archive pointer.
  /// @return std::nullopt on success, or an Error if reading fails.
  template <class MapReader>
  std::optional<Error> read_map(const MapReader& _map_reader,
                                const InputMapType& _map) const noexcept {
    try {
      std::size_t size;
      (*_map.ar) & size;
      for (std::size_t i = 0; i < size; ++i) {
        std::string key;
        (*_map.ar) & key;
        _map_reader.read(std::string_view(key), InputVarType{_map.ar});
      }
      return std::nullopt;
    } catch (std::exception& e) {
      return Error(std::string("yas map read error: ") + e.what());
    }
  }

  /// @brief Reads an object from the archive using the provided ObjectReader.
  /// @tparam ObjectReader The type responsible for reading object fields.
  /// @param _object_reader The reader instance for object fields.
  /// @param _obj The input object type containing the archive pointer.
  /// @return std::nullopt on success, or an Error if reading fails.
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    try {
      [&]<int... Is>(std::integer_sequence<int, Is...>) {
        (_object_reader.read(std::integral_constant<int, Is>{},
                             InputVarType{_obj.ar}),
         ...);
      }(std::make_integer_sequence<int, ObjectReader::size()>{});
      return std::nullopt;
    } catch (std::exception& e) {
      return Error(std::string("yas object read error: ") + e.what());
    }
  }

  /// @brief Reads a union from the archive using the provided UnionReader.
  /// @tparam T The type to deserialize.
  /// @tparam UnionReader The type responsible for reading union alternatives.
  /// @param _union The input union type containing the archive pointer.
  /// @return The deserialized union value or an error if reading fails.
  template <class T, class UnionReader>
  rfl::Result<T> read_union(const InputUnionType& _union) const noexcept {
    try {
      std::size_t index;
      (*_union.ar) & index;
      return UnionReader::read(*this, index, InputVarType{_union.ar});
    } catch (std::exception& e) {
      return error(std::string("yas union read error: ") + e.what());
    }
  }

  /// @brief Returns an error indicating that custom constructors are not
  /// supported.
  /// @tparam T The type for which a custom constructor would be used.
  /// @param _var The input variable (unused).
  /// @return An error indicating custom constructors are not supported.
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& /*_var*/) const noexcept {
    return rfl::error("Custom constructors are not supported.");
  }
};

static_assert(parsing::schemaful::IsSchemafulReader<Reader>);

}  // namespace rfl::yas

#endif
