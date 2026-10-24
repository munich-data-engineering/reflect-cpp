#ifndef RFL_PARSING_PARSER_ARRAY_HPP_
#define RFL_PARSING_PARSER_ARRAY_HPP_

#include <array>
#include <map>
#include <optional>
#include <type_traits>

#include "../Ref.hpp"
#include "../Result.hpp"
#include "../always_false.hpp"
#include "../internal/ptr_cast.hpp"
#include "ArrayReader.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "call_destructors_on_array_where_necessary.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

template <class T>
struct is_array : std::false_type {};

template <class T, size_t _size>
struct is_array<std::array<T, _size>> : std::true_type {
  using element_type = T;
  static constexpr size_t size = _size;
};

template <class T>
constexpr bool is_array_v = is_array<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, size_t _size, class ProcessorsType>
struct ParserArray {
 public:
  using InputArrayType = typename R::InputArrayType;
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  /**
   * @brief Reads a std::array from the input.
   * @param _r The reader to use.
   * @param _var The input variable to read.
   * @return A Result containing the parsed array or an error.
   */
  static Result<std::array<T, _size>> read(const R& _r,
                                           const InputVarType& _var) noexcept {
    alignas(
        std::array<T, _size>) unsigned char buf[sizeof(std::array<T, _size>)];
    auto ptr = internal::ptr_cast<std::array<T, _size>*>(&buf);
    const auto array_reader =
        ArrayReader<R, W, ProcessorsType, T, _size>(&_r, ptr);
    auto err = [&]() -> std::optional<Error> {
      if constexpr (requires {
                      _r.template read_fixed_array<_size>(array_reader, _var);
                    }) {
        return _r.template read_fixed_array<_size>(array_reader, _var);
      } else {
        const auto arr = _r.to_array(_var);
        if (!arr) {
          return arr.error();
        }
        return _r.read_array(array_reader, *arr);
      }
    }();
    if (err) {
      call_destructors_on_array_where_necessary(array_reader.num_set(), ptr);
      return error(*err);
    }
    err = array_reader.check_size();
    if (err) {
      call_destructors_on_array_where_necessary(array_reader.num_set(), ptr);
      return error(*err);
    }
    auto result = Result<std::array<T, _size>>(std::move(*ptr));
    call_destructors_on_array_where_necessary(array_reader.num_set(), ptr);
    return result;
  }

  /**
   * @brief Writes a std::array to the output.
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _arr The array to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const std::array<T, _size>& _arr,
                    const P& _parent) {
    auto arr = ParentType::add_fixed_array(_w, _size, _parent);
    const auto new_parent = typename ParentType::Array{&arr};
    for (const auto& e : _arr) {
      Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, e,
                                                                  new_parent);
    }
    _w.end_array(&arr);
  }

  /**
   * @brief Generates the schema for std::array.
   * @param _definitions The map of definitions to add to.
   * @return The schema type for std::array.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    using U = std::remove_cvref_t<T>;
    return schema::Type{schema::Type::FixedSizeTypedArray{
        .size_ = _size,
        .type_ = Ref<schema::Type>::make(
            Parser<R, W, U, ProcessorsType>::to_schema(_definitions))}};
  }
};

}  // namespace rfl::parsing

#endif
