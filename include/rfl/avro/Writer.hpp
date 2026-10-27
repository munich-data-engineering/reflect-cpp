#ifndef RFL_AVRO_WRITER_HPP_
#define RFL_AVRO_WRITER_HPP_

#include <avro.h>

#include <climits>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "../always_false.hpp"
#include "../common.hpp"
#include "../concepts.hpp"
#include "../internal/is_literal.hpp"

namespace rfl::avro {

class RFL_API Writer {
 public:
  struct AVROOutputArray {
    avro_value_t val_;
  };

  struct AVROOutputMap {
    avro_value_t val_;
  };

  struct AVROOutputObject {
    avro_value_t val_;
  };

  struct AVROOutputUnion {
    avro_value_t val_;
  };

  struct AVROOutputVar {
    avro_value_t val_;
  };

  using OutputArrayType = AVROOutputArray;
  using OutputMapType = AVROOutputMap;
  using OutputObjectType = AVROOutputObject;
  using OutputUnionType = AVROOutputUnion;
  using OutputVarType = AVROOutputVar;

  /// Constructs a Writer with a root Avro value.
  /// @param _root Pointer to the root avro_value_t that will be written to
  Writer(avro_value_t* _root);

  ~Writer();

  /// Creates an array as the root element of the Avro document.
  /// @param _size The number of elements in the array
  /// @return An OutputArrayType representing the array
  OutputArrayType array_as_root(const size_t _size) const;

  /// Creates a map as the root element of the Avro document.
  /// Avro maps are key-value containers where keys are strings.
  /// @param _size The number of entries in the map
  /// @return An OutputMapType representing the map
  OutputMapType map_as_root(const size_t _size) const;

  /// Creates an object (record) as the root element of the Avro document.
  /// @param _size The number of fields in the object
  /// @return An OutputObjectType representing the object
  OutputObjectType object_as_root(const size_t _size) const;

  /// Creates a null value as the root element of the Avro document.
  /// @return An OutputVarType representing the null value
  OutputVarType null_as_root() const;

  /// Creates a union as the root element of the Avro document.
  /// Avro unions allow a value to be one of several types.
  /// @return An OutputUnionType representing the union
  OutputUnionType union_as_root() const;

  template <class T>
  OutputVarType value_as_root(const T& _var) const {
    set_value(_var, root_);
    return OutputVarType{*root_};
  }

  /// Adds a nested array to an existing array.
  /// @param _size The number of elements in the new array
  /// @param _parent Pointer to the parent array
  /// @return An OutputArrayType representing the new array
  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const;

  /// Adds an array to a map with the specified key.
  /// @param _name The key name for the array in the map
  /// @param _size The number of elements in the array
  /// @param _parent Pointer to the parent map
  /// @return An OutputArrayType representing the new array
  OutputArrayType add_array_to_map(const std::string_view& _name,
                                   const size_t _size,
                                   OutputMapType* _parent) const;

  /// Adds an array to an object (record) field.
  /// @param _name The field name for the array
  /// @param _size The number of elements in the array
  /// @param _parent Pointer to the parent object
  /// @return An OutputArrayType representing the new array
  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t _size,
                                      OutputObjectType* _parent) const;

  /// Adds an array to a specific branch of a union.
  /// @param _index The index of the union branch to use
  /// @param _size The number of elements in the array
  /// @param _parent Pointer to the parent union
  /// @return An OutputArrayType representing the new array
  OutputArrayType add_array_to_union(const size_t _index, const size_t _size,
                                     OutputUnionType* _parent) const;

  /// Adds a nested map to an existing array.
  /// @param _size The number of entries in the new map
  /// @param _parent Pointer to the parent array
  /// @return An OutputMapType representing the new map
  OutputMapType add_map_to_array(const size_t _size,
                                 OutputArrayType* _parent) const;

  /// Adds a map to another map with the specified key.
  /// @param _name The key name for the map in the parent map
  /// @param _size The number of entries in the new map
  /// @param _parent Pointer to the parent map
  /// @return An OutputMapType representing the new map
  OutputMapType add_map_to_map(const std::string_view& _name,
                               const size_t _size,
                               OutputMapType* _parent) const;

  /// Adds a map to an object (record) field.
  /// @param _name The field name for the map
  /// @param _size The number of entries in the map
  /// @param _parent Pointer to the parent object
  /// @return An OutputMapType representing the new map
  OutputMapType add_map_to_object(const std::string_view& _name,
                                  const size_t _size,
                                  OutputObjectType* _parent) const;

  /// Adds a map to a specific branch of a union.
  /// @param _index The index of the union branch to use
  /// @param _size The number of entries in the map
  /// @param _parent Pointer to the parent union
  /// @return An OutputMapType representing the new map
  OutputMapType add_map_to_union(const size_t _index, const size_t _size,
                                 OutputUnionType* _parent) const;

  /// Adds a nested object (record) to an existing array.
  /// @param _size The number of fields in the new object
  /// @param _parent Pointer to the parent array
  /// @return An OutputObjectType representing the new object
  OutputObjectType add_object_to_array(const size_t _size,
                                       OutputArrayType* _parent) const;

  /// Adds an object to a map with the specified key.
  /// @param _name The key name for the object in the map
  /// @param _size The number of fields in the object
  /// @param _parent Pointer to the parent map
  /// @return An OutputObjectType representing the new object
  OutputObjectType add_object_to_map(const std::string_view& _name,
                                     const size_t _size,
                                     OutputMapType* _parent) const;

  /// Adds an object to another object's field.
  /// @param _name The field name for the new object
  /// @param _size The number of fields in the new object
  /// @param _parent Pointer to the parent object
  /// @return An OutputObjectType representing the new object
  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t _size,
                                        OutputObjectType* _parent) const;

  /// Adds an object to a specific branch of a union.
  /// @param _index The index of the union branch to use
  /// @param _size The number of fields in the object
  /// @param _parent Pointer to the parent union
  /// @return An OutputObjectType representing the new object
  OutputObjectType add_object_to_union(const size_t _index, const size_t _size,
                                       OutputUnionType* _parent) const;

  /// Adds a union to an existing array.
  /// @param _parent Pointer to the parent array
  /// @return An OutputUnionType representing the new union
  OutputUnionType add_union_to_array(OutputArrayType* _parent) const;

  /// Adds a union to a map with the specified key.
  /// @param _name The key name for the union in the map
  /// @param _parent Pointer to the parent map
  /// @return An OutputUnionType representing the new union
  OutputUnionType add_union_to_map(const std::string_view& _name,
                                   OutputMapType* _parent) const;

  /// Adds a union to an object (record) field.
  /// @param _name The field name for the union
  /// @param _parent Pointer to the parent object
  /// @return An OutputUnionType representing the new union
  OutputUnionType add_union_to_object(const std::string_view& _name,
                                      OutputObjectType* _parent) const;

  /// Adds a union to a specific branch of another union.
  /// @param _index The index of the parent union's branch to use
  /// @param _parent Pointer to the parent union
  /// @return An OutputUnionType representing the new union
  OutputUnionType add_union_to_union(const size_t _index,
                                     OutputUnionType* _parent) const;

  /// Adds a null value to an array.
  /// @param _parent Pointer to the parent array
  /// @return An OutputVarType representing the null value
  OutputVarType add_null_to_array(OutputArrayType* _parent) const;

  /// Adds a null value to a map with the specified key.
  /// @param _name The key name for the null value
  /// @param _parent Pointer to the parent map
  /// @return An OutputVarType representing the null value
  OutputVarType add_null_to_map(const std::string_view& _name,
                                OutputMapType* _parent) const;

  /// Adds a null value to an object (record) field.
  /// @param _name The field name for the null value
  /// @param _parent Pointer to the parent object
  /// @return An OutputVarType representing the null value
  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const;

  /// Adds a null value to a specific branch of a union.
  /// @param _index The index of the union branch to use
  /// @param _parent Pointer to the parent union
  /// @return An OutputVarType representing the null value
  OutputVarType add_null_to_union(const size_t _index,
                                  OutputUnionType* _parent) const;

  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const {
    avro_value_t new_value;
    int result = avro_value_append(&_parent->val_, &new_value, nullptr);
    if (result != 0) {
      throw std::runtime_error("Error adding value to array: error(" +
                               std::to_string(result) +
                               "): " + avro_strerror());
    }
    set_value(_var, &new_value);
    return OutputVarType{new_value};
  }

  template <class T>
  OutputVarType add_value_to_map(const std::string_view& _name, const T& _var,
                                 OutputMapType* _parent) const {
    avro_value_t new_value;
    // Copy name to nullterminate.
    int result = avro_value_add(&_parent->val_, std::string(_name).c_str(), &new_value,
                                nullptr, nullptr);
    if (result != 0) {
      throw std::runtime_error("Error adding value to map: error(" +
                               std::to_string(result) +
                               "): " + avro_strerror());
    }
    set_value(_var, &new_value);
    return OutputVarType{new_value};
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* _parent) const {
    avro_value_t new_value;
    // Copy name to nullterminate.
    int result = avro_value_get_by_name(&_parent->val_, std::string(_name).c_str(),
                                        &new_value, nullptr);
    if (result != 0) {
      throw std::runtime_error("Error adding value to object: error(" +
                               std::to_string(result) +
                               "): " + avro_strerror());
    }
    set_value(_var, &new_value);
    return OutputVarType{new_value};
  }

  template <class T>
  OutputVarType add_value_to_union(const size_t _index, const T& _var,
                                   OutputUnionType* _parent) const {
    if (_index > static_cast<size_t>(INT_MAX)) {
      throw std::runtime_error("Error adding value to unions: Index error");
    }
    avro_value_t new_value;
    int result = avro_value_set_branch(&_parent->val_, static_cast<int>(_index),
                                       &new_value);
    if (result != 0) {
      throw std::runtime_error("Error adding value to union: error(" +
                               std::to_string(result) +
                               "): " + avro_strerror());
    }
    set_value(_var, &new_value);
    return OutputVarType{new_value};
  }

  /// Finalizes an array after all elements have been added.
  /// This is a no-op for Avro but required by the Writer interface.
  /// @param _arr Pointer to the array to finalize (unused)
  void end_array(OutputArrayType* /*_arr*/) const noexcept {}

  /// Finalizes a map after all entries have been added.
  /// This is a no-op for Avro but required by the Writer interface.
  /// @param _obj Pointer to the map to finalize (unused)
  void end_map(OutputMapType* /*_obj*/) const noexcept {}

  /// Finalizes an object after all fields have been added.
  /// This is a no-op for Avro but required by the Writer interface.
  /// @param _obj Pointer to the object to finalize (unused)
  void end_object(OutputObjectType* /*_obj*/) const noexcept {}

 private:
  template <class T>
  void set_value(const T& _var, avro_value_t* _val) const {
    using Type = std::remove_cvref_t<T>;

    if constexpr (std::is_same_v<Type, std::string>) {
      int result =
          avro_value_set_string_len(_val, _var.c_str(), _var.size() + 1);
      if (result != 0) {
        throw std::runtime_error(
            "Error setting string value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (concepts::MutableContiguousByteContainer<Type>) {
      auto var = _var;
      // Avro requires a non-null pointer even for an empty bytes value.
      typename Type::value_type empty{};
      auto* data = var.size() == 0 ? &empty : var.data();
      int result = avro_value_set_bytes(_val, data, var.size());
      if (result != 0) {
        throw std::runtime_error(
            "Error setting bytestring value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (std::is_same_v<Type, bool>) {
      int result = avro_value_set_boolean(_val, _var);
      if (result != 0) {
        throw std::runtime_error(
            "Error setting boolean value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (std::is_same_v<Type, float>) {
      int result = avro_value_set_float(_val, _var);
      if (result != 0) {
        throw std::runtime_error(
            "Error setting float value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (std::is_same_v<Type, double>) {
      int result = avro_value_set_double(_val, _var);
      if (result != 0) {
        throw std::runtime_error(
            "Error setting double value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (std::is_floating_point_v<Type>) {
      int result = avro_value_set_double(_val, static_cast<double>(_var));
      if (result != 0) {
        throw std::runtime_error(
            "Error setting double value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (std::is_same_v<Type, std::int64_t> ||
                         std::is_same_v<Type, std::uint32_t> ||
                         std::is_same_v<Type, std::uint64_t>) {
      int result = avro_value_set_long(_val, static_cast<std::int64_t>(_var));
      if (result != 0) {
        throw std::runtime_error(
            "Error setting long value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (std::is_integral_v<Type>) {
      int result = avro_value_set_int(_val, static_cast<std::int32_t>(_var));
      if (result != 0) {
        throw std::runtime_error(
            "Error setting int value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else if constexpr (internal::is_literal_v<T>) {
      int result = avro_value_set_enum(_val, static_cast<int>(_var.value()));
      if (result != 0) {
        throw std::runtime_error(
            "Error setting literal value: " + std::to_string(result) + ": " +
            avro_strerror());
      }

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

 private:
  avro_value_t* root_;
};

}  // namespace rfl::avro

#endif
