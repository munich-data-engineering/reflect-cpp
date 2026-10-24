#ifndef RFL_CEREAL_WRITER_HPP_
#define RFL_CEREAL_WRITER_HPP_

#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../common.hpp"
#include "../internal/is_literal.hpp"
#include "../parsing/schemaful/IsSchemafulWriter.hpp"

namespace rfl::cereal {

/// Writer for Cereal serialization library integration.
/// Writes data to Cereal's portable binary archive format for cross-platform
/// serialization.
class Writer {
 public:
  using CerealArchive = ::cereal::PortableBinaryOutputArchive;

  struct CerealOutputArray {};

  struct CerealOutputMap {};

  struct CerealOutputObject {};

  struct CerealOutputUnion {};

  struct CerealOutputVar {};

  using OutputArrayType = CerealOutputArray;
  using OutputObjectType = CerealOutputObject;
  using OutputMapType = CerealOutputMap;
  using OutputUnionType = CerealOutputUnion;
  using OutputVarType = CerealOutputVar;

  /// A context for writing a typed map entry's key, then its value.
  struct OutputMapEntryType {};

  OutputMapEntryType map_entry(OutputMapType*) const { return {}; }

  OutputArrayType add_array_to_map_entry(const size_t _size,
                                         OutputMapEntryType*) const {
    return array_as_root(_size);
  }

  OutputObjectType add_object_to_map_entry(const size_t _size,
                                           OutputMapEntryType*) const {
    return object_as_root(_size);
  }

  template <class T>
  OutputVarType add_value_to_map_entry(const T& _var,
                                       OutputMapEntryType*) const {
    add_value(_var);
    return OutputVarType{};
  }

  OutputVarType add_null_to_map_entry(OutputMapEntryType*) const {
    return null_as_root();
  }

  OutputMapType add_map_to_map_entry(const size_t _size,
                                     OutputMapEntryType*) const {
    return map_as_root(_size);
  }

  OutputUnionType add_union_to_map_entry(OutputMapEntryType*) const {
    return union_as_root();
  }

  /// Constructs a Writer with a Cereal output archive.
  /// @param _archive Pointer to the Cereal output archive to write to
  Writer(CerealArchive* _archive);

  ~Writer() = default;

  /// Creates an array as the root element.
  /// Writes the size tag to the archive first.
  /// @param _size The number of elements in the array
  /// @return An OutputArrayType
  OutputArrayType array_as_root(const size_t _size) const;

  /// Creates a map as the root element.
  /// Writes the size tag to the archive first.
  /// @param _size The number of entries in the map
  /// @return An OutputMapType
  OutputMapType map_as_root(const size_t _size) const;

  /// Creates an object as the root element.
  /// @param _size The number of fields in the object
  /// @return An OutputObjectType
  OutputObjectType object_as_root(const size_t _size) const;

  /// Creates a union as the root element.
  /// @return An OutputUnionType
  OutputUnionType union_as_root() const;

  /// Creates a null value as the root element.
  /// @return An OutputVarType
  OutputVarType null_as_root() const;

  template <class T>
  OutputVarType value_as_root(const T& _var) const {
    (*archive_)(_var);
    return OutputVarType{};
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const;

  OutputArrayType add_array_to_map(const std::string_view& _name,
                                   const size_t _size,
                                   OutputMapType* _parent) const;

  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t _size,
                                      OutputObjectType* _parent) const;

  OutputArrayType add_array_to_union(const size_t _index, const size_t _size,
                                     OutputUnionType* _parent) const;

  OutputMapType add_map_to_array(const size_t _size,
                                 OutputArrayType* _parent) const;

  OutputMapType add_map_to_map(const std::string_view& _name,
                               const size_t _size,
                               OutputMapType* _parent) const;

  OutputMapType add_map_to_object(const std::string_view& _name,
                                  const size_t _size,
                                  OutputObjectType* _parent) const;

  OutputMapType add_map_to_union(const size_t _index, const size_t _size,
                                 OutputUnionType* _parent) const;

  OutputObjectType add_object_to_array(const size_t _size,
                                       OutputArrayType* _parent) const;

  OutputObjectType add_object_to_map(const std::string_view& _name,
                                     const size_t _size,
                                     OutputMapType* _parent) const;

  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t _size,
                                        OutputObjectType* _parent) const;

  OutputObjectType add_object_to_union(const size_t _index, const size_t _size,
                                       OutputUnionType* _parent) const;

  OutputUnionType add_union_to_array(OutputArrayType* _parent) const;

  OutputUnionType add_union_to_map(const std::string_view& _name,
                                   OutputMapType* _parent) const;

  OutputUnionType add_union_to_object(const std::string_view& _name,
                                      OutputObjectType* _parent) const;

  OutputUnionType add_union_to_union(const size_t _index,
                                     OutputUnionType* _parent) const;

  template <class T>
  OutputVarType add_value_to_array(const T& _var, OutputArrayType*) const {
    add_value(_var);
    return OutputVarType{};
  }

  template <class T>
  OutputVarType add_value_to_map(const std::string_view& _name, const T& _var,
                                 OutputMapType* _parent) const {
    add_string_view(_name);
    add_value(_var);
    return OutputVarType{};
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var, OutputObjectType*) const {
    add_value(_var);
    return OutputVarType{};
  }

  template <class T>
  OutputVarType add_value_to_union(const size_t _index, const T& _var,
                                   OutputUnionType* _parent) const {
    (*archive_)(_index);
    add_value(_var);
    return OutputVarType{};
  }

  OutputVarType add_null_to_array(OutputArrayType* _parent) const;

  OutputVarType add_null_to_map(const std::string_view& _name,
                                OutputMapType* _parent) const;

  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const;

  OutputVarType add_null_to_union(const size_t _index,
                                  OutputUnionType* _parent) const;

  void end_array(OutputArrayType* _arr) const noexcept;

  void end_map(OutputMapType* _map) const noexcept;

  void end_object(OutputObjectType* _obj) const noexcept;

  template <class T>
  void write_array_data(const T* _data, const std::size_t _size) const {
    if (_size != 0) {
      // A pointer value preserves the element width in Cereal BinaryData.
      (*archive_)(::cereal::binary_data(static_cast<const T*>(_data),
                                        _size * sizeof(T)));
    }
  }

 private:
  void add_string_view(const std::string_view& _str) const;

  template <class T>
  void add_value(const T& _var) const noexcept {
    using Type = std::remove_cvref_t<T>;
    if constexpr (internal::is_literal_v<Type>) {
      add_value(_var.str());

    } else if constexpr (std::is_same<Type, rfl::Bytestring>() ||
                         std::is_same<Type, rfl::Vectorstring>()) {
      (*archive_)(::cereal::make_size_tag(_var.size()));
      (*archive_)(::cereal::binary_data(_var.data(), _var.size()));

    } else {
      (*archive_)(_var);
    }
  }

 private:
  CerealArchive* archive_;
};

static_assert(parsing::schemaful::IsSchemafulWriter<Writer>);

}  // namespace rfl::cereal

#endif
