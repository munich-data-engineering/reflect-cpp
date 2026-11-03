#ifndef RFL_PARSING_PARSER_VARIANT_HPP_
#define RFL_PARSING_PARSER_VARIANT_HPP_

#include <map>
#include <optional>
#include <type_traits>
#include <variant>

#include "../NamedTuple.hpp"
#include "../Result.hpp"
#include "../Variant.hpp"
#include "../internal/add_tags_to_variants_v.hpp"
#include "../internal/all_fields.hpp"
#include "../internal/to_ptr_field.hpp"
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
struct is_variant : std::false_type {};

template <class... AlternativeTypes>
struct is_variant<std::variant<AlternativeTypes...>> : std::true_type {};

template <class T>
constexpr bool is_variant_v = is_variant<std::remove_cvref_t<T>>::value;

template <class R, class W, class T, class ProcessorsType>
class ParserVariant;

/**
 * @brief Partial specialization of ParserVariant for std::variant.
 *
 * This class handles the parsing and serialization of std::variant types,
 * dispatching to the appropriate parser logic based on the reader/writer
 * capabilities and the presence of schema-related processors.
 */
template <class R, class W, class... AlternativeTypes, class ProcessorsType>
  requires AreReaderAndWriter<R, W, std::variant<AlternativeTypes...>>
class ParserVariant<R, W, std::variant<AlternativeTypes...>, ProcessorsType> {
  template <class T>
  using ptr_field_t =
      decltype(internal::to_ptr_field(std::declval<const T&>()));

  using ParentType = Parent<W>;

 public:
  using InputVarType = typename R::InputVarType;

  /**
   * @brief Reads a variant from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed variant or an error.
   */
  static Result<std::variant<AlternativeTypes...>> read(
      const R& _r, const InputVarType& _var) noexcept {
    if constexpr (internal::all_fields_v<AlternativeTypes...>) {
      if constexpr (schemaful::IsSchemafulReader<R>) {
        using WrappedType = rfl::Variant<NamedTuple<AlternativeTypes>...>;
        return Parser<R, W, WrappedType, ProcessorsType>::read(_r, _var)
            .transform(
                [](auto&& _variant) -> std::variant<AlternativeTypes...> {
                  return std::move(_variant).visit([](auto&& _named_tuple) {
                    return std::variant<AlternativeTypes...>(std::move(
                        std::move(_named_tuple).fields().template get<0>()));
                  });
                });

      } else {
        const auto wrap = [](auto&& _v) {
          return std::variant<AlternativeTypes...>(std::move(_v));
        };
        const auto to_std_variant = [&](auto&& _v) {
          return rfl::visit(wrap, std::move(_v));
        };
        return FieldVariantParser<R, W, ProcessorsType,
                                  AlternativeTypes...>::read(_r, _var)
            .transform(to_std_variant);
      }

    } else if constexpr (schemaful::IsSchemafulReader<R>) {
      using V =
          schemaful::VariantReader<R, W, std::variant<AlternativeTypes...>,
                                   ProcessorsType, AlternativeTypes...>;
      return _r.to_union(_var).and_then([&](const auto& _u) {
        return _r.template read_union<std::variant<AlternativeTypes...>, V>(_u);
      });

    } else if constexpr (internal::add_tags_to_variants_v<ProcessorsType> ||
                         internal::add_namespaced_tags_to_variants_v<
                             ProcessorsType>) {
      constexpr bool remove_namespaces =
          internal::add_tags_to_variants_v<ProcessorsType>;
      using FieldVariantType = rfl::Variant<
          VariantAlternativeWrapper<AlternativeTypes, remove_namespaces>...>;
      const auto from_field_variant =
          [](auto&& _field) -> std::variant<AlternativeTypes...> {
        return std::move(_field.value());
      };
      return Parser<R, W, FieldVariantType, ProcessorsType>::read(_r, _var)
          .transform([&](FieldVariantType&& _f) {
            return _f.visit(from_field_variant);
          });

    } else {
      std::optional<std::variant<AlternativeTypes...>> result;
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
   * @brief Writes a variant to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _variant The variant to write.
   * @param _parent The parent object to write into.
   */
  template <class P>
  static void write(const W& _w,
                    const std::variant<AlternativeTypes...>& _variant,
                    const P& _parent) {
    if constexpr (internal::all_fields_v<AlternativeTypes...>) {
      if constexpr (schemaful::IsSchemafulWriter<W>) {
        using WrappedType = rfl::Variant<
            NamedTuple<Field<AlternativeTypes::name_,
                             const typename AlternativeTypes::Type*>>...>;
        const auto to_wrapped = [](const auto& _variant) -> WrappedType {
          return std::visit(
              [](const auto& _field) -> WrappedType {
                return make_named_tuple(internal::to_ptr_field(_field));
              },
              _variant);
        };
        Parser<R, W, WrappedType, ProcessorsType>::write(
            _w, to_wrapped(_variant), _parent);

      } else {
        const auto wrap = [](const auto& _v) {
          return rfl::Variant<ptr_field_t<AlternativeTypes>...>(
              internal::to_ptr_field(_v));
        };
        const auto to_rfl_variant = [&](const auto& _v) {
          return std::visit(wrap, _v);
        };
        FieldVariantParser<
            R, W, ProcessorsType,
            ptr_field_t<AlternativeTypes>...>::write(_w,
                                                     to_rfl_variant(_variant),
                                                     _parent);
      }

    } else if constexpr (schemaful::IsSchemafulWriter<W>) {
      return std::visit(
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
          _w, std::visit(to_field_variant, _variant), _parent);

    } else {
      const auto handle = [&](const auto& _v) {
        using Type = std::remove_cvref_t<decltype(_v)>;
        Parser<R, W, Type, ProcessorsType>::write(_w, _v, _parent);
      };
      return std::visit(handle, _variant);
    }
  }

  /**
   * @brief Generates a schema for the variant.
   *
   * @param _definitions The map of definitions to add the schema to.
   * @return The generated schema type.
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

  /**
   * @brief Reads a single alternative from the input.
   *
   * @tparam _i The index of the alternative.
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _result The result pointer to store the parsed alternative.
   * @param _errors The vector of errors to add parsing errors to.
   */
  template <int _i>
  static void read_one_alternative(
      const R& _r, const InputVarType& _var,
      std::optional<std::variant<AlternativeTypes...>>* _result,
      std::vector<std::string>* _errors) noexcept {
    if (!*_result) {
      using AltType =
          std::remove_cvref_t<internal::nth_element_t<_i, AlternativeTypes...>>;
      auto res = Parser<R, W, AltType, ProcessorsType>::read(_r, _var);
      if (res) {
        _result->emplace(std::move(*res));
      } else {
        // Reserve lazily, so that reading an alternative that matches
        // does not allocate.
        _errors->reserve(sizeof...(AlternativeTypes));
        _errors->emplace_back(std::move(res).error().what());
      }
    }
  }

  /**
   * @brief Reads the variant from the input by trying each alternative.
   *
   * @tparam _is The indices of the alternatives.
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _result The result pointer to store the parsed alternative.
   * @param _errors The vector of errors to add parsing errors to.
   */
  template <int... _is>
  static void read_variant(
      const R& _r, const InputVarType& _var,
      std::optional<std::variant<AlternativeTypes...>>* _result,
      std::vector<std::string>* _errors,
      std::integer_sequence<int, _is...>) noexcept {
    (read_one_alternative<_is>(_r, _var, _result, _errors), ...);
  }
};

}  // namespace rfl::parsing

#endif
