#ifndef RFL_PARSING_PARSER_RFL_VARIANT_HPP_
#define RFL_PARSING_PARSER_RFL_VARIANT_HPP_

#include <map>
#include <type_traits>

#include "../Result.hpp"
#include "../Variant.hpp"
#include "../internal/add_tags_to_variants_v.hpp"
#include "../internal/all_fields.hpp"
#include "../internal/nth_element_t.hpp"
#include "../visit.hpp"
#include "FieldVariantParser.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "VariantAlternativeWrapper.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"
#include "schemaful/VariantReader.hpp"
#include "to_single_error_message.hpp"

namespace rfl::parsing {

template <class T>
struct is_rfl_variant : std::false_type {};

template <class... T>
struct is_rfl_variant<rfl::Variant<T...>> : std::true_type {};

template <class T>
constexpr bool is_rfl_variant_v = is_rfl_variant<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, class ProcessorsType>
class ParserRflVariant;

template <class R, class W, class... AlternativeTypes, class ProcessorsType>
  requires AreReaderAndWriter<R, W, rfl::Variant<AlternativeTypes...>>
class ParserRflVariant<R, W, rfl::Variant<AlternativeTypes...>,
                       ProcessorsType> {
  using ParentType = Parent<W>;

 public:
  using InputVarType = typename R::InputVarType;

  /**
   * @brief Reads an rfl::Variant from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed rfl::Variant or an error.
   */
  static Result<rfl::Variant<AlternativeTypes...>> read(
      const R& _r, const InputVarType& _var) noexcept {
    if constexpr (internal::all_fields_v<AlternativeTypes...>) {
      if constexpr (schemaful::IsSchemafulReader<R>) {
        using WrappedType = rfl::Variant<NamedTuple<AlternativeTypes>...>;
        return Parser<R, W, WrappedType, ProcessorsType>::read(_r, _var)
            .transform(
                [](auto&& _variant) -> rfl::Variant<AlternativeTypes...> {
                  return std::move(_variant).visit([](auto&& _named_tuple) {
                    return rfl::Variant<AlternativeTypes...>(std::move(
                        std::move(_named_tuple).fields().template get<0>()));
                  });
                });

      } else {
        return FieldVariantParser<R, W, ProcessorsType,
                                  AlternativeTypes...>::read(_r, _var);
      }

    } else if constexpr (schemaful::IsSchemafulReader<R>) {
      using V = schemaful::VariantReader<R, W, Variant<AlternativeTypes...>,
                                         ProcessorsType, AlternativeTypes...>;
      return _r.to_union(_var).and_then([&](const auto& _u) {
        return _r.template read_union<Variant<AlternativeTypes...>, V>(_u);
      });

    } else if constexpr (internal::add_tags_to_variants_v<ProcessorsType> ||
                         internal::add_namespaced_tags_to_variants_v<
                             ProcessorsType>) {
      constexpr bool remove_namespaces =
          internal::add_tags_to_variants_v<ProcessorsType>;
      using FieldVariantType = rfl::Variant<
          VariantAlternativeWrapper<AlternativeTypes, remove_namespaces>...>;
      const auto from_field_variant =
          [](auto&& _field) -> rfl::Variant<AlternativeTypes...> {
        return std::move(_field.value());
      };
      return Parser<R, W, FieldVariantType, ProcessorsType>::read(_r, _var)
          .transform([&](FieldVariantType&& _f) {
            return _f.visit(from_field_variant);
          });

    } else {
      std::optional<rfl::Variant<AlternativeTypes...>> result;
      std::vector<std::string> errors;
      read_variant(
          _r, _var, &result, &errors,
          std::make_integer_sequence<int, sizeof...(AlternativeTypes)>());
      if (result) {
        return std::move(*result);
      } else {
        return error(
            to_single_error_message(errors,
                                    "Could not parse the variant. Each of the "
                                    "possible alternatives failed "
                                    "for the following reasons: ",
                                    100000));
      }
    }
  }

  /**
   * @brief Writes an rfl::Variant to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _variant The rfl::Variant to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w,
                    const rfl::Variant<AlternativeTypes...>& _variant,
                    const P& _parent) {
    if constexpr (internal::all_fields_v<AlternativeTypes...>) {
      if constexpr (schemaful::IsSchemafulWriter<W>) {
        using WrappedType = rfl::Variant<
            NamedTuple<Field<AlternativeTypes::name_,
                             const typename AlternativeTypes::Type*>>...>;
        const auto to_wrapped = [](const auto& _variant) -> WrappedType {
          return _variant.visit([](const auto& _field) -> WrappedType {
            return make_named_tuple(internal::to_ptr_field(_field));
          });
        };
        Parser<R, W, WrappedType, ProcessorsType>::write(
            _w, to_wrapped(_variant), _parent);

      } else {
        FieldVariantParser<R, W, ProcessorsType, AlternativeTypes...>::write(
            _w, _variant, _parent);
      }

    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      return rfl::visit(
          [&](const auto& _v) {
            using Type = std::remove_cvref_t<decltype(_v)>;
            auto u = ParentType::add_union(_w, _parent);
            using UnionType = typename ParentType::template Union<decltype(u)>;
            auto p = UnionType{.index_ = static_cast<size_t>(_variant.index()),
                               .union_ = &u};
            Parser<R, W, Type, ProcessorsType>::write(_w, _v, p);
          },
          _variant);

    } else if constexpr (internal::add_tags_to_variants_v<ProcessorsType> ||
                         internal::add_namespaced_tags_to_variants_v<
                             ProcessorsType>) {
      constexpr bool remove_namespaces =
          internal::add_tags_to_variants_v<ProcessorsType>;
      using FieldVariantType =
          rfl::Variant<VariantAlternativeWrapper<const AlternativeTypes*,
                                                 remove_namespaces>...>;
      const auto to_field_variant =
          []<class T>(const T& _t) -> FieldVariantType {
        return VariantAlternativeWrapper<const T*, remove_namespaces>(&_t);
      };
      Parser<R, W, FieldVariantType, ProcessorsType>::write(
          _w, _variant.visit(to_field_variant), _parent);

    } else {
      const auto handle = [&](const auto& _v) {
        using Type = std::remove_cvref_t<decltype(_v)>;
        Parser<R, W, Type, ProcessorsType>::write(_w, _v, _parent);
      };
      return rfl::visit(handle, _variant);
    }
  }

  /**
   * @brief Generates the schema for the rfl::Variant.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    if constexpr (internal::all_fields_v<AlternativeTypes...>) {
      return FieldVariantParser<R, W, ProcessorsType,
                                AlternativeTypes...>::to_schema(_definitions);

    } else if constexpr (internal::add_tags_to_variants_v<ProcessorsType> ||
                         internal::add_namespaced_tags_to_variants_v<
                             ProcessorsType>) {
      constexpr bool remove_namespaces =
          internal::add_tags_to_variants_v<ProcessorsType>;
      using FieldVariantType = rfl::Variant<
          VariantAlternativeWrapper<AlternativeTypes, remove_namespaces>...>;
      return Parser<R, W, FieldVariantType, ProcessorsType>::to_schema(
          _definitions);

    } else {
      return schema::Type{schema::Type::AnyOf{
          .types_ = std::vector<schema::Type>(
              {one_field_to_type<AlternativeTypes>(_definitions)...})}};
    }
  }

 private:
  template <class AltType>
  static schema::Type one_field_to_type(
      std::map<std::string, schema::Type>* _definitions) noexcept {
    return Parser<R, W, std::remove_cvref_t<AltType>,
                  ProcessorsType>::to_schema(_definitions);
  }

  template <int _i>
  static void read_one_alternative(
      const R& _r, const InputVarType& _var,
      std::optional<rfl::Variant<AlternativeTypes...>>* _result,
      std::vector<std::string>* _errors) noexcept {
    if (!*_result) {
      using AltType =
          std::remove_cvref_t<internal::nth_element_t<_i, AlternativeTypes...>>;
      auto res = Parser<R, W, AltType, ProcessorsType>::read(_r, _var);
      if (res) {
        *_result = std::move(*res);
      } else {
        // Reserve lazily, so that reading an alternative that matches
        // does not allocate.
        _errors->reserve(sizeof...(AlternativeTypes));
        _errors->emplace_back(std::move(res).error().what());
      }
    }
  }

  template <int... _is>
  static void read_variant(
      const R& _r, const InputVarType& _var,
      std::optional<rfl::Variant<AlternativeTypes...>>* _result,
      std::vector<std::string>* _errors,
      std::integer_sequence<int, _is...>) noexcept {
    (read_one_alternative<_is>(_r, _var, _result, _errors), ...);
  }
};

}  // namespace rfl::parsing

#endif
