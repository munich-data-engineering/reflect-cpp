#ifndef RFL_PARSING_PARSER_SKIP_HPP_
#define RFL_PARSING_PARSER_SKIP_HPP_

#include <exception>
#include <map>
#include <type_traits>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../internal/Skip.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

template <class T>
struct is_skip : std::false_type {};

template <class T, bool _skip_serialization, bool _skip_deserialization>
struct is_skip<internal::Skip<T, _skip_serialization, _skip_deserialization>>
    : std::true_type {};

template <class T>
constexpr bool is_skip_v = is_skip<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, bool _skip_serialization,
          bool _skip_deserialization, class ProcessorsType>
struct ParserSkip {
  using InputVarType = typename R::InputVarType;
  using ParentType = Parent<W>;

  /**
   * @brief Reads a value and skips it if required.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed value or an error.
   */
  static Result<internal::Skip<T, _skip_serialization, _skip_deserialization>>
  read(const R& _r, const InputVarType& _var) noexcept {
    if constexpr (_skip_deserialization) {
      // The default value can throw, for instance a Validator whose default
      // value is invalid.
      try {
        return internal::Skip<T, _skip_serialization, _skip_deserialization>(
            std::remove_cvref_t<T>());
      } catch (std::exception& e) {
        return error(e.what());
      }
    } else {
      const auto to_skip = [&](auto&& _t) {
        return internal::Skip<T, _skip_serialization, _skip_deserialization>(
            std::move(_t));
      };
      return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r,
                                                                        _var)
          .transform(to_skip);
    }
  }

  /**
   * @brief Writes a value and skips it if required.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _skip The value to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w,
                    const internal::Skip<T, _skip_serialization,
                                         _skip_deserialization>& _skip,
                    const P& _parent) {
    if constexpr (_skip_serialization) {
      ParentType::add_null(_w, _parent);
    } else {
      Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(
          _w, _skip.value(), _parent);
    }
  }

  /**
   * @brief Generates the schema for the value.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::to_schema(
        _definitions);
  }
};

}  // namespace rfl::parsing

#endif
