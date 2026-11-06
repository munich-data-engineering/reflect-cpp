#ifndef RFL_PARSING_FIELD_VARIANT_PARSER_HPP_
#define RFL_PARSING_FIELD_VARIANT_PARSER_HPP_

#include <map>
#include <string>
#include <type_traits>

#include "../NamedTuple.hpp"
#include "../Result.hpp"
#include "../Tuple.hpp"
#include "../Variant.hpp"
#include "../internal/no_duplicate_field_names.hpp"
#include "../internal/to_ptr_field.hpp"
#include "../make_named_tuple.hpp"
#include "FieldVariantReader.hpp"
#include "Parser_base.hpp"
#include "schema/Type.hpp"

namespace rfl::parsing {

/// To be used when all options of the variants are rfl::Field. Essentially,
/// this is an externally tagged union.
template <class R, class W, class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<R, W, rfl::Variant<FieldTypes...>>
struct FieldVariantParser {
  using FieldVariantType = rfl::Variant<FieldTypes...>;
  using ResultType = Result<FieldVariantType>;

 public:
  using InputObjectType = typename R::InputObjectType;
  using InputVarType = typename R::InputVarType;

  /**
   * @brief Reads a field variant from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed field variant or an error.
   */
  static ResultType read(const R& _r, const InputVarType& _var) noexcept {
    static_assert(
        internal::no_duplicate_field_names<rfl::Tuple<FieldTypes...>>(),
        "Externally tagged variants cannot have duplicate field "
        "names.");

    const auto to_result = [&](const auto _obj) -> ResultType {
      auto field_variant = std::optional<Result<FieldVariantType>>();
      const auto reader =
          FieldVariantReader<R, W, ProcessorsType, FieldTypes...>(
              &_r, &field_variant);
      auto err = _r.read_object(reader, _obj);
      if (err) {
        return error(std::move(*err));
      }
      if (!field_variant) {
        return error(
            "Could not parse: Expected the object to have "
            "exactly one field, but found none.");
      }
      return std::move(*field_variant);
    };

    return _r.to_object(_var).and_then(to_result);
  }

  /**
   * @brief Writes a field variant to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _v The field variant to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const rfl::Variant<FieldTypes...>& _v,
                    const P& _parent) {
    static_assert(
        internal::no_duplicate_field_names<rfl::Tuple<FieldTypes...>>(),
        "Externally tagged variants cannot have duplicate field "
        "names.");

    _v.visit([&](const auto& _field) {
      const auto named_tuple = make_named_tuple(internal::to_ptr_field(_field));
      using NamedTupleType = std::remove_cvref_t<decltype(named_tuple)>;
      Parser<R, W, NamedTupleType, ProcessorsType>::write(_w, named_tuple,
                                                          _parent);
    });
  }

  /**
   * @brief Generates the schema for the field variant.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    return schema::Type{schema::Type::AnyOf{
        .types_ = std::vector<schema::Type>(
            {one_field_to_type<FieldTypes>(_definitions)...})}};
  }

 private:
  template <class FieldType>
  static schema::Type one_field_to_type(
      std::map<std::string, schema::Type>* _definitions) noexcept {
    using NamedTupleType = NamedTuple<std::remove_cvref_t<FieldType>>;
    return Parser<R, W, NamedTupleType, ProcessorsType>::to_schema(
        _definitions);
  }
};
}  // namespace rfl::parsing

#endif
