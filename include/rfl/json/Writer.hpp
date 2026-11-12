#ifndef RFL_JSON_WRITER_HPP_
#define RFL_JSON_WRITER_HPP_

#if __has_include(<yyjson.h>)
#include <yyjson.h>
#else
#include "../thirdparty/yyjson.h"
#endif

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../always_false.hpp"
#include "../common.hpp"
#include "../internal/base64.hpp"

namespace rfl {
namespace json {

/// Writer class for serializing C++ objects to JSON (JavaScript Object Notation) format.
/// This class provides the interface for converting C++ objects into JSON text.
/// Uses the yyjson library for high-performance JSON generation.
class RFL_API Writer {
 public:
  /// Represents a JSON array being constructed during serialization.
  /// Wraps a pointer to the underlying yyjson mutable value representing the array.
  struct YYJSONOutputArray {
    YYJSONOutputArray(yyjson_mut_val* _val) : val_(_val) {}
    yyjson_mut_val* val_;
  };

  /// Represents a JSON object being constructed during serialization.
  /// Wraps a pointer to the underlying yyjson mutable value representing the object.
  struct YYJSONOutputObject {
    YYJSONOutputObject(yyjson_mut_val* _val) : val_(_val) {}
    yyjson_mut_val* val_;
  };

  /// Represents a JSON value being constructed during serialization.
  /// Can be constructed from an array, object, or primitive value.
  /// Wraps a pointer to the underlying yyjson mutable value.
  struct YYJSONOutputVar {
    YYJSONOutputVar(yyjson_mut_val* _val) : val_(_val) {}

    YYJSONOutputVar(YYJSONOutputArray _arr) : val_(_arr.val_) {}

    YYJSONOutputVar(YYJSONOutputObject _obj) : val_(_obj.val_) {}

    yyjson_mut_val* val_;
  };

  using OutputArrayType = YYJSONOutputArray;
  using OutputObjectType = YYJSONOutputObject;
  using OutputVarType = YYJSONOutputVar;

  /// Constructs a new JSON Writer, initializing a new mutable document.
  Writer();

  ~Writer() = default;

  /// Returns the underlying yyjson mutable document.
  /// Used internally for document management.
  /// @return Pointer to the yyjson mutable document
  yyjson_mut_doc* doc() const { return doc_.get(); }

  /// Creates a JSON array as the root element of the output.
  /// @param The expected size (unused, reserved for future optimization)
  /// @return An output array that can be populated with elements
  OutputArrayType array_as_root(const size_t) const noexcept;

  /// Creates a JSON object as the root element of the output.
  /// @param The expected size (unused, reserved for future optimization)
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType object_as_root(const size_t) const noexcept;

  /// Creates a null value as the root element of the output.
  /// @return An output variable representing null
  OutputVarType null_as_root() const noexcept;

  /// Creates a value as the root element of the output.
  /// Supports basic types like strings, numbers, and booleans.
  /// @tparam T The type of the value to serialize
  /// @param _var The value to write as the root element
  /// @return An output variable representing the serialized value
  template <class T>
  OutputVarType value_as_root(const T& _var) const noexcept {
    const auto val = from_basic_type(_var);
    yyjson_mut_doc_set_root(doc(), val.val_);
    return OutputVarType(val);
  }

  /// Adds a nested array to a parent array.
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent array to add to
  /// @return An output array that can be populated with elements
  OutputArrayType add_array_to_array(const size_t,
                                     OutputArrayType* _parent) const;

  /// Adds a nested array to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent object to add to
  /// @return An output array that can be populated with elements
  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t,
                                      OutputObjectType* _parent) const;

  /// Adds a nested object to a parent array.
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent array to add to
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType add_object_to_array(const size_t,
                                       OutputArrayType* _parent) const;

  /// Adds a nested object to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent object to add to
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t,
                                        OutputObjectType* _parent) const;

  /// Adds a value to a parent array.
  /// Supports basic types like strings, numbers, and booleans.
  /// @tparam T The type of the value to add
  /// @param _var The value to add to the array
  /// @param _parent Pointer to the parent array to add to
  /// @return An output variable representing the added value
  /// @throws std::runtime_error if the value cannot be added
  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const {
    const auto val = from_basic_type(_var);
    const bool ok = yyjson_mut_arr_add_val(_parent->val_, val.val_);
    if (!ok) {
      throw std::runtime_error("Could not add value to array.");
    }
    return OutputVarType(val);
  }

  /// Adds a value to a parent object with the specified field name.
  /// Supports basic types like strings, numbers, and booleans.
  /// @tparam T The type of the value to add
  /// @param _name The name of the field in the parent object
  /// @param _var The value to add to the object
  /// @param _parent Pointer to the parent object to add to
  /// @return An output variable representing the added value
  /// @throws std::runtime_error if the field cannot be added
  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* _parent) const {
    const auto val = from_basic_type(_var);
    const bool ok = yyjson_mut_obj_add(
        _parent->val_, yyjson_mut_strncpy(doc(), _name.data(), _name.size()), val.val_);
    if (!ok) {
      throw std::runtime_error("Could not add field '" + std::string(_name) +
                               "' to object.");
    }
    return OutputVarType(val);
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

  /// Finalizes a JSON array after all elements have been added.
  /// This is a no-op for yyjson as arrays don't need explicit finalization.
  /// @param Pointer to the array (unused)
  void end_array(OutputArrayType*) const noexcept;

  /// Finalizes a JSON object after all fields have been added.
  /// This is a no-op for yyjson as objects don't need explicit finalization.
  /// @param Pointer to the object (unused)
  void end_object(OutputObjectType*) const noexcept;

 private:
  template <class T>
  OutputVarType from_basic_type(const T& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      return OutputVarType(yyjson_mut_strncpy(doc(), _var.c_str(), _var.size()));

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, Bytestring>()) {
      const auto encoded = internal::encode_base64(_var);
      return OutputVarType(
          yyjson_mut_strncpy(doc(), encoded.data(), encoded.size()));

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      return OutputVarType(yyjson_mut_bool(doc(), _var));

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      return OutputVarType(yyjson_mut_real(doc(), static_cast<double>(_var)));

    } else if constexpr (std::is_unsigned<std::remove_cvref_t<T>>()) {
      return OutputVarType(yyjson_mut_uint(doc(), static_cast<uint64_t>(_var)));

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      return OutputVarType(yyjson_mut_int(doc(), static_cast<int64_t>(_var)));

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

 private:
  std::shared_ptr<yyjson_mut_doc> doc_;
};

}  // namespace json
}  // namespace rfl

#endif
