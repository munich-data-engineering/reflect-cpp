#ifndef RFL_PARSING_SUPPORTSTYPEDMAPWRITER_HPP_
#define RFL_PARSING_SUPPORTSTYPEDMAPWRITER_HPP_

#include <concepts>
#include <cstddef>

#include "schemaful/IsSchemafulWriter.hpp"

namespace rfl::parsing {

/// A writer with an explicit context for the key and value of a map entry.
template <class W>
concept SupportsTypedMapWriter =
    requires(const W& w, typename W::OutputMapEntryType* entry,
             std::size_t size) {
      {
        w.add_array_to_map_entry(size, entry)
      } -> std::same_as<typename W::OutputArrayType>;
      {
        w.add_object_to_map_entry(size, entry)
      } -> std::same_as<typename W::OutputObjectType>;
      {
        w.add_value_to_map_entry(0, entry)
      } -> std::same_as<typename W::OutputVarType>;
      {
        w.add_null_to_map_entry(entry)
      } -> std::same_as<typename W::OutputVarType>;
    } &&
    ((schemaful::IsSchemafulWriter<W> &&
      requires(const W& w, typename W::OutputMapType* map,
               typename W::OutputMapEntryType* entry, std::size_t size) {
        { w.map_entry(map) } -> std::same_as<typename W::OutputMapEntryType>;
        {
          w.add_map_to_map_entry(size, entry)
        } -> std::same_as<typename W::OutputMapType>;
        {
          w.add_union_to_map_entry(entry)
        } -> std::same_as<typename W::OutputUnionType>;
      }) ||
     (!schemaful::IsSchemafulWriter<W> &&
      requires(const W& w, typename W::OutputObjectType* map) {
        { w.map_entry(map) } -> std::same_as<typename W::OutputMapEntryType>;
      }));

}  // namespace rfl::parsing

#endif
