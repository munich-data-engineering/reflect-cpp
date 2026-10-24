#ifndef RFL_BOOST_SERIALIZATION_WRITER_HPP_
#define RFL_BOOST_SERIALIZATION_WRITER_HPP_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "../always_false.hpp"
#include "../internal/is_literal.hpp"

namespace rfl::boost_serialization {

template <class OArchive>
class Writer {
 public:
  struct BoostOutputArray {};

  struct BoostOutputMap {};

  struct BoostOutputObject {};

  struct BoostOutputUnion {};

  struct BoostOutputVar {};

  using OutputArrayType = BoostOutputArray;
  using OutputMapType = BoostOutputMap;
  using OutputObjectType = BoostOutputObject;
  using OutputUnionType = BoostOutputUnion;
  using OutputVarType = BoostOutputVar;

  /// Constructs a Writer with a Boost output archive.
  /// @param _ar Pointer to the Boost output archive to write to
  Writer(OArchive* _ar) : ar_(_ar) {}

  ~Writer() = default;

  /// Creates an array as the root element.
  /// Writes the size to the archive first.
  /// @param _size The number of elements in the array
  /// @return An OutputArrayType
  OutputArrayType array_as_root(const size_t _size) const {
    write_size(_size);
    return OutputArrayType{};
  }

  /// Fixed-size arrays store only their elements; parent metadata is retained.
  OutputArrayType fixed_array_as_root(const size_t /*_size*/) const {
    return OutputArrayType{};
  }

  OutputArrayType add_fixed_array_to_array(const size_t /*_size*/,
                                           OutputArrayType* /*_parent*/) const {
    return OutputArrayType{};
  }

  OutputArrayType add_fixed_array_to_object(
      const std::string_view& /*_name*/, const size_t /*_size*/,
      OutputObjectType* /*_parent*/) const {
    return OutputArrayType{};
  }

  OutputArrayType add_fixed_array_to_map(const std::string_view& _name,
                                         const size_t /*_size*/,
                                         OutputMapType* /*_parent*/) const {
    write_key(_name);
    return OutputArrayType{};
  }

  OutputArrayType add_fixed_array_to_union(const size_t _index,
                                           const size_t /*_size*/,
                                           OutputUnionType* /*_parent*/) const {
    write_index(_index);
    return OutputArrayType{};
  }

  /// Creates a map as the root element.
  /// Writes the size to the archive first.
  /// @param _size The number of entries in the map
  /// @return An OutputMapType
  OutputMapType map_as_root(const size_t _size) const {
    write_size(_size);
    return OutputMapType{};
  }

  /// Creates an object as the root element.
  /// Writes the size to the archive first.
  /// @param _size The number of fields in the object
  /// @return An OutputObjectType
  OutputObjectType object_as_root(const size_t _size) const {
    write_size(_size);
    return OutputObjectType{};
  }

  /// Creates a null value as the root element.
  /// This is a no-op for Boost.Serialization.
  /// @return An OutputVarType
  OutputVarType null_as_root() const { return OutputVarType{}; }

  /// Creates a union as the root element.
  /// @return An OutputUnionType
  OutputUnionType union_as_root() const { return OutputUnionType{}; }

  /// Writes a value as the root element.
  /// @tparam T The type of the value
  /// @param _var The value to write
  /// @return An OutputVarType
  template <class T>
  OutputVarType value_as_root(const T& _var) const {
    new_value(_var);
    return OutputVarType{};
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* /*_parent*/) const {
    write_size(_size);
    return OutputArrayType{};
  }

  OutputArrayType add_array_to_map(const std::string_view& _name,
                                   const size_t _size,
                                   OutputMapType* /*_parent*/) const {
    write_key(_name);
    write_size(_size);
    return OutputArrayType{};
  }

  OutputArrayType add_array_to_object(const std::string_view& /*_name*/,
                                      const size_t _size,
                                      OutputObjectType* /*_parent*/) const {
    write_size(_size);
    return OutputArrayType{};
  }

  OutputArrayType add_array_to_union(const size_t _index, const size_t _size,
                                     OutputUnionType* /*_parent*/) const {
    write_index(_index);
    write_size(_size);
    return OutputArrayType{};
  }

  OutputMapType add_map_to_array(const size_t _size,
                                 OutputArrayType* /*_parent*/) const {
    write_size(_size);
    return OutputMapType{};
  }

  OutputMapType add_map_to_map(const std::string_view& _name,
                               const size_t _size,
                               OutputMapType* /*_parent*/) const {
    write_key(_name);
    write_size(_size);
    return OutputMapType{};
  }

  OutputMapType add_map_to_object(const std::string_view& /*_name*/,
                                  const size_t _size,
                                  OutputObjectType* /*_parent*/) const {
    write_size(_size);
    return OutputMapType{};
  }

  OutputMapType add_map_to_union(const size_t _index, const size_t _size,
                                 OutputUnionType* /*_parent*/) const {
    write_index(_index);
    write_size(_size);
    return OutputMapType{};
  }

  OutputObjectType add_object_to_array(const size_t _size,
                                       OutputArrayType* /*_parent*/) const {
    write_size(_size);
    return OutputObjectType{};
  }

  OutputObjectType add_object_to_map(const std::string_view& _name,
                                     const size_t _size,
                                     OutputMapType* /*_parent*/) const {
    write_key(_name);
    write_size(_size);
    return OutputObjectType{};
  }

  OutputObjectType add_object_to_object(const std::string_view& /*_name*/,
                                        const size_t _size,
                                        OutputObjectType* /*_parent*/) const {
    write_size(_size);
    return OutputObjectType{};
  }

  OutputObjectType add_object_to_union(const size_t _index, const size_t _size,
                                       OutputUnionType* /*_parent*/) const {
    write_index(_index);
    write_size(_size);
    return OutputObjectType{};
  }

  OutputUnionType add_union_to_array(OutputArrayType* /*_parent*/) const {
    return OutputUnionType{};
  }

  OutputUnionType add_union_to_map(const std::string_view& _name,
                                   OutputMapType* /*_parent*/) const {
    write_key(_name);
    return OutputUnionType{};
  }

  OutputUnionType add_union_to_object(const std::string_view& /*_name*/,
                                      OutputObjectType* /*_parent*/) const {
    return OutputUnionType{};
  }

  OutputUnionType add_union_to_union(const size_t _index,
                                     OutputUnionType* /*_parent*/) const {
    write_index(_index);
    return OutputUnionType{};
  }

  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* /*_parent*/) const {
    new_value(_var);
    return OutputVarType{};
  }

  template <class T>
  OutputVarType add_value_to_map(const std::string_view& _name, const T& _var,
                                 OutputMapType* /*_parent*/) const {
    write_key(_name);
    new_value(_var);
    return OutputVarType{};
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& /*_name*/,
                                    const T& _var,
                                    OutputObjectType* /*_parent*/) const {
    new_value(_var);
    return OutputVarType{};
  }

  template <class T>
  OutputVarType add_value_to_union(const size_t _index, const T& _var,
                                   OutputUnionType* /*_parent*/) const {
    write_index(_index);
    new_value(_var);
    return OutputVarType{};
  }

  OutputVarType add_null_to_array(OutputArrayType* /*_parent*/) const {
    return OutputVarType{};
  }

  OutputVarType add_null_to_map(const std::string_view& _name,
                                OutputMapType* /*_parent*/) const {
    write_key(_name);
    return OutputVarType{};
  }

  OutputVarType add_null_to_object(const std::string_view& /*_name*/,
                                   OutputObjectType* /*_parent*/) const {
    return OutputVarType{};
  }

  OutputVarType add_null_to_union(const size_t _index,
                                  OutputUnionType* /*_parent*/) const {
    write_index(_index);
    return OutputVarType{};
  }

  /// Finalizes an array after all elements have been added.
  /// This is a no-op for Boost.Serialization.
  /// @param _arr Pointer to the array (unused)
  void end_array(OutputArrayType* /*_arr*/) const {}

  /// Finalizes a map after all entries have been added.
  /// This is a no-op for Boost.Serialization.
  /// @param _obj Pointer to the map (unused)
  void end_map(OutputMapType* /*_obj*/) const {}

  /// Finalizes an object after all fields have been added.
  /// This is a no-op for Boost.Serialization.
  /// @param _obj Pointer to the object (unused)
  void end_object(OutputObjectType* /*_obj*/) const {}

  template <class T>
  void write_array_data(const T* _data, const std::size_t _size) const {
    if (_size != 0) {
      ar_->save_binary(_data, _size * sizeof(T));
    }
  }

 private:
  void write_size(const size_t _size) const {
    *ar_ << static_cast<std::uint64_t>(_size);
  }

  void write_index(const size_t _index) const {
    *ar_ << static_cast<std::uint64_t>(_index);
  }

  void write_key(const std::string_view& _name) const {
    *ar_ << std::string(_name);
  }

  template <class T>
  void new_value(const T& _var) const {
    using Type = std::remove_cvref_t<T>;
    if constexpr (internal::is_literal_v<Type>) {
      *ar_ << static_cast<std::int64_t>(_var.value());
    } else {
      *ar_ << _var;
    }
  }

 private:
  OArchive* ar_;
};

}  // namespace rfl::boost_serialization

#endif
