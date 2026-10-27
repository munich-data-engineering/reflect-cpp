#ifndef RFL_CBOR_PARSER_HPP_
#define RFL_CBOR_PARSER_HPP_

#include "../parsing/Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::parsing {

template <std::size_t N, class ProcessorsType>
struct Parser<cbor::Reader, cbor::Writer, std::bitset<N>, ProcessorsType>
    : ParserBasicType<cbor::Reader, cbor::Writer, std::bitset<N>,
                      ProcessorsType> {};

}  // namespace rfl::parsing

namespace rfl::cbor {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}  // namespace rfl::cbor

#endif
