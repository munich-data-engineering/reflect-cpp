#ifndef RFL_CEREAL_READER_HPP_
#define RFL_CEREAL_READER_HPP_

#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <exception>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/is_literal.hpp"
#include "../parsing/schemaful/IsSchemafulReader.hpp"

namespace rfl::cereal {

/// Reader for Cereal serialization library integration.
/// Cereal is a C++11 serialization library that supports multiple archive formats.
/// This reader uses Cereal's portable binary archive format for cross-platform compatibility.
struct Reader {
  using CerealArchive = ::cereal::PortableBinaryInputArchive;

  struct CerealInputVar {
    CerealArchive* archive_;
  };

  struct InputVarType {
    CerealArchive* archive_;
  };

  struct InputArrayType {
    CerealArchive* archive_;
  };

  struct InputObjectType {
    CerealArchive* archive_;
  };

  struct InputMapType {
    CerealArchive* archive_;
  };

  struct InputUnionType {
    CerealArchive* archive_;
  };

  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_cereal_obj(var); });

  /// Checks if the Cereal value is empty (null).
  /// @param _var The Cereal archive variable to check
  /// @return true if empty/null, false otherwise
  bool is_empty(const InputVarType& _var) const noexcept;

  /// Converts a value from the Cereal archive to a basic C++ type.
  /// Supports strings, literals, byte containers, and all standard types Cereal can serialize.
  /// @tparam T The target C++ type to convert to
  /// @param _var The Cereal archive variable containing the value
  /// @return A Result containing the converted value or an error
  template <class T>
  RFL_ALWAYS_INLINE rfl::Result<T> to_basic_type(
      const InputVarType& _var) const noexcept {
    try {
      if constexpr (internal::is_literal_v<T>) {
        std::string str;
        (*_var.archive_)(str);
        return std::remove_cvref_t<T>::from_string(str);

      } else if constexpr (std::is_same<std::remove_cvref_t<T>,
                                        rfl::Bytestring>() ||
                           std::is_same<std::remove_cvref_t<T>,
                                        rfl::Vectorstring>()) {
        size_t size;
        (*_var.archive_)(::cereal::make_size_tag(size));
        std::remove_cvref_t<T> result(size);
        (*_var.archive_)(::cereal::binary_data(result.data(), size));
        return result;

      } else {
        T value;
        (*_var.archive_)(value);
        return value;
      }
    } catch (std::exception& e) {
      return error(std::string("Cereal read error: ") + e.what());
    }
  }

  /// Converts a Cereal archive value to an array type.
  /// Reads the size tag first, then prepares to read array elements.
  /// @param _var The Cereal archive variable
  /// @return A Result containing an InputArrayType or an error
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept;

  /// Converts a Cereal archive value to an object (struct) type.
  /// @param _var The Cereal archive variable
  /// @return A Result containing an InputObjectType or an error
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept;

  /// Converts a Cereal archive value to a map type.
  /// @param _var The Cereal archive variable
  /// @return A Result containing an InputMapType or an error
  rfl::Result<InputMapType> to_map(const InputVarType& _var) const noexcept;

  /// Converts a Cereal archive value to a union (variant) type.
  /// @param _var The Cereal archive variable
  /// @return A Result containing an InputUnionType or an error
  rfl::Result<InputUnionType> to_union(
      const InputVarType& _var) const noexcept;

  /// Reads all elements from a Cereal array using the provided array reader.
  /// Cereal stores the size as a size tag before the elements.
  /// @tparam ArrayReader The type of reader that processes individual array elements
  /// @param _array_reader The reader object that processes each element
  /// @param _arr The Cereal array structure
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    try {
      size_t size;
      (*_arr.archive_)(::cereal::make_size_tag(size));
      if constexpr (requires { _array_reader.reserve(size); }) {
        if (const auto err = _array_reader.reserve(size)) {
          return err;
        }
      }
      for (size_t i = 0; i < size; ++i) {
        const auto err = _array_reader.read(InputVarType{_arr.archive_});
        if (err) {
          return err;
        }
      }
      return std::nullopt;
    } catch (std::exception& e) {
      return Error(std::string("Cereal array read error: ") + e.what());
    }
  }

  /// Reads keys and values directly from the archive in sequence.
  template <class MapReader>
  std::optional<Error> read_typed_map(const MapReader& _reader,
                                      const InputMapType& _map) const noexcept {
    try {
      size_t size;
      (*_map.archive_)(::cereal::make_size_tag(size));
      for (size_t i = 0; i < size; ++i) {
        const auto var = InputVarType{_map.archive_};
        if (const auto err = _reader.read(var, var)) {
          return err;
        }
      }
      return std::nullopt;
    } catch (const std::exception& e) {
      return Error(e.what());
    }
  }

  /// Reads all fields from a Cereal object using the provided object reader.
  /// Fields are read in declaration order by index.
  /// @tparam ObjectReader The type of reader that processes individual object fields
  /// @param _object_reader The reader object that processes each field
  /// @param _obj The Cereal object structure
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  RFL_ALWAYS_INLINE std::optional<Error> read_object(
      const ObjectReader& _object_reader,
      const InputObjectType& _obj) const noexcept {
    try {
      [&]<int... Is>(std::integer_sequence<int, Is...>) {
        (_object_reader.read(std::integral_constant<int, Is>{},
                             InputVarType{_obj.archive_}), ...);
      }(std::make_integer_sequence<int, ObjectReader::size()>{});
      return std::nullopt;
    } catch (std::exception& e) {
      return Error(std::string("Cereal object read error: ") + e.what());
    }
  }

  /// Reads a union (variant) value and converts it to the appropriate variant type.
  /// Reads the discriminant index first to determine which alternative is active.
  /// @tparam T The C++ variant type to construct
  /// @tparam UnionReader The type of reader that handles union alternatives
  /// @param _union The Cereal union structure
  /// @return A Result containing the variant or an error
  template <class T, class UnionReader>
  rfl::Result<T> read_union(const InputUnionType& _union) const noexcept {
    try {
      size_t index;
      (*_union.archive_)(index);
      return UnionReader::read(*this, index, InputVarType{_union.archive_});
    } catch (std::exception& e) {
      return error(std::string("Cereal union read error: ") + e.what());
    }
  }

  /// Uses a custom constructor to create an object from a Cereal archive value.
  /// The type T must provide a static method T::from_cereal_obj(InputVarType).
  /// @tparam T The type to construct
  /// @param _var The Cereal archive variable to construct from
  /// @return A Result containing the constructed object or an error
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_cereal_obj(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

static_assert(parsing::schemaful::IsSchemafulReader<Reader>);

}  // namespace rfl::cereal

#endif
