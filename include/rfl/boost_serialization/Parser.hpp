#ifndef RFL_BOOST_SERIALIZATION_PARSER_HPP_
#define RFL_BOOST_SERIALIZATION_PARSER_HPP_

#include <array>
#include <bitset>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/bitset.hpp>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>
#include <vector>

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

// Keep bool on the generic path to retain Boost's scalar validity assertions.
// Other archive types (including text archives) keep their generic parser too.
template <class T, std::size_t N, class ProcessorsType>
  requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool> &&
           !std::is_const_v<T> && !std::is_volatile_v<T>)
struct Parser<boost_serialization::Reader<::boost::archive::binary_iarchive>,
              boost_serialization::Writer<::boost::archive::binary_oarchive>,
              std::array<T, N>, ProcessorsType>
    : ParserArray<
          boost_serialization::Reader<::boost::archive::binary_iarchive>,
          boost_serialization::Writer<::boost::archive::binary_oarchive>, T, N,
          ProcessorsType> {
  using R = boost_serialization::Reader<::boost::archive::binary_iarchive>;
  using W = boost_serialization::Writer<::boost::archive::binary_oarchive>;
  using Type = std::array<T, N>;

  static Result<Type> read(const R&,
                           const typename R::InputVarType& _var) noexcept {
    try {
      Type values;
      if constexpr (N != 0) {
        _var.ar->load_binary(values.data(), N * sizeof(T));
      }
      return values;
    } catch (const std::exception& e) {
      return error(std::string("Boost binary array read error: ") + e.what());
    }
  }

  template <class ParentType>
  static void write(const W& _w, const Type& _values,
                    const ParentType& _parent) {
    auto arr = Parent<W>::add_fixed_array(_w, N, _parent);
    if constexpr (N != 0) {
      _w.write_array_data(_values.data(), N);
    }
    _w.end_array(&arr);
  }
};

template <class IArchive, class OArchive, std::size_t N, class ProcessorsType>
struct Parser<boost_serialization::Reader<IArchive>,
              boost_serialization::Writer<OArchive>, std::bitset<N>,
              ProcessorsType>
    : ParserBasicType<boost_serialization::Reader<IArchive>,
                      boost_serialization::Writer<OArchive>, std::bitset<N>,
                      ProcessorsType> {};

template <class T, class ProcessorsType>
  requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool> &&
           !std::is_same_v<T, char>)
struct Parser<boost_serialization::Reader<::boost::archive::binary_iarchive>,
              boost_serialization::Writer<::boost::archive::binary_oarchive>,
              std::vector<T>, ProcessorsType>
    : VectorParser<
          boost_serialization::Reader<::boost::archive::binary_iarchive>,
          boost_serialization::Writer<::boost::archive::binary_oarchive>,
          std::vector<T>, ProcessorsType> {
  using R = boost_serialization::Reader<::boost::archive::binary_iarchive>;
  using W = boost_serialization::Writer<::boost::archive::binary_oarchive>;
  using Type = std::vector<T>;

  static Result<Type> read(const R&,
                           const typename R::InputVarType& _var) noexcept {
    try {
      std::uint64_t size;
      *_var.ar >> size;
      Type values;
      if (size > values.max_size()) {
        return error("Array length exceeds vector max_size");
      }
      values.resize(static_cast<std::size_t>(size));
      if (!values.empty()) {
        _var.ar->load_binary(values.data(), values.size() * sizeof(T));
      }
      return values;
    } catch (const std::exception& e) {
      return error(std::string("Boost binary array read error: ") + e.what());
    }
  }

  template <class ParentType>
  static void write(const W& _w, const Type& _values,
                    const ParentType& _parent) {
    // Keep the adapter's array prefix and parent metadata unchanged.
    auto arr = Parent<W>::add_array(_w, _values.size(), _parent);
    _w.write_array_data(_values.data(), _values.size());
    _w.end_array(&arr);
  }
};

/// Boost.Serialization requires us to explicitly set all fields. Because
/// of that, we require all of the fields and then set them to nullptr, if
/// necessary.
template <class IArchive, class OArchive, class ProcessorsType,
          class... FieldTypes>
  requires AreReaderAndWriter<boost_serialization::Reader<IArchive>,
                              boost_serialization::Writer<OArchive>,
                              NamedTuple<FieldTypes...>>
struct Parser<boost_serialization::Reader<IArchive>,
              boost_serialization::Writer<OArchive>, NamedTuple<FieldTypes...>,
              ProcessorsType>
    : public NamedTupleParser<
          boost_serialization::Reader<IArchive>,
          boost_serialization::Writer<OArchive>,
          /*_ignore_empty_containers=*/false,
          /*_all_required=*/true,
          /*_no_field_names=*/internal::no_field_names_v<ProcessorsType>,
          ProcessorsType, FieldTypes...> {};

template <class IArchive, class OArchive, class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<boost_serialization::Reader<IArchive>,
                              boost_serialization::Writer<OArchive>,
                              rfl::Tuple<Ts...>>
struct Parser<boost_serialization::Reader<IArchive>,
              boost_serialization::Writer<OArchive>, rfl::Tuple<Ts...>,
              ProcessorsType>
    : public TupleParser<boost_serialization::Reader<IArchive>,
                         boost_serialization::Writer<OArchive>,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         rfl::Tuple<Ts...>> {};

template <class IArchive, class OArchive, class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<boost_serialization::Reader<IArchive>,
                              boost_serialization::Writer<OArchive>,
                              std::tuple<Ts...>>
struct Parser<boost_serialization::Reader<IArchive>,
              boost_serialization::Writer<OArchive>, std::tuple<Ts...>,
              ProcessorsType>
    : public TupleParser<boost_serialization::Reader<IArchive>,
                         boost_serialization::Writer<OArchive>,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         std::tuple<Ts...>> {};

template <class IArchive, class OArchive, class T, bool _skip_serialization,
          bool _skip_deserialization, class ProcessorsType>
  requires AreReaderAndWriter<
      boost_serialization::Reader<IArchive>,
      boost_serialization::Writer<OArchive>,
      internal::Skip<T, _skip_serialization, _skip_deserialization>>
struct Parser<boost_serialization::Reader<IArchive>,
              boost_serialization::Writer<OArchive>,
              internal::Skip<T, _skip_serialization, _skip_deserialization>,
              ProcessorsType> {
  using R = boost_serialization::Reader<IArchive>;
  using W = boost_serialization::Writer<OArchive>;

  template <class U>
  static Result<internal::Skip<T, _skip_serialization, _skip_deserialization>>
  read(const R&, const U&) noexcept {
    static_assert(always_false_v<T>,
                  "rfl::Skip is unsupported in Boost.Serialization.");
    return error("Unsupported");
  }

  template <class P>
  static void write(const W& /*_w*/,
                    const internal::Skip<T, _skip_serialization,
                                         _skip_deserialization>& /*_skip*/,
                    const P& /*_parent*/) noexcept {
    static_assert(always_false_v<P>,
                  "rfl::Skip is unsupported in Boost.Serialization.");
  }

  template <class U>
  static schema::Type to_schema(U* /*_definitions*/) {
    static_assert(always_false_v<U>,
                  "rfl::Skip is unsupported in Boost.Serialization.");
    return schema::Type{};
  }
};

}  // namespace parsing
}  // namespace rfl

namespace rfl::boost_serialization {

template <class IArchive, class OArchive, class T, class ProcessorsType>
using Parser =
    parsing::Parser<Reader<IArchive>, Writer<OArchive>, T, ProcessorsType>;

}  // namespace rfl::boost_serialization

#endif
