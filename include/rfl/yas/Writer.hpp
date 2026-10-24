#ifndef RFL_YAS_WRITER_HPP_
#define RFL_YAS_WRITER_HPP_

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <yas/binary_oarchive.hpp>
#include <yas/mem_streams.hpp>
#include <yas/std_types.hpp>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/is_literal.hpp"
#include "../parsing/schemaful/IsSchemafulWriter.hpp"

namespace rfl::yas {

struct Writer {
  using OArchive = ::yas::binary_oarchive<::yas::mem_ostream>;

  struct OutputArrayType {
    OArchive* ar;
  };

  struct OutputMapType {
    OArchive* ar;
  };

  struct OutputObjectType {
    OArchive* ar;
  };

  struct OutputUnionType {
    OArchive* ar;
  };

  struct OutputVarType {
    OArchive* ar;
  };

  ///
  /// @brief Constructs a Writer with the given output archive pointer.
  /// @param _ar Pointer to the output archive used for serialization.
  Writer(OArchive* _ar) : ar_(_ar) {}

  ///
  /// @brief Begins serializing an array as the root object.
  /// @param _size The number of elements in the array.
  /// @return OutputArrayType representing the serialized array context.
  OutputArrayType array_as_root(const size_t _size) const noexcept {
    (*ar_) & _size;
    return OutputArrayType{ar_};
  }

  /// Fixed-size arrays store only their elements; parent metadata is retained.
  OutputArrayType fixed_array_as_root(const size_t /*_size*/) const {
    return OutputArrayType{ar_};
  }

  OutputArrayType add_fixed_array_to_array(const size_t /*_size*/,
                                           OutputArrayType* /*_parent*/) const {
    return OutputArrayType{ar_};
  }

  OutputArrayType add_fixed_array_to_object(
      const std::string_view& /*_name*/, const size_t /*_size*/,
      OutputObjectType* /*_parent*/) const {
    return OutputArrayType{ar_};
  }

  OutputArrayType add_fixed_array_to_map(const std::string_view& _name,
                                         const size_t /*_size*/,
                                         OutputMapType* /*_parent*/) const {
    add_string_view(_name);
    return OutputArrayType{ar_};
  }

  OutputArrayType add_fixed_array_to_union(const size_t _index,
                                           const size_t /*_size*/,
                                           OutputUnionType* /*_parent*/) const {
    (*ar_) & _index;
    return OutputArrayType{ar_};
  }

  ///
  /// @brief Begins serializing a map as the root object.
  /// @param _size The number of key-value pairs in the map.
  /// @return OutputMapType representing the serialized map context.
  OutputMapType map_as_root(const size_t _size) const noexcept {
    (*ar_) & _size;
    return OutputMapType{ar_};
  }

  ///
  /// @brief Begins serializing an object as the root object.
  /// @param _size The number of fields in the object (unused).
  /// @return OutputObjectType representing the serialized object context.
  OutputObjectType object_as_root(const size_t /*_size*/) const noexcept {
    return OutputObjectType{ar_};
  }

  ///
  /// @brief Begins serializing a union as the root object.
  /// @return OutputUnionType representing the serialized union context.
  OutputUnionType union_as_root() const noexcept {
    return OutputUnionType{ar_};
  }

  ///
  /// @brief Begins serializing a null value as the root object.
  /// @return OutputVarType representing the serialized null context.
  OutputVarType null_as_root() const noexcept { return OutputVarType{ar_}; }

  ///
  /// @brief Begins serializing a value as the root object.
  /// @tparam T The type of the value.
  /// @param _var The value to serialize.
  /// @return OutputVarType representing the serialized value context.
  template <class T>
  OutputVarType value_as_root(const T& _var) const noexcept {
    add_value(_var);
    return OutputVarType{ar_};
  }

  ///
  /// @brief Adds an array as a child to an array parent during serialization.
  /// @param _size The number of elements in the child array.
  /// @param _parent Pointer to the parent OutputArrayType.
  /// @return OutputArrayType representing the new array context.
  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const noexcept {
    (*_parent->ar) & _size;
    return OutputArrayType{_parent->ar};
  }

  ///
  /// @brief Adds an array as a child to a map parent during serialization.
  /// @param _name The key name for the array in the map.
  /// @param _size The number of elements in the array.
  /// @param _parent Pointer to the parent OutputMapType.
  /// @return OutputArrayType representing the new array context.
  OutputArrayType add_array_to_map(const std::string_view& _name,
                                   const size_t _size,
                                   OutputMapType* _parent) const noexcept;

  ///
  /// @brief Adds an array as a child to an object parent during serialization.
  /// @param _name The field name for the array in the object (unused).
  /// @param _size The number of elements in the array.
  /// @param _parent Pointer to the parent OutputObjectType.
  /// @return OutputArrayType representing the new array context.
  OutputArrayType add_array_to_object(
      const std::string_view& /*_name*/, const size_t _size,
      OutputObjectType* _parent) const noexcept {
    (*_parent->ar) & _size;
    return OutputArrayType{_parent->ar};
  }

  ///
  /// @brief Adds an array as a child to a union parent during serialization.
  /// @param _index The union field index.
  /// @param _size The number of elements in the array.
  /// @param _parent Pointer to the parent OutputUnionType.
  /// @return OutputArrayType representing the new array context.
  OutputArrayType add_array_to_union(const size_t _index, const size_t _size,
                                     OutputUnionType* _parent) const noexcept;

  ///
  /// @brief Adds a map as a child to an array parent during serialization.
  /// @param _size The number of key-value pairs in the map.
  /// @param _parent Pointer to the parent OutputArrayType.
  /// @return OutputMapType representing the new map context.
  OutputMapType add_map_to_array(const size_t _size,
                                 OutputArrayType* _parent) const noexcept {
    (*_parent->ar) & _size;
    return OutputMapType{_parent->ar};
  }

  ///
  /// @brief Adds a map as a child to an object parent during serialization.
  /// @param _name The field name for the map in the object (unused).
  /// @param _size The number of key-value pairs in the map.
  /// @param _parent Pointer to the parent OutputObjectType.
  /// @return OutputMapType representing the new map context.
  OutputMapType add_map_to_object(const std::string_view& /*_name*/,
                                  const size_t _size,
                                  OutputObjectType* _parent) const noexcept {
    (*_parent->ar) & _size;
    return OutputMapType{_parent->ar};
  }

  ///
  /// @brief Adds a map as a child to a map parent during serialization.
  /// @param _name The key name for the map in the parent map.
  /// @param _size The number of key-value pairs in the map.
  /// @param _parent Pointer to the parent OutputMapType.
  /// @return OutputMapType representing the new map context.
  OutputMapType add_map_to_map(const std::string_view& _name,
                               const size_t _size,
                               OutputMapType* _parent) const noexcept;

  ///
  /// @brief Adds a map as a child to a union parent during serialization.
  /// @param _index The union field index.
  /// @param _size The number of key-value pairs in the map.
  /// @param _parent Pointer to the parent OutputUnionType.
  /// @return OutputMapType representing the new map context.
  OutputMapType add_map_to_union(const size_t _index, const size_t _size,
                                 OutputUnionType* _parent) const noexcept;

  ///
  /// @brief Adds an object as a child to an array parent during serialization.
  /// @param _size The number of fields in the object (unused).
  /// @param _parent Pointer to the parent OutputArrayType.
  /// @return OutputObjectType representing the new object context.
  OutputObjectType add_object_to_array(
      const size_t /*_size*/, OutputArrayType* _parent) const noexcept {
    return OutputObjectType{_parent->ar};
  }

  ///
  /// @brief Adds an object as a child to a map parent during serialization.
  /// @param _name The key name for the object in the map.
  /// @param _size The number of fields in the object.
  /// @param _parent Pointer to the parent OutputMapType.
  /// @return OutputObjectType representing the new object context.
  OutputObjectType add_object_to_map(const std::string_view& _name,
                                     const size_t _size,
                                     OutputMapType* _parent) const noexcept;

  ///
  /// @brief Adds an object as a child to an object parent during serialization.
  /// @param _name The field name for the object in the parent object (unused).
  /// @param _size The number of fields in the object (unused).
  /// @param _parent Pointer to the parent OutputObjectType.
  /// @return OutputObjectType representing the new object context.
  OutputObjectType add_object_to_object(
      const std::string_view& /*_name*/, const size_t /*_size*/,
      OutputObjectType* _parent) const noexcept {
    return OutputObjectType{_parent->ar};
  }

  ///
  /// @brief Adds an object as a child to a union parent during serialization.
  /// @param _index The union field index.
  /// @param _size The number of fields in the object.
  /// @param _parent Pointer to the parent OutputUnionType.
  /// @return OutputObjectType representing the new object context.
  OutputObjectType add_object_to_union(const size_t _index, const size_t _size,
                                       OutputUnionType* _parent) const noexcept;

  ///
  /// @brief Adds a union as a child to an array parent during serialization.
  /// @param _parent Pointer to the parent OutputArrayType.
  /// @return OutputUnionType representing the new union context.
  OutputUnionType add_union_to_array(OutputArrayType* _parent) const noexcept {
    return OutputUnionType{_parent->ar};
  }

  ///
  /// @brief Adds a union as a child to a map parent during serialization.
  /// @param _name The key name for the union in the map.
  /// @param _parent Pointer to the parent OutputMapType.
  /// @return OutputUnionType representing the new union context.
  OutputUnionType add_union_to_map(const std::string_view& _name,
                                   OutputMapType* _parent) const noexcept;

  ///
  /// @brief Adds a union as a child to an object parent during serialization.
  /// @param _name The field name for the union in the object (unused).
  /// @param _parent Pointer to the parent OutputObjectType.
  /// @return OutputUnionType representing the new union context.
  OutputUnionType add_union_to_object(
      const std::string_view& /*_name*/,
      OutputObjectType* _parent) const noexcept {
    return OutputUnionType{_parent->ar};
  }

  ///
  /// @brief Adds a union as a child to a union parent during serialization.
  /// @param _index The union field index.
  /// @param _parent Pointer to the parent OutputUnionType.
  /// @return OutputUnionType representing the new union context.
  OutputUnionType add_union_to_union(const size_t _index,
                                     OutputUnionType* _parent) const noexcept;

  ///
  /// @brief Adds a value as a child to an array parent during serialization.
  /// @tparam T The type of the value.
  /// @param _var The value to serialize.
  /// @param _parent Pointer to the parent OutputArrayType.
  /// @return OutputVarType representing the serialized value context.
  template <class T>
  OutputVarType add_value_to_array(const T& _var, OutputArrayType*) const {
    add_value(_var);
    return OutputVarType{};
  }

  ///
  /// @brief Adds a value as a child to a map parent during serialization.
  /// @tparam T The type of the value.
  /// @param _name The key name for the value in the map.
  /// @param _var The value to serialize.
  /// @param _parent Pointer to the parent OutputMapType.
  /// @return OutputVarType representing the serialized value context.
  template <class T>
  OutputVarType add_value_to_map(const std::string_view& _name, const T& _var,
                                 OutputMapType* _parent) const {
    add_string_view(_name);
    add_value(_var);
    return OutputVarType{};
  }

  ///
  /// @brief Adds a value as a child to an object parent during serialization.
  /// @tparam T The type of the value.
  /// @param _name The field name for the value in the object.
  /// @param _var The value to serialize.
  /// @param _parent Pointer to the parent OutputObjectType.
  /// @return OutputVarType representing the serialized value context.
  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var, OutputObjectType*) const {
    add_value(_var);
    return OutputVarType{};
  }

  ///
  /// @brief Adds a value as a child to a union parent during serialization.
  /// @tparam T The type of the value.
  /// @param _index The union field index.
  /// @param _var The value to serialize.
  /// @param _parent Pointer to the parent OutputUnionType.
  /// @return OutputVarType representing the serialized value context.
  template <class T>
  OutputVarType add_value_to_union(const size_t _index, const T& _var,
                                   OutputUnionType* _parent) const {
    (*ar_) & _index;
    add_value(_var);
    return OutputVarType{};
  }

  ///
  /// @brief Adds a null value as a child to an array parent during
  /// serialization.
  /// @param _parent Pointer to the parent OutputArrayType (unused).
  /// @return OutputVarType representing the serialized null context.
  OutputVarType add_null_to_array(OutputArrayType* /*_parent*/) const {
    return OutputVarType{};
  }

  ///
  /// @brief Adds a null value as a child to a map parent during serialization.
  /// @param _name The key name for the null value (unused).
  /// @param _parent Pointer to the parent OutputMapType (unused).
  /// @return OutputVarType representing the serialized null context.
  OutputVarType add_null_to_map(const std::string_view& /*_name*/,
                                OutputMapType* /*_parent*/) const {
    return OutputVarType{};
  }

  ///
  /// @brief Adds a null value as a child to an object parent during
  /// serialization.
  /// @param _name The field name for the null value (unused).
  /// @param _parent Pointer to the parent OutputObjectType (unused).
  /// @return OutputVarType representing the serialized null context.
  OutputVarType add_null_to_object(const std::string_view& /*_name*/,
                                   OutputObjectType* /*_parent*/) const {
    return OutputVarType{};
  }

  ///
  /// @brief Adds a null value as a child to a union parent during
  /// serialization.
  /// @param _index The union field index.
  /// @param _parent Pointer to the parent OutputUnionType.
  /// @return OutputVarType representing the serialized null context.
  OutputVarType add_null_to_union(const size_t _index,
                                  OutputUnionType* _parent) const noexcept;

  ///
  /// @brief Marks the end of an array serialization context.
  /// @param _arr Pointer to the OutputArrayType (unused).
  void end_array(OutputArrayType* /*_arr*/) const noexcept {}

  ///
  /// @brief Marks the end of a map serialization context.
  /// @param _map Pointer to the OutputMapType (unused).
  void end_map(OutputMapType* /*_map*/) const noexcept {}

  ///
  /// @brief Marks the end of an object serialization context.
  /// @param _obj Pointer to the OutputObjectType (unused).
  void end_object(OutputObjectType* /*_obj*/) const noexcept {}

  ///
  /// @brief Serializes a string_view as a sequence of bytes.
  /// @param _str The string_view to serialize.
  void add_string_view(const std::string_view& _str) const {
    ar_->write_seq_size(_str.length());
    ar_->write(_str.data(), _str.length());
  }

  ///
  /// @brief Serializes a value, handling literal types specially.
  /// @tparam T The type of the value.
  /// @param _var The value to serialize.
  template <class T>
  void add_value(const T& _var) const {
    if constexpr (internal::is_literal_v<T>) {
      (*ar_) & _var.str();
    } else {
      (*ar_) & _var;
    }
  }

  ///
  /// @brief Pointer to the output archive used for serialization.
  OArchive* ar_;
};

static_assert(parsing::schemaful::IsSchemafulWriter<Writer>);

}  // namespace rfl::yas

#endif
