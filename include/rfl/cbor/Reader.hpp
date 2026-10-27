#ifndef RFL_CBOR_READER_HPP_
#define RFL_CBOR_READER_HPP_

#include <cstddef>
#include <exception>
// Silence a -Wmaybe-uninitialized false positive in jsoncons
// (utility/bigint.hpp).
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#endif
#include <jsoncons/json.hpp>
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
#endif
#include <string>
#include <type_traits>
#include <vector>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/is_bitset.hpp"
#include "../internal/ptr_cast.hpp"

namespace rfl::cbor {

/// Reader class for deserializing CBOR (Concise Binary Object Representation)
/// data. This class provides the interface for parsing CBOR format into C++
/// objects. CBOR is a binary data serialization format that is more compact
/// than JSON while maintaining similar data model capabilities.
class Reader {
 public:
  /// Represents a CBOR array during deserialization.
  /// Wraps a pointer to the underlying jsoncons JSON value representing the
  /// array.
  struct CBORInputArray {
    jsoncons::json* val_;
  };

  /// Represents a CBOR object during deserialization.
  /// Wraps a pointer to the underlying jsoncons JSON value representing the
  /// object.
  struct CBORInputObject {
    jsoncons::json* val_;
  };

  /// Represents a variant CBOR value during deserialization.
  /// Can hold any CBOR type (array, object, primitive, etc.).
  /// Wraps a pointer to the underlying jsoncons JSON value.
  struct CBORInputVar {
    jsoncons::json* val_;
  };

  using InputArrayType = CBORInputArray;
  using InputObjectType = CBORInputObject;
  using InputVarType = CBORInputVar;

  /// Constructs a new CBOR Reader.
  Reader() {}

  ~Reader() = default;

  /// Compile-time flag indicating whether type T has a custom constructor from
  /// CBOR. If true, the type provides a static from_cbor_obj() method for
  /// custom deserialization.
  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_cbor_obj(var); });

  /// Retrieves a field from a CBOR array by index.
  /// @param _idx The zero-based index of the element to retrieve
  /// @param _arr The CBOR array to read from
  /// @return Result containing the field value, or an error if index is out of
  /// bounds
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.val_->size()) {
      return error("Index out of range.");
    }
    return InputVarType{&_arr.val_->at(_idx)};
  }

  /// Retrieves a field from a CBOR object by name.
  /// @param _name The name of the field to retrieve
  /// @param _obj The CBOR object to read from
  /// @return Result containing the field value, or an error if field is not
  /// found
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    for (auto& kv : _obj.val_->object_range()) {
      if (kv.key() == _name) {
        return InputVarType{&kv.value()};
      };
    }
    return error("Field name '" + _name + "' not found.");
  }

  /// Checks if a CBOR value is empty or null.
  /// @param _var The CBOR value to check
  /// @return true if the value is null, false otherwise
  bool is_empty(const InputVarType& _var) const noexcept {
    return _var.val_->is_null();
  }

  /// Converts a CBOR value to a basic C++ type.
  /// Supports strings, bytesstrings, booleans, floating-point numbers, and
  /// integers.
  /// @tparam T The target C++ type
  /// @param _var The CBOR value to convert
  /// @return Result containing the converted value, or an error if conversion
  /// fails
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      if (!_var.val_->is_string()) {
        return error("Could not cast to string.");
      }
      return _var.val_->as<std::string>();

    } else if constexpr (internal::is_bitset_v<T>) {
      try {
        return _var.val_->template as<T>();
      } catch (const std::exception& e) {
        return error(e.what());
      }

    } else if constexpr (std::is_same<std::remove_cvref_t<T>,
                                      rfl::Bytestring>() ||
                         std::is_same<std::remove_cvref_t<T>,
                                      rfl::Vectorstring>()) {
      using VectorType = std::remove_cvref_t<T>;
      using ValueType = typename VectorType::value_type;
      if (!_var.val_->is_byte_string()) {
        if constexpr (std::is_same<std::remove_cvref_t<T>, rfl::Bytestring>()) {
          return error("Could not cast to bytestring.");
        } else {
          return error("Could not cast to vectorstring.");
        }
      }
      const auto vec = _var.val_->as<std::vector<uint8_t>>();
      const auto data = internal::ptr_cast<const ValueType*>(vec.data());
      return VectorType(data, data + vec.size());

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (!_var.val_->is_bool()) {
        return error("Could not cast to boolean.");
      }
      return _var.val_->as<bool>();

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (_var.val_->is_double()) {
        return static_cast<T>(_var.val_->as<double>());
      }
      return error("Could not cast to double.");

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if (_var.val_->is_int64()) {
        return static_cast<T>(_var.val_->as<int64_t>());
      }
      if (_var.val_->is_uint64()) {
        return static_cast<T>(_var.val_->as<uint64_t>());
      }
      return error("Could not cast to integer.");

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  /// Converts a CBOR value to an array.
  /// @param _var The CBOR value to convert
  /// @return Result containing the array, or an error if value is not an array
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!_var.val_->is_array()) {
      return error("Could not cast to an array.");
    }
    return InputArrayType{_var.val_};
  }

  /// Converts a CBOR value to an object.
  /// @param _var The CBOR value to convert
  /// @return Result containing the object, or an error if value is not an
  /// object
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.val_->is_object()) {
      return error("Could not cast to an object.");
    }
    return InputObjectType{_var.val_};
  }

  /// Reads all elements from a CBOR array using a provided array reader.
  /// The array reader's read() method is called for each element in the array.
  /// @tparam ArrayReader Type that provides a read() method for processing
  /// elements
  /// @param _array_reader The reader object used to process each array element
  /// @param _arr The CBOR array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    if constexpr (requires { _array_reader.reserve(_arr.val_->size()); }) {
      if (const auto err = _array_reader.reserve(_arr.val_->size())) {
        return err;
      }
    }
    for (auto& val : _arr.val_->array_range()) {
      const auto err = _array_reader.read(InputVarType{&val});
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads all key-value pairs from a CBOR object using a provided object
  /// reader. The object reader's read() method is called for each key-value
  /// pair.
  /// @tparam ObjectReader Type that provides a read() method for processing
  /// key-value pairs
  /// @param _object_reader The reader object used to process each field
  /// @param _obj The CBOR object to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (auto& kv : _obj.val_->object_range()) {
      _object_reader.read(kv.key(), InputVarType{&kv.value()});
    }
    return std::nullopt;
  }

  /// Uses a type's custom constructor to deserialize from CBOR.
  /// Calls the type's static from_cbor_obj() method for custom deserialization
  /// logic.
  /// @tparam T The type to construct, must have a from_cbor_obj() static method
  /// @param _var The CBOR value to deserialize from
  /// @return Result containing the constructed object, or an error if
  /// construction fails
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_cbor_obj(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

}  // namespace rfl::cbor

#endif
