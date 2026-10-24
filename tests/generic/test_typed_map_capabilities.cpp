#include <optional>
#include <rfl/parsing/SupportsTypedMapReader.hpp>
#include <rfl/parsing/schemaful/IsSchemafulReader.hpp>

namespace test_typed_map_capabilities {

// A schemaful backend must be able to provide either map API independently.
struct ReaderBase {
  struct InputVarType {};
  struct InputObjectType {};
  struct InputMapType {};
  struct InputUnionType {};

  template <class ObjectReader>
  std::optional<rfl::Error> read_object(const ObjectReader&,
                                        const InputObjectType&) const;
  template <class T, class UnionReader>
  rfl::Result<T> read_union(const InputUnionType&) const;
  rfl::Result<InputMapType> to_map(const InputVarType&) const;
  rfl::Result<InputUnionType> to_union(const InputVarType&) const;
};

struct StringKeyReader : ReaderBase {
  template <class MapReader>
  std::optional<rfl::Error> read_map(const MapReader&,
                                     const InputMapType&) const;
};

struct TypedKeyReader : ReaderBase {
  template <class MapReader>
  std::optional<rfl::Error> read_typed_map(const MapReader&,
                                           const InputMapType&) const;
};

struct InvalidTypedKeyReader : ReaderBase {
  template <class MapReader>
  bool read_typed_map(const MapReader&, const InputMapType&) const;
};

static_assert(rfl::parsing::schemaful::IsSchemafulReader<StringKeyReader>);
static_assert(rfl::parsing::schemaful::IsSchemafulReader<TypedKeyReader>);
static_assert(!rfl::parsing::schemaful::IsSchemafulReader<ReaderBase>);
static_assert(
    !rfl::parsing::schemaful::IsSchemafulReader<InvalidTypedKeyReader>);
static_assert(rfl::parsing::SupportsTypedMapReader<TypedKeyReader,
                                                   ReaderBase::InputMapType>);
static_assert(!rfl::parsing::SupportsTypedMapReader<StringKeyReader,
                                                    ReaderBase::InputMapType>);

}  // namespace test_typed_map_capabilities
