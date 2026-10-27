#ifndef RFL_CBOR_WRITER_HPP_
#define RFL_CBOR_WRITER_HPP_

// Silence a -Wmaybe-uninitialized false positive in jsoncons
// (utility/bigint.hpp).
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#endif
#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor_encoder.hpp>
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
#endif
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../common.hpp"
#include "../internal/is_bitset.hpp"

namespace rfl::cbor {

/// Writer class for serializing C++ objects to CBOR (Concise Binary Object
/// Representation) format. This class provides the interface for converting C++
/// objects into compact binary CBOR data. CBOR is a binary data serialization
/// format designed to be smaller and faster than JSON.
class RFL_API Writer {
  using Encoder = jsoncons::cbor::cbor_bytes_encoder;

 public:
  /// Represents a CBOR array being constructed during serialization.
  /// This is a placeholder type as the actual array data is managed by the
  /// encoder.
  struct CBOROutputArray {};

  /// Represents a CBOR object being constructed during serialization.
  /// This is a placeholder type as the actual object data is managed by the
  /// encoder.
  struct CBOROutputObject {};

  /// Represents a CBOR value being constructed during serialization.
  /// This is a placeholder type as the actual value data is managed by the
  /// encoder.
  struct CBOROutputVar {};

  using OutputArrayType = CBOROutputArray;
  using OutputObjectType = CBOROutputObject;
  using OutputVarType = CBOROutputVar;

  /// Constructs a CBOR Writer with the specified encoder.
  /// @param _encoder Pointer to the CBOR encoder that will write the binary
  /// output
  Writer(Encoder* _encoder);

  ~Writer();

  /// Creates a CBOR array as the root element of the output.
  /// @param _size The expected size of the array (may be used for optimization)
  /// @return An output array that can be populated with elements
  OutputArrayType array_as_root(const size_t _size) const;

  /// Creates a CBOR object as the root element of the output.
  /// @param The expected number of fields (unused, reserved for future
  /// optimization)
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType object_as_root(const size_t) const;

  /// Creates a null value as the root element of the output.
  /// @return An output variable representing null
  OutputVarType null_as_root() const;

  /// Creates a value as the root element of the output.
  /// Supports basic types like strings, numbers, booleans, and byte strings.
  /// @tparam T The type of the value to serialize
  /// @param _var The value to write as the root element
  /// @return An output variable representing the serialized value
  template <class T>
  OutputVarType value_as_root(const T& _var) const {
    return new_value(_var);
  }

  /// Adds a nested array to a parent array.
  /// @param _size The expected size of the nested array
  /// @param _parent Pointer to the parent array to add to
  /// @return An output array that can be populated with elements
  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const;

  /// Adds a nested array to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param _size The expected size of the nested array
  /// @param _parent Pointer to the parent object to add to
  /// @return An output array that can be populated with elements
  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t _size,
                                      OutputObjectType* _parent) const;

  /// Adds a nested object to a parent array.
  /// @param The expected number of fields (unused, reserved for future
  /// optimization)
  /// @param _parent Pointer to the parent array to add to
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType add_object_to_array(const size_t,
                                       OutputArrayType* _parent) const;

  /// Adds a nested object to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param The expected number of fields (unused, reserved for future
  /// optimization)
  /// @param _parent Pointer to the parent object to add to
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t,
                                        OutputObjectType* _parent) const;

  /// Adds a value to a parent array.
  /// Supports basic types like strings, numbers, booleans, and byte strings.
  /// @tparam T The type of the value to add
  /// @param _var The value to add to the array
  /// @param _parent Pointer to the parent array (unused as encoder tracks
  /// context)
  /// @return An output variable representing the added value
  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* /*_parent*/) const {
    return new_value(_var);
  }

  /// Adds a value to a parent object with the specified field name.
  /// Supports basic types like strings, numbers, booleans, and byte strings.
  /// @tparam T The type of the value to add
  /// @param _name The name of the field in the parent object
  /// @param _var The value to add to the object
  /// @param _parent Pointer to the parent object (unused as encoder tracks
  /// context)
  /// @return An output variable representing the added value
  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* /*_parent*/) const {
    encoder_->key(_name);
    return new_value(_var);
  }

  /// Adds a null value to a parent array.
  /// @param _parent Pointer to the parent array to add to
  /// @return An output variable representing the null value
  OutputVarType add_null_to_array(OutputArrayType* _parent) const;

  /// Adds a null value to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param _parent Pointer to the parent object to add to
  /// @return An output variable representing the null value
  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const;

  /// Finalizes a CBOR array after all elements have been added.
  /// @param _arr Pointer to the array to finalize
  void end_array(OutputArrayType* _arr) const;

  /// Finalizes a CBOR object after all fields have been added.
  /// @param _obj Pointer to the object to finalize
  void end_object(OutputObjectType* _obj) const;

 private:
  OutputArrayType new_array(const size_t _size) const;

  OutputObjectType new_object() const;

  template <class T>
  OutputVarType new_value(const T& _var) const {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      encoder_->string_value(_var);

    } else if constexpr (internal::is_bitset_v<T>) {
      // Native jsoncons conversion retains the CBOR base16 semantic tag.
      jsoncons::json(_var).dump(*encoder_);

    } else if constexpr (std::is_same<std::remove_cvref_t<T>,
                                      rfl::Bytestring>() ||
                         std::is_same<std::remove_cvref_t<T>,
                                      rfl::Vectorstring>()) {
      encoder_->byte_string_value(_var);

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      encoder_->bool_value(_var);

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      encoder_->double_value(static_cast<double>(_var));

    } else if constexpr (std::is_unsigned<std::remove_cvref_t<T>>()) {
      encoder_->uint64_value(static_cast<std::uint64_t>(_var));

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      encoder_->int64_value(static_cast<std::int64_t>(_var));

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
    return OutputVarType{};
  }

 private:
  /// The underlying TinyCBOR encoder.
  Encoder* const encoder_;
};
}  // namespace rfl::cbor

#endif
