#ifndef RFL_PARSING_PARSER_DEFAULTVAL_HPP_
#define RFL_PARSING_PARSER_DEFAULTVAL_HPP_

#include <map>
#include <type_traits>

#include "../DefaultVal.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

template <class T>
struct is_default_val : std::false_type {};

template <class T>
struct is_default_val<DefaultVal<T>> : std::true_type {
  using element_type = T;
};

template <class T>
constexpr bool is_default_val_v = is_default_val<std::remove_cvref_t<T>>::value;

template <class R, class W, class DefVal, class ProcessorsType>
struct ParserDefaultVal {
  using InputVarType = typename R::InputVarType;
  using T = typename DefVal::Type;

  using ParentType = Parent<W>;

  /**
   * @brief Reads a DefaultVal from the input.
   * @param _r The reader to use.
   * @param _var The input variable to read.
   * @return A Result containing the parsed DefaultVal or an error.
   */
  static Result<DefaultVal<T>> read(const R& _r,
                                    const InputVarType& _var) noexcept {
    return Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::read(_r, _var)
        .transform([](auto&& _t) {
          return DefaultVal<T>(std::forward<decltype(_t)>(_t));
        });
  }

  /**
   * @brief Writes a DefaultVal to the output.
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _d The DefaultVal to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const DefaultVal<T>& _d, const P& _parent) {
    Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, _d.value(),
                                                                _parent);
  }

  /**
   * @brief Generates the schema for DefaultVal.
   * @param _definitions The map of definitions to add to.
   * @return The schema type for DefaultVal.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    using U = std::remove_cvref_t<T>;
    return schema::Type{schema::Type::DefaultVal{
        .type_ = Ref<schema::Type>::make(
            Parser<R, W, U, ProcessorsType>::to_schema(_definitions)),
        .default_value_ = std::nullopt}};
  }
};

}  // namespace rfl::parsing

#endif
