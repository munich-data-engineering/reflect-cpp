#ifndef RFL_CAPNPROTO_TOSCHEMA_HPP_
#define RFL_CAPNPROTO_TOSCHEMA_HPP_

#include <string>

#include "../Literal.hpp"
#include "../Processors.hpp"
#include "../Result.hpp"
#include "../SnakeCaseToCamelCase.hpp"
#include "../Variant.hpp"
#include "../common.hpp"
#include "../json.hpp"
#include "../parsing/schema/Type.hpp"
#include "../parsing/schema/ValidationType.hpp"
#include "../parsing/schema/make.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "Schema.hpp"
#include "Writer.hpp"
#include "schema/Type.hpp"

namespace rfl::capnproto {

RFL_API std::string to_string_representation(
    const parsing::schema::Definition& internal_schema);

/// This ensures that the schema is only generated once.
template <class T, class... Ps>
struct SchemaHolder {
  static SchemaHolder<T, Ps...> make() noexcept {
    const auto internal_schema =
        parsing::schema::make<Reader, Writer, T,
                              Processors<SnakeCaseToCamelCase, Ps...>>();
    const auto str = to_string_representation(internal_schema);
    return SchemaHolder<T, Ps...>{Schema<T>::from_string(str)};
  }

  rfl::Result<Schema<T>> schema_;
};

template <class T, class... Ps>
static const SchemaHolder<T, Ps...> schema_holder =
    SchemaHolder<T, Ps...>::make();

/// Returns the Cap'n Proto schema for a class.
template <class T, class... Ps>
Schema<T> to_schema() {
  return schema_holder<T, Ps...>.schema_.value();
}
}  // namespace rfl::capnproto

#endif
