#ifndef RFL_PARSING_SCHEMAFUL_ISSCHEMAFULREADER_HPP_
#define RFL_PARSING_SCHEMAFUL_ISSCHEMAFULREADER_HPP_

#include <concepts>
#include <optional>
#include <string>
#include <variant>

#include "../../Result.hpp"
#include "../SupportsTypedMapReader.hpp"

namespace rfl::parsing::schemaful {

using MockVariantType = std::variant<std::string, int>;

/**
 * @brief A mock reader for maps.
 *
 * @tparam R The reader type.
 */
template <class R>
struct MockMapReader {
  void read(const std::string_view&, typename R::InputVarType&) const {}
};

/**
 * @brief A mock reader for objects.
 *
 * @tparam R The reader type.
 */
template <class R>
struct MockObjectReader {
  void read(const int, typename R::InputVarType&) const {}
};

/**
 * @brief A mock reader for unions.
 *
 * @tparam R The reader type.
 */
template <class R>
struct MockUnionReader {
  static rfl::Result<MockVariantType> read(const R&, const size_t,
                                           typename R::InputVarType&) {
    return error("This is a mock type.");
  }
};

/**
 * @brief Concept for a schemaful reader.
 *
 * @tparam R The reader type.
 */
template <class R>
concept IsSchemafulReader =
    requires(R r, typename R::InputVarType var, typename R::InputObjectType obj,
             typename R::InputMapType m, typename R::InputUnionType u,
             MockMapReader<R> map_reader, MockObjectReader<R> object_reader) {
      /// A schemaful reader needs to differentiate between objects, for which
      /// the field names are known at compile time and maps, for which the
      /// field names are not known at compile time.
      requires SupportsTypedMapReader<R, typename R::InputMapType> || requires {
        { r.read_map(map_reader, m) } -> std::same_as<std::optional<Error>>;
      };

      /// A schemaful reader can read fields by order and does not have to
      /// compare strings - the correct order of the fields is guaranteed by the
      /// schema.
      {
        r.read_object(object_reader, obj)
      } -> std::same_as<std::optional<Error>>;

      /// A schemaful reader needs an explicit union type.
      {
        r.template read_union<MockVariantType, MockUnionReader<R>>(u)
      } -> std::same_as<rfl::Result<MockVariantType>>;

      /// It needs to be possible to transform variables to maps.
      { r.to_map(var) } -> std::same_as<rfl::Result<typename R::InputMapType>>;

      /// It needs to be possible to transform variables to unions.
      {
        r.to_union(var)
      } -> std::same_as<rfl::Result<typename R::InputUnionType>>;
    };

}  // namespace rfl::parsing::schemaful

#endif
