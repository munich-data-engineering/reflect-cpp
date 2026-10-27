#ifndef RFL_BSON_PARSER_HPP_
#define RFL_BSON_PARSER_HPP_

#include <bson/bson.h>

#include "../parsing/AreReaderAndWriter.hpp"
#include "../parsing/Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::parsing {

template <std::size_t N, class ProcessorsType>
struct Parser<bson::Reader, bson::Writer, std::bitset<N>, ProcessorsType>
    : ParserBitset<bson::Reader, bson::Writer, N, ProcessorsType,
                   Bytestring> {};

}  // namespace rfl::parsing

namespace rfl::parsing {

/// bson_oid_t needs to be treated as a special case, otherwise it will be read
/// as a struct.
template <class R, class W, class ProcessorsType>
  requires AreReaderAndWriter<R, W, bson_oid_t>
struct Parser<R, W, ProcessorsType, bson_oid_t> {
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  static Result<bson_oid_t> read(const R& _r,
                                 const InputVarType& _var) noexcept {
    return _r.template to_basic_type<bson_oid_t>(_var);
  }

  template <class P>
  static void write(const W& _w, const bson_oid_t& _oid,
                    const P& _parent) noexcept {
    ParentType::add_value(_w, _oid, _parent);
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    static_assert(rfl::always_false_v<R>,
                  "bson_oid_t cannot be expressed inside a JSON schema.");
    return schema::Type{schema::Type::String{}};
  }
};

}  // namespace rfl::parsing

namespace rfl::bson {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}  // namespace rfl::bson

#endif
