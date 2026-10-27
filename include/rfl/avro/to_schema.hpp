#ifndef RFL_AVRO_TOSCHEMA_HPP_
#define RFL_AVRO_TOSCHEMA_HPP_

#include <string>

#include "../Processors.hpp"
#include "../common.hpp"
#include "../parsing/schema/make.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "Schema.hpp"
#include "Writer.hpp"

namespace rfl::avro {

/// Converts an internal schema definition to JSON representation.
/// This is used internally to generate Avro schema JSON.
/// @param internal_schema The internal schema definition
/// @return A JSON string representing the Avro schema
RFL_API std::string to_json_representation(
    const parsing::schema::Definition& internal_schema);

/// Generates an Avro schema for the given C++ type.
/// Uses reflection to analyze the type structure and creates a corresponding Avro schema.
/// @tparam T The C++ type to generate a schema for
/// @tparam Ps Optional processors that may affect schema generation
/// @return A Schema object that can be used for reading/writing Avro data
template <class T, class... Ps>
Schema<T> to_schema() noexcept {
  const auto internal_schema =
      parsing::schema::make<Reader, Writer, T, Processors<Ps...>>();
  const auto json_str = to_json_representation(internal_schema);
  return std::move(Schema<T>::from_json(json_str).value());
}
}  // namespace rfl::avro

#endif
