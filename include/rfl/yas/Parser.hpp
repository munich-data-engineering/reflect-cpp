#ifndef RFL_YAS_PARSER_HPP_
#define RFL_YAS_PARSER_HPP_

#include <array>
#include <bitset>
#include <cstddef>
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

template <class T, std::size_t N, class ProcessorsType>
  requires((std::is_integral_v<T> || std::is_same_v<T, float> ||
            std::is_same_v<T, double>) &&
           !std::is_const_v<T> && !std::is_volatile_v<T>)
struct Parser<yas::Reader, yas::Writer, std::array<T, N>, ProcessorsType>
    : ParserArray<yas::Reader, yas::Writer, T, N, ProcessorsType> {
  using R = yas::Reader;
  using W = yas::Writer;
  using Type = std::array<T, N>;

  static Result<Type> read(const R&,
                           const typename R::InputVarType& _var) noexcept {
    try {
      Type values;
      if constexpr (N != 0) {
        // This adapter uses YAS's non-compacted, host-endian binary archive.
        _var.ar->read(values.data(), N * sizeof(T));
      }
      return values;
    } catch (const std::exception& e) {
      return error(std::string("yas array read error: ") + e.what());
    }
  }

  template <class ParentType>
  static void write(const W& _w, const Type& _values,
                    const ParentType& _parent) {
    auto arr = Parent<W>::add_fixed_array(_w, N, _parent);
    if constexpr (N != 0) {
      // bool and character scalars also use their native representation.
      arr.ar->write(_values.data(), N * sizeof(T));
    }
    _w.end_array(&arr);
  }
};

template <std::size_t N, class ProcessorsType>
struct Parser<yas::Reader, yas::Writer, std::bitset<N>, ProcessorsType>
    : ParserBasicType<yas::Reader, yas::Writer, std::bitset<N>, ProcessorsType> {};

template <class T, class ProcessorsType>
  requires((std::is_integral_v<T> || std::is_same_v<T, float> ||
            std::is_same_v<T, double>) &&
           !std::is_same_v<T, bool> && !std::is_same_v<T, char>)
struct Parser<yas::Reader, yas::Writer, std::vector<T>, ProcessorsType>
    : VectorParser<yas::Reader, yas::Writer, std::vector<T>, ProcessorsType> {
  using R = yas::Reader;
  using W = yas::Writer;
  using Type = std::vector<T>;

  static Result<Type> read(const R&,
                           const typename R::InputVarType& _var) noexcept {
    try {
      std::size_t size;
      (*_var.ar) & size;
      Type values(size);
      if (!values.empty()) {
        _var.ar->read(values.data(), values.size() * sizeof(T));
      }
      return values;
    } catch (const std::exception& e) {
      return error(std::string("yas array read error: ") + e.what());
    }
  }

  template <class ParentType>
  static void write(const W& _w, const Type& _values,
                    const ParentType& _parent) {
    // Keep the adapter's array prefix and parent metadata unchanged.
    auto arr = Parent<W>::add_array(_w, _values.size(), _parent);
    if (!_values.empty()) {
      arr.ar->write(_values.data(), _values.size() * sizeof(T));
    }
    _w.end_array(&arr);
  }
};

/// Yas requires us to explicitly set the number of fields in advance.
/// Because of that, we require all of the fields and then set them to nullptr,
/// if necessary.
template <class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<yas::Reader, yas::Writer,
                              NamedTuple<FieldTypes...>>
struct Parser<yas::Reader, yas::Writer, NamedTuple<FieldTypes...>,
              ProcessorsType>
    : public NamedTupleParser<
          yas::Reader, yas::Writer,
          /*_ignore_empty_containers=*/false,
          /*_all_required=*/true,
          /*_no_field_names=*/internal::no_field_names_v<ProcessorsType>,
          ProcessorsType, FieldTypes...> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<yas::Reader, yas::Writer, rfl::Tuple<Ts...>>
struct Parser<yas::Reader, yas::Writer, rfl::Tuple<Ts...>, ProcessorsType>
    : public TupleParser<yas::Reader, yas::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         rfl::Tuple<Ts...>> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<yas::Reader, yas::Writer, std::tuple<Ts...>>
struct Parser<yas::Reader, yas::Writer, std::tuple<Ts...>, ProcessorsType>
    : public TupleParser<yas::Reader, yas::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         std::tuple<Ts...>> {};

}  // namespace parsing
}  // namespace rfl

namespace rfl {
namespace yas {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}
}  // namespace rfl

#endif
