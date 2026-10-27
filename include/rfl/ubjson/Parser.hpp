#ifndef RFL_UBJSON_PARSER_HPP_
#define RFL_UBJSON_PARSER_HPP_

#include "../Tuple.hpp"
#include "../internal/no_field_names_v.hpp"
#include "../parsing/Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl::parsing {

template <std::size_t N, class ProcessorsType>
struct Parser<ubjson::Reader, ubjson::Writer, std::bitset<N>, ProcessorsType>
    : ParserBitset<ubjson::Reader, ubjson::Writer, N, ProcessorsType,
                   Bytestring> {};

/// UBJSON requires us to explicitly set the number of fields in advance.
/// Because of that, we require all of the fields and then set them to nullptr,
/// if necessary.
template <class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<ubjson::Reader, ubjson::Writer,
                              NamedTuple<FieldTypes...>>
struct Parser<ubjson::Reader, ubjson::Writer, NamedTuple<FieldTypes...>,
              ProcessorsType>
    : public NamedTupleParser<
          ubjson::Reader, ubjson::Writer,
          /*_ignore_empty_containers=*/false,
          /*_all_required=*/true,
          /*_no_field_names=*/internal::no_field_names_v<ProcessorsType>,
          ProcessorsType, FieldTypes...> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<ubjson::Reader, ubjson::Writer, rfl::Tuple<Ts...>>
struct Parser<ubjson::Reader, ubjson::Writer, rfl::Tuple<Ts...>, ProcessorsType>
    : public TupleParser<ubjson::Reader, ubjson::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         rfl::Tuple<Ts...>> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<ubjson::Reader, ubjson::Writer, std::tuple<Ts...>>
struct Parser<ubjson::Reader, ubjson::Writer, std::tuple<Ts...>, ProcessorsType>
    : public TupleParser<ubjson::Reader, ubjson::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         std::tuple<Ts...>> {};

}  // namespace rfl::parsing

namespace rfl::ubjson {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}  // namespace rfl::ubjson

#endif
