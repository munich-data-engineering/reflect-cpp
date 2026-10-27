#ifndef RFL_PARSING_PARSER_BITSET_HPP_
#define RFL_PARSING_PARSER_BITSET_HPP_

#include <bitset>
#include <cstddef>
#include <map>
#include <string>
#include <type_traits>

#include "../Result.hpp"
#include "../internal/bitset.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

// Text formats use strings; binary formats can select Bytestring. Both
// representations use the normal parser path to retain parent metadata.
template <class R, class W, std::size_t N, class ProcessorsType,
          class Representation = std::string>
struct ParserBitset {
  using Type = std::bitset<N>;
  using ReprParser = Parser<R, W, Representation, ProcessorsType>;

  static Result<Type> read(const R& _r,
                           const typename R::InputVarType& _var) noexcept {
    return ReprParser::read(_r, _var).and_then([](const auto& _value) {
      if constexpr (std::is_same_v<Representation, std::string>) {
        return internal::bitset_from_string<N>(_value);
      } else {
        return internal::bitset_from_bytes<N>(_value);
      }
    });
  }

  template <class P>
  static void write(const W& _w, const Type& _bits, const P& _parent) {
    if constexpr (std::is_same_v<Representation, std::string>) {
      ReprParser::write(_w, _bits.to_string(), _parent);
    } else {
      ReprParser::write(_w, internal::bitset_to_bytes<Representation>(_bits),
                        _parent);
    }
  }

  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    if constexpr (std::is_same_v<Representation, std::string>) {
      // Unlike $, the final assertion also rejects a trailing newline.
      return schema::Type{schema::Type::Validated{
          .type_ = Ref<schema::Type>::make(schema::Type::String{}),
          .validation_ = schema::ValidationType{schema::ValidationType::Regex{
              "^[01]{" + std::to_string(N) + "}(?![\\s\\S])"}}}};
    } else {
      return ReprParser::to_schema(_definitions);
    }
  }
};

template <class R, class W, std::size_t N, class ProcessorsType>
struct Parser<R, W, std::bitset<N>, ProcessorsType>
    : ParserBitset<R, W, N, ProcessorsType> {};

}  // namespace rfl::parsing

#endif
