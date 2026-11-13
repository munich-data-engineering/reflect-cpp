#ifndef RFL_PARSING_VECTORPARSER_HPP_
#define RFL_PARSING_VECTORPARSER_HPP_

#include <iterator>
#include <map>
#include <string>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "MapParser.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "StringMapKey.hpp"
#include "VectorReader.hpp"
#include "is_forward_list.hpp"
#include "is_map_like.hpp"
#include "is_map_like_not_multimap.hpp"
#include "is_set_like.hpp"
#include "schema/Type.hpp"

namespace rfl {
namespace parsing {

/// This can be used for data structures that would be expressed as array in
/// serialized format (std::vector, std::set, std::deque, ...),
/// but also includes map-like types, when the key is not of type
/// std::string.
template <class R, class W, class VecType, class ProcessorsType>
  requires AreReaderAndWriter<R, W, VecType>
struct VectorParser {
 public:
  using InputArrayType = typename R::InputArrayType;
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  using T = typename VecType::value_type;

  static_assert(!std::is_same_v<std::remove_cvref_t<VecType>, Bytestring>,
                "Cannot be a bytestring.");
  static_assert(!std::is_same_v<std::remove_cvref_t<VecType>, Vectorstring>,
                "Cannot be a vectorstring.");

  /**
   * @brief Reads a vector-like type from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed type or an error.
   */
  static Result<VecType> read(const R& _r, const InputVarType& _var) noexcept {
    if constexpr (treat_as_map()) {
      return MapParser<R, W, VecType, ProcessorsType>::read(_r, _var);
    } else if constexpr (is_forward_list<VecType>()) {
      const auto to_forward_list = [](auto&& vec) -> std::forward_list<T> {
        std::forward_list<T> list;
        for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
          list.emplace_front(std::move(*it));
        }
        return list;
      };
      return Parser<R, W, std::vector<T>, ProcessorsType>::read(_r, _var)
          .transform(to_forward_list);
    } else {
      const auto parse = [&](const InputArrayType& _arr) -> Result<VecType> {
        VecType vec;
        auto vector_reader =
            VectorReader<R, W, VecType, ProcessorsType>(&_r, &vec);
        auto err = _r.read_array(vector_reader, _arr);
        if (err) {
          return error(std::move(*err));
        }
        return vec;
      };
      return _r.to_array(_var).and_then(parse);
    }
  }

  /**
   * @brief Writes a vector-like type to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _vec The vector-like type to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const VecType& _vec, const P& _parent) {
    if constexpr (treat_as_map()) {
      MapParser<R, W, VecType, ProcessorsType>::write(_w, _vec, _parent);
    } else {
      auto arr = ParentType::add_array(
          _w, std::distance(_vec.begin(), _vec.end()), _parent);
      const auto new_parent = typename ParentType::Array{&arr};
      for (const auto& v : _vec) {
        Parser<R, W, std::remove_cvref_t<T>, ProcessorsType>::write(_w, v,
                                                                    new_parent);
      }
      _w.end_array(&arr);
    }
  }

  /**
   * @brief Generates the schema for the vector-like type.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    if constexpr (treat_as_map()) {
      return MapParser<R, W, VecType, ProcessorsType>::to_schema(_definitions);
    } else {
      using Type = schema::Type;
      return Type{Type::TypedArray{
          .type_ = Ref<Type>::make(
              Parser<R, W, T, ProcessorsType>::to_schema(_definitions))}};
    }
  }

 private:
  /**
   * @brief Determines if the vector-like type should be treated as a map.
   *
   * @return true if the type should be treated as a map, false otherwise.
   */
  static constexpr bool treat_as_map() {
    if constexpr (is_map_like_not_multimap<VecType>()) {
      using Key = std::remove_cvref_t<typename T::first_type>;
      if constexpr (StringMapKey<Key, ProcessorsType>::enabled) {
        return true;
      } else if constexpr (internal::has_reflection_type_v<
                               typename T::first_type>) {
        using U = std::remove_cvref_t<typename T::first_type::ReflectionType>;
        return std::is_same<U, std::string>() || std::is_integral_v<U> ||
               std::is_floating_point_v<U>;

        // We do not need std::string here, it is already caught by the template
        // specialization.
      } else if constexpr (std::is_integral_v<typename T::first_type> ||
                           std::is_floating_point_v<typename T::first_type>) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
};

}  // namespace parsing
}  // namespace rfl

#endif
