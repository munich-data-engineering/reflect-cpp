#ifndef FLEXBUF_PARSER_HPP_
#define FLEXBUF_PARSER_HPP_

#include "../parsing/Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::parsing {

template <std::size_t N, class ProcessorsType>
struct Parser<flexbuf::Reader, flexbuf::Writer, std::bitset<N>, ProcessorsType>
    : ParserBitset<flexbuf::Reader, flexbuf::Writer, N, ProcessorsType,
                   Bytestring> {};

}  // namespace rfl::parsing

namespace rfl {
namespace flexbuf {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}
}  // namespace rfl

#endif
