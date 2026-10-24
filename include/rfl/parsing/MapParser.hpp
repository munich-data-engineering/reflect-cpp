#ifndef RFL_PARSING_MAPPARSER_HPP_
#define RFL_PARSING_MAPPARSER_HPP_

#include <map>
#include <string>
#include <type_traits>

#include "../Ref.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"
#include "MapReader.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "SupportsTypedMapReader.hpp"
#include "SupportsTypedMapWriter.hpp"
#include "TypedMapReader.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "to_single_error_message.hpp"

namespace rfl {
namespace parsing {

template <class R, class W, class MapType, class ProcessorsType>
  requires AreReaderAndWriter<R, W, MapType>
struct MapParser {
 public:
  using InputObjectType = typename R::InputObjectType;
  using InputVarType = typename R::InputVarType;

  using KeyType = std::remove_cvref_t<typename MapType::value_type::first_type>;
  using ValueType =
      std::remove_cvref_t<typename MapType::value_type::second_type>;

  using ParentType = Parent<W>;

  /**
   * @brief Reads a map from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed map or an error.
   */
  static Result<MapType> read(const R& _r, const InputVarType& _var) noexcept {
    const auto to_map = [&](auto obj) -> Result<MapType> {
      return make_map(_r, obj);
    };
    if constexpr (schemaful::IsSchemafulReader<R>) {
      return _r.to_map(_var).and_then(to_map);
    } else {
      return _r.to_object(_var).and_then(to_map);
    }
  }

  /**
   * @brief Writes a map to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _m The map to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const MapType& _m, const P& _parent) {
    if constexpr (SupportsTypedMapWriter<W>) {
      write_typed_map(_w, _m, _parent);
    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      write_map(_w, _m, _parent);
    } else {
      write_object(_w, _m, _parent);
    }
  }

  /**
   * @brief Generates the schema for the map.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return schema::Type{schema::Type::StringMap{Ref<schema::Type>::make(
        Parser<R, W, ValueType, ProcessorsType>::to_schema(_definitions))}};
  }

 private:
  static Result<MapType> make_map(const R& _r, const auto& _obj_or_map) {
    MapType map;
    if constexpr (SupportsTypedMapReader<
                      R, std::remove_cvref_t<decltype(_obj_or_map)>>) {
      const auto reader =
          TypedMapReader<R, W, MapType, ProcessorsType>{&_r, &map};
      if (const auto err = _r.read_typed_map(reader, _obj_or_map)) {
        return error(*err);
      }
      return map;
    } else {
      std::vector<std::string> errors;
      const auto map_reader =
          MapReader<R, W, MapType, ProcessorsType>(&_r, &map, &errors);
      if constexpr (schemaful::IsSchemafulReader<R>) {
        const auto err = _r.read_map(map_reader, _obj_or_map);
        if (err) {
          return error(*err);
        }
      } else {
        const auto err = _r.read_object(map_reader, _obj_or_map);
        if (err) {
          return error(*err);
        }
      }
      if (errors.size() != 0) {
        return error(to_single_error_message(errors));
      }
      return map;
    }
  }

  template <class P>
  static void write_typed_map(const W& _w, const MapType& _m,
                              const P& _parent) {
    const auto write_entries = [&](auto* map) {
      for (const auto& [key, value] : _m) {
        auto entry = _w.map_entry(map);
        const auto parent = typename ParentType::template MapEntry<
            typename W::OutputMapEntryType>{&entry};
        Parser<R, W, KeyType, ProcessorsType>::write(_w, key, parent);
        Parser<R, W, ValueType, ProcessorsType>::write(_w, value, parent);
      }
    };
    if constexpr (schemaful::IsSchemafulWriter<W>) {
      auto map = ParentType::add_map(_w, _m.size(), _parent);
      write_entries(&map);
      _w.end_map(&map);
    } else {
      auto map = ParentType::add_object(_w, _m.size(), _parent);
      write_entries(&map);
      _w.end_object(&map);
    }
  }

  template <class P>
  static void write_map(const W& _w, const MapType& _m, const P& _parent) {
    auto m = ParentType::add_map(_w, _m.size(), _parent);

    using ParentMapType =
        typename ParentType::template Map<typename W::OutputMapType>;

    for (const auto& [k, v] : _m) {
      if constexpr (internal::has_reflection_type_v<KeyType>) {
        using ReflT = typename KeyType::ReflectionType;

        if constexpr (std::is_integral_v<ReflT> ||
                      std::is_floating_point_v<ReflT>) {
          const auto name = std::to_string(k.reflection());
          const auto new_parent = ParentMapType{name, &m};
          Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
              _w, v, new_parent);
        } else {
          const auto name = k.reflection();
          const auto new_parent = ParentMapType{name, &m};
          Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
              _w, v, new_parent);
        }

      } else if constexpr (std::is_integral_v<KeyType> ||
                           std::is_floating_point_v<KeyType>) {
        const auto name = std::to_string(k);
        const auto new_parent = ParentMapType{name, &m};
        Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
            _w, v, new_parent);
      } else {
        const auto new_parent = ParentMapType{k, &m};
        Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
            _w, v, new_parent);
      }
    }
    _w.end_map(&m);
  }

  template <class P>
  static void write_object(const W& _w, const MapType& _m, const P& _parent) {
    auto obj = ParentType::add_object(_w, _m.size(), _parent);
    for (const auto& [k, v] : _m) {
      if constexpr (internal::has_reflection_type_v<KeyType>) {
        using ReflT = typename KeyType::ReflectionType;

        if constexpr (std::is_integral_v<ReflT> ||
                      std::is_floating_point_v<ReflT>) {
          const auto name = std::to_string(k.reflection());
          const auto new_parent = typename ParentType::Object{name, &obj};
          Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
              _w, v, new_parent);
        } else {
          const auto name = k.reflection();
          const auto new_parent = typename ParentType::Object{name, &obj};
          Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
              _w, v, new_parent);
        }

      } else if constexpr (std::is_integral_v<KeyType> ||
                           std::is_floating_point_v<KeyType>) {
        const auto name = std::to_string(k);
        const auto new_parent = typename ParentType::Object{name, &obj};
        Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
            _w, v, new_parent);
      } else {
        const auto new_parent = typename ParentType::Object{k, &obj};
        Parser<R, W, std::remove_cvref_t<ValueType>, ProcessorsType>::write(
            _w, v, new_parent);
      }
    }
    _w.end_object(&obj);
  }
};

}  // namespace parsing
}  // namespace rfl

#endif
