#ifndef RFL_PARSING_PARSER_C_ARRAY_HPP_
#define RFL_PARSING_PARSER_C_ARRAY_HPP_

#include <map>
#include <type_traits>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../internal/Array.hpp"
#include "../internal/to_std_array.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

template <class T>
struct is_c_array : std::false_type {};

template <class T, size_t _size>
struct is_c_array<T[_size]> : std::true_type {
  using element_type = T;
  static constexpr size_t size = _size;
};

template <class T>
inline constexpr bool is_c_array_v = is_c_array<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, size_t _size, class ProcessorsType>
struct ParserCArray {
 public:
  using InputArrayType = typename R::InputArrayType;
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;
  using CArray = T[_size];

  /**
   * @brief Reads a C array from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed C array or an error.
   */
  static Result<internal::Array<CArray>> read(
      const R& _r, const InputVarType& _var) noexcept {
    using StdArray = internal::to_std_array_t<CArray>;
    return Parser<R, W, StdArray, ProcessorsType>::read(_r, _var);
  }

  /**
   * @brief Writes a C array to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _arr The C array to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const CArray& _arr, const P& _parent) {
    auto arr = ParentType::add_fixed_array(_w, _size, _parent);
    const auto new_parent = typename ParentType::Array{&arr};
    for (const auto& e : _arr) {
      Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, e,
                                                                  new_parent);
    }
    _w.end_array(&arr);
  }

  /**
   * @brief Generates the schema for the C array.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    using StdArray = internal::to_std_array_t<CArray>;
    return Parser<R, W, std::remove_cvref_t<StdArray>,
                  ProcessorsType>::to_schema(_definitions);
  }
};

}  // namespace rfl::parsing

#endif
