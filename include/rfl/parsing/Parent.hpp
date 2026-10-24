#ifndef RFL_PARSING_PARENT_HPP_
#define RFL_PARSING_PARENT_HPP_

#include <string_view>
#include <type_traits>

#include "../always_false.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "supports_attributes.hpp"
#include "supports_comments.hpp"

namespace rfl::parsing {

template <class W>
struct Parent {
  using OutputArrayType = typename W::OutputArrayType;
  using OutputObjectType = typename W::OutputObjectType;
  using OutputVarType = typename W::OutputVarType;

  struct Array {
    OutputArrayType* arr_;
  };

  // The key and value of a typed map entry are serialized in that order.
  template <class T>
  struct MapEntry {
    T* entry_;
  };

  // For schemaful formats only.
  template <class T>
  struct Map {
    std::string_view name_;
    T* map_;
  };

  struct Object {
    std::string_view name_;
    OutputObjectType* obj_;
    bool is_attribute_ = false;
    Object as_attribute() const { return Object{name_, obj_, true}; }
  };

  // For schemaful formats only.
  template <class T>
  struct Union {
    size_t index_;
    T* union_;
  };

  struct Root {};

  /**
   * @brief Adds an array to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @param _w The writer to use.
   * @param _size The size of the array.
   * @param _parent The parent object.
   * @return The new array.
   */
  template <class ParentType>
  static OutputArrayType add_array(const W& _w, const size_t _size,
                                   const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (std::is_same<Type, Array>()) {
      return _w.add_array_to_array(_size, _parent.arr_);

    } else if constexpr (is_map_entry<Type>()) {
      return _w.add_array_to_map_entry(_size, _parent.entry_);

    } else if constexpr (std::is_same<Type, Object>()) {
      return _w.add_array_to_object(_parent.name_, _size, _parent.obj_);

    } else if constexpr (std::is_same<Type, Root>()) {
      return _w.array_as_root(_size);

    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      if constexpr (std::is_same<Type, Map<typename W::OutputMapType>>()) {
        return _w.add_array_to_map(_parent.name_, _size, _parent.map_);
      } else if constexpr (std::is_same<Type,
                                        Union<typename W::OutputUnionType>>()) {
        return _w.add_array_to_union(_parent.index_, _size, _parent.union_);
      } else {
        static_assert(always_false_v<Type>, "Unsupported option.");
      }

    } else {
      static_assert(always_false_v<Type>, "Unsupported option.");
    }
  }

  /**
   * @brief Adds a comment to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @param _w The writer to use.
   * @param _comment The comment to add.
   * @param _parent The parent object.
   */
  template <class ParentType>
  static void add_comment(const W& _w, std::string_view _comment,
                          const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (supports_comments<W>) {
      if constexpr (std::is_same<Type, Array>()) {
        _w.add_comment_to_array(_comment, _parent.arr_);

      } else if constexpr (std::is_same<Type, Object>()) {
        _w.add_comment_to_object(_comment, _parent.obj_);
      }
    }
  }

  /**
   * @brief Adds a map to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @param _w The writer to use.
   * @param _size The size of the map.
   * @param _parent The parent object.
   * @return The new map.
   */
  template <class ParentType>
  static auto add_map(const W& _w, const size_t _size,
                      const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (std::is_same<Type, Array>()) {
      return _w.add_map_to_array(_size, _parent.arr_);

    } else if constexpr (is_map_entry<Type>()) {
      return _w.add_map_to_map_entry(_size, _parent.entry_);

    } else if constexpr (std::is_same<Type, Map<typename W::OutputMapType>>()) {
      return _w.add_map_to_map(_parent.name_, _size, _parent.map_);

    } else if constexpr (std::is_same<Type, Object>()) {
      return _w.add_map_to_object(_parent.name_, _size, _parent.obj_);

    } else if constexpr (std::is_same<Type,
                                      Union<typename W::OutputUnionType>>()) {
      return _w.add_map_to_union(_parent.index_, _size, _parent.union_);

    } else if constexpr (std::is_same<Type, Root>()) {
      return _w.map_as_root(_size);

    } else {
      static_assert(always_false_v<Type>, "Unsupported option.");
    }
  }

  /**
   * @brief Adds an object to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @param _w The writer to use.
   * @param _size The size of the object.
   * @param _parent The parent object.
   * @return The new object.
   */
  template <class ParentType>
  static OutputObjectType add_object(const W& _w, const size_t _size,
                                     const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (std::is_same<Type, Array>()) {
      return _w.add_object_to_array(_size, _parent.arr_);

    } else if constexpr (is_map_entry<Type>()) {
      return _w.add_object_to_map_entry(_size, _parent.entry_);

    } else if constexpr (std::is_same<Type, Object>()) {
      return _w.add_object_to_object(_parent.name_, _size, _parent.obj_);

    } else if constexpr (std::is_same<Type, Root>()) {
      return _w.object_as_root(_size);

    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      if constexpr (std::is_same<Type, Map<typename W::OutputMapType>>()) {
        return _w.add_object_to_map(_parent.name_, _size, _parent.map_);
      } else if constexpr (std::is_same<Type,
                                        Union<typename W::OutputUnionType>>()) {
        return _w.add_object_to_union(_parent.index_, _size, _parent.union_);
      } else {
        static_assert(always_false_v<Type>, "Unsupported option.");
      }

    } else {
      static_assert(always_false_v<Type>, "Unsupported option.");
    }
  }

  /**
   * @brief Adds a null value to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @param _w The writer to use.
   * @param _parent The parent object.
   * @return The new null value.
   */
  template <class ParentType>
  static OutputVarType add_null(const W& _w, const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (std::is_same<Type, Array>()) {
      return _w.add_null_to_array(_parent.arr_);

    } else if constexpr (is_map_entry<Type>()) {
      return _w.add_null_to_map_entry(_parent.entry_);

    } else if constexpr (std::is_same<Type, Object>()) {
      if constexpr (supports_attributes<std::remove_cvref_t<W>>) {
        return _w.add_null_to_object(_parent.name_, _parent.obj_,
                                     _parent.is_attribute_);
      } else {
        return _w.add_null_to_object(_parent.name_, _parent.obj_);
      }

    } else if constexpr (std::is_same<Type, Root>()) {
      return _w.null_as_root();

    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      if constexpr (std::is_same<Type, Map<typename W::OutputMapType>>()) {
        return _w.add_null_to_map(_parent.name_, _parent.map_);
      } else if constexpr (std::is_same<Type,
                                        Union<typename W::OutputUnionType>>()) {
        return _w.add_null_to_union(_parent.index_, _parent.union_);
      } else {
        static_assert(always_false_v<Type>, "Unsupported option.");
      }

    } else {
      static_assert(always_false_v<Type>, "Unsupported option.");
    }
  }

  /**
   * @brief Adds a union to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @param _w The writer to use.
   * @param _parent The parent object.
   * @return The new union.
   */
  template <class ParentType>
  static auto add_union(const W& _w, const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (std::is_same<Type, Array>()) {
      return _w.add_union_to_array(_parent.arr_);

    } else if constexpr (is_map_entry<Type>()) {
      return _w.add_union_to_map_entry(_parent.entry_);

    } else if constexpr (std::is_same<Type, Map<typename W::OutputMapType>>()) {
      return _w.add_union_to_map(_parent.name_, _parent.map_);

    } else if constexpr (std::is_same<Type, Object>()) {
      return _w.add_union_to_object(_parent.name_, _parent.obj_);

    } else if constexpr (std::is_same<Type,
                                      Union<typename W::OutputUnionType>>()) {
      return _w.add_union_to_union(_parent.index_, _parent.union_);

    } else if constexpr (std::is_same<Type, Root>()) {
      return _w.union_as_root();

    } else {
      static_assert(always_false_v<Type>, "Unsupported option.");
    }
  }

  /**
   * @brief Adds a value to the parent.
   *
   * @tparam ParentType The type of the parent.
   * @tparam T The type of the value.
   * @param _w The writer to use.
   * @param _var The value to add.
   * @param _parent The parent object.
   * @return The new value.
   */
  template <class ParentType, class T>
  static OutputVarType add_value(const W& _w, const T& _var,
                                 const ParentType& _parent) {
    using Type = std::remove_cvref_t<ParentType>;
    if constexpr (std::is_same<Type, Array>()) {
      return _w.add_value_to_array(_var, _parent.arr_);

    } else if constexpr (is_map_entry<Type>()) {
      return _w.add_value_to_map_entry(_var, _parent.entry_);

    } else if constexpr (std::is_same<Type, Object>()) {
      if constexpr (supports_attributes<std::remove_cvref_t<W>>) {
        return _w.add_value_to_object(_parent.name_, _var, _parent.obj_,
                                      _parent.is_attribute_);
      } else {
        return _w.add_value_to_object(_parent.name_, _var, _parent.obj_);
      }

    } else if constexpr (std::is_same<Type, Root>()) {
      return _w.value_as_root(_var);

    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      if constexpr (std::is_same<Type, Map<typename W::OutputMapType>>()) {
        return _w.add_value_to_map(_parent.name_, _var, _parent.map_);
      } else if constexpr (std::is_same<Type,
                                        Union<typename W::OutputUnionType>>()) {
        return _w.add_value_to_union(_parent.index_, _var, _parent.union_);
      } else {
        static_assert(always_false_v<Type>, "Unsupported option.");
      }

    } else {
      static_assert(always_false_v<Type>, "Unsupported option.");
    }
  }

 private:
  template <class T>
  static constexpr bool is_map_entry() {
    if constexpr (requires { typename W::OutputMapEntryType; }) {
      return std::is_same_v<T, MapEntry<typename W::OutputMapEntryType>>;
    } else {
      return false;
    }
  }
};

}  // namespace rfl::parsing

#endif
