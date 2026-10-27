#ifndef RFL_CAPNPROTO_PARSER_HPP_
#define RFL_CAPNPROTO_PARSER_HPP_

#include "../Generic.hpp"
#include "../NamedTuple.hpp"
#include "../Tuple.hpp"
#include "../always_false.hpp"
#include "../internal/no_field_names_v.hpp"
#include "../parsing/Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl {
namespace parsing {

template <std::size_t N, class ProcessorsType>
struct Parser<capnproto::Reader, capnproto::Writer, std::bitset<N>, ProcessorsType>
    : ParserBitset<capnproto::Reader, capnproto::Writer, N, ProcessorsType,
                   Bytestring> {};

/// Cap'n proto requires us to explicitly set all fields. Because
/// of that, we require all of the fields and then set them to nullptr, if
/// necessary.
template <class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<capnproto::Reader, capnproto::Writer,
                              NamedTuple<FieldTypes...>>
struct Parser<capnproto::Reader, capnproto::Writer, NamedTuple<FieldTypes...>,
              ProcessorsType>
    : public NamedTupleParser<
          capnproto::Reader, capnproto::Writer,
          /*_ignore_empty_containers=*/false,
          /*_all_required=*/true,
          /*_no_field_names=*/internal::no_field_names_v<ProcessorsType>,
          ProcessorsType, FieldTypes...> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<capnproto::Reader, capnproto::Writer,
                              rfl::Tuple<Ts...>>
struct Parser<capnproto::Reader, capnproto::Writer, rfl::Tuple<Ts...>,
              ProcessorsType>
    : public TupleParser<capnproto::Reader, capnproto::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         rfl::Tuple<Ts...>> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<capnproto::Reader, capnproto::Writer,
                              std::tuple<Ts...>>
struct Parser<capnproto::Reader, capnproto::Writer, std::tuple<Ts...>,
              ProcessorsType>
    : public TupleParser<capnproto::Reader, capnproto::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         std::tuple<Ts...>> {};

template <class ProcessorsType>
  requires AreReaderAndWriter<capnproto::Reader, capnproto::Writer, Generic>
struct Parser<capnproto::Reader, capnproto::Writer, Generic, ProcessorsType> {
  template <class T>
  static Result<Generic> read(const capnproto::Reader&, const T&) noexcept {
    static_assert(always_false_v<T>,
                  "Generics are unsupported in Cap'n Proto.");
    return error("Unsupported");
  }

  template <class P>
  static void write(const capnproto::Writer&, const Generic&,
                    const P&) noexcept {
    static_assert(always_false_v<P>,
                  "Generics are unsupported in Cap'n Proto.");
  }

  template <class T>
  static schema::Type to_schema(T*) {
    static_assert(always_false_v<T>,
                  "Generics are unsupported in Cap'n Proto.");
    return schema::Type{};
  }
};

template <class T, bool _skip_serialization, bool _skip_deserialization,
          class ProcessorsType>
  requires AreReaderAndWriter<
      capnproto::Reader, capnproto::Writer,
      internal::Skip<T, _skip_serialization, _skip_deserialization>>
struct Parser<capnproto::Reader, capnproto::Writer,
              internal::Skip<T, _skip_serialization, _skip_deserialization>,
              ProcessorsType> {
  using R = capnproto::Reader;
  using W = capnproto::Writer;

  template <class U>
  static Result<internal::Skip<T, _skip_serialization, _skip_deserialization>>
  read(const R&, const U&) noexcept {
    static_assert(always_false_v<T>,
                  "rfl::Skip is unsupported in Cap'n Proto.");
    return error("Unsupported");
  }

  template <class P>
  static void write(const W& /*_w*/,
                    const internal::Skip<T, _skip_serialization,
                                         _skip_deserialization>& /*_skip*/,
                    const P& /*_parent*/) noexcept {
    static_assert(always_false_v<P>,
                  "rfl::Skip is unsupported in Cap'n Proto.");
  }

  template <class U>
  static schema::Type to_schema(U* /*_definitions*/) {
    static_assert(always_false_v<U>,
                  "rfl::Skip is unsupported in Cap'n Proto.");
    return schema::Type{};
  }
};

}  // namespace parsing
}  // namespace rfl

namespace rfl {
namespace capnproto {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}
}  // namespace rfl

#endif
