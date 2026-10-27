#ifndef RFL_CEREAL_PARSER_HPP_
#define RFL_CEREAL_PARSER_HPP_

#include <array>
#include <bitset>
#include <cereal/types/bitset.hpp>
#include <cstddef>
#include <exception>
#include <string>
#include <type_traits>
#include <vector>

#include "../internal/no_field_names_v.hpp"
#include "../parsing/Parser.hpp"
#include "Reader.hpp"
#include "Writer.hpp"

namespace rfl {
namespace parsing {

template <class T, std::size_t N, class ProcessorsType>
  requires(std::is_arithmetic_v<T> && !std::is_const_v<T> &&
           !std::is_volatile_v<T>)
struct Parser<cereal::Reader, cereal::Writer, std::array<T, N>, ProcessorsType>
    : ParserArray<cereal::Reader, cereal::Writer, T, N, ProcessorsType> {
  using R = cereal::Reader;
  using W = cereal::Writer;
  using Type = std::array<T, N>;

  static Result<Type> read(const R&,
                           const typename R::InputVarType& _var) noexcept {
    try {
      Type values;
      if constexpr (N != 0) {
        // Keep the typed pointer: portable binary swaps each T separately.
        (*_var.archive_)(::cereal::binary_data(values.data(), N * sizeof(T)));
      }
      return values;
    } catch (const std::exception& e) {
      return error(std::string("Cereal array read error: ") + e.what());
    }
  }

  template <class ParentType>
  static void write(const W& _w, const Type& _values,
                    const ParentType& _parent) {
    // Even empty arrays need parent metadata, but never a length prefix.
    auto arr = Parent<W>::add_fixed_array(_w, N, _parent);
    if constexpr (N != 0) {
      // Cereal encodes arithmetic scalars (including bool and character types)
      // as sizeof(T) bytes, just like this contiguous std::array payload.
      _w.write_array_data(_values.data(), N);
    }
    _w.end_array(&arr);
  }
};

// Delegate to Cereal's tagged, packed encoding through the reader and writer,
// retaining normal error handling and parent metadata.
template <std::size_t N, class ProcessorsType>
struct Parser<cereal::Reader, cereal::Writer, std::bitset<N>, ProcessorsType>
    : ParserBasicType<cereal::Reader, cereal::Writer, std::bitset<N>,
                      ProcessorsType> {};

template <class T, class ProcessorsType>
  requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool> &&
           !std::is_same_v<T, char>)
struct Parser<cereal::Reader, cereal::Writer, std::vector<T>, ProcessorsType>
    : VectorParser<cereal::Reader, cereal::Writer, std::vector<T>,
                   ProcessorsType> {
  using R = cereal::Reader;
  using W = cereal::Writer;
  using Type = std::vector<T>;

  static Result<Type> read(const R&,
                           const typename R::InputVarType& _var) noexcept {
    try {
      std::size_t size;
      (*_var.archive_)(::cereal::make_size_tag(size));
      Type values(size);
      if (!values.empty()) {
        (*_var.archive_)(
            ::cereal::binary_data(values.data(), values.size() * sizeof(T)));
      }
      return values;
    } catch (const std::exception& e) {
      return error(std::string("Cereal array read error: ") + e.what());
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

/// Cereal requires us to explicitly set the number of fields in advance.
/// Because of that, we require all of the fields and then set them to nullptr,
/// if necessary.
template <class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<cereal::Reader, cereal::Writer,
                              NamedTuple<FieldTypes...>>
struct Parser<cereal::Reader, cereal::Writer, NamedTuple<FieldTypes...>,
              ProcessorsType>
    : public NamedTupleParser<
          cereal::Reader, cereal::Writer,
          /*_ignore_empty_containers=*/false,
          /*_all_required=*/true,
          /*_no_field_names=*/internal::no_field_names_v<ProcessorsType>,
          ProcessorsType, FieldTypes...> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<cereal::Reader, cereal::Writer, rfl::Tuple<Ts...>>
struct Parser<cereal::Reader, cereal::Writer, rfl::Tuple<Ts...>, ProcessorsType>
    : public TupleParser<cereal::Reader, cereal::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         rfl::Tuple<Ts...>> {};

template <class ProcessorsType, class... Ts>
  requires AreReaderAndWriter<cereal::Reader, cereal::Writer, std::tuple<Ts...>>
struct Parser<cereal::Reader, cereal::Writer, std::tuple<Ts...>, ProcessorsType>
    : public TupleParser<cereal::Reader, cereal::Writer,
                         /*_ignore_empty_containers=*/false,
                         /*_all_required=*/true, ProcessorsType,
                         std::tuple<Ts...>> {};

}  // namespace parsing
}  // namespace rfl

namespace rfl {
namespace cereal {

template <class T, class ProcessorsType>
using Parser = parsing::Parser<Reader, Writer, T, ProcessorsType>;

}
}  // namespace rfl

#endif
