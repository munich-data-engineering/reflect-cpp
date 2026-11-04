/*

MIT License

Copyright (c) 2023-2024 Code17 GmbH

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "rfl/json/to_schema.hpp"

#include "rfl/json/schema/Type.hpp"
#include "rfl/json/write.hpp"

namespace rfl::json {

schema::Type type_to_json_schema_type(const parsing::schema::Type& _type,
                                      const bool _no_required);

bool is_optional(const parsing::schema::Type& _t) {
  return _t.variant_.visit([&](const auto& _v) -> bool {
    using T = std::remove_cvref_t<decltype(_v)>;
    if constexpr (std::is_same_v<T, parsing::schema::Type::Deprecated>) {
      return is_optional(*_v.type_);

    } else if constexpr (std::is_same_v<T,
                                        parsing::schema::Type::Description>) {
      return is_optional(*_v.type_);

    } else if constexpr (std::is_same_v<T, parsing::schema::Type::Validated>) {
      return is_optional(*_v.type_);

    } else if constexpr (std::is_same_v<T, parsing::schema::Type::AnyOf>) {
      return !_v.types_.empty() &&
             rfl::holds_alternative<parsing::schema::Type::Null>(
                 _v.types_.front().variant_);

    } else {
      return std::is_same_v<T, parsing::schema::Type::Optional> ||
             std::is_same_v<T, parsing::schema::Type::DefaultVal>;
    }
  });
}

std::string numeric_type_to_string(const parsing::schema::Type& _type) {
  const auto handle_variant = [](const auto& _t) -> std::string {
    using T = std::remove_cvref_t<decltype(_t)>;
    using Type = parsing::schema::Type;
    if constexpr (std::is_same<T, Type::Int32>() ||
                  std::is_same<T, Type::Int64>() ||
                  std::is_same<T, Type::UInt32>() ||
                  std::is_same<T, Type::UInt64>() ||
                  std::is_same<T, Type::Integer>()) {
      return schema::Type::Integer{}.type.str();
    } else {
      return schema::Type::Number{}.type.str();
    }
  };
  return rfl::visit(handle_variant, _type.variant_);
}

schema::Type handle_validation_type(
    const parsing::schema::Type& _type,
    const parsing::schema::ValidationType& _validation_type,
    const bool _no_required) {
  auto handle_variant = [&](const auto& _v) -> schema::Type {
    using T = std::remove_cvref_t<decltype(_v)>;
    using ValidationType = parsing::schema::ValidationType;
    if constexpr (std::is_same<T, ValidationType::AllOf>()) {
      auto all_of = std::vector<schema::Type>();
      for (const auto& t : _v.types_) {
        all_of.emplace_back(handle_validation_type(_type, t, _no_required));
      }
      return schema::Type{.value = schema::Type::AllOf{.allOf = all_of}};

    } else if constexpr (std::is_same<T, ValidationType::AnyOf>()) {
      auto any_of = std::vector<schema::Type>();
      for (const auto& t : _v.types_) {
        any_of.emplace_back(handle_validation_type(_type, t, _no_required));
      }
      return schema::Type{.value = schema::Type::AnyOf{.anyOf = any_of}};

    } else if constexpr (std::is_same<T, ValidationType::OneOf>()) {
      auto one_of = std::vector<schema::Type>();
      for (const auto& t : _v.types_) {
        one_of.emplace_back(handle_validation_type(_type, t, _no_required));
      }
      return schema::Type{.value = schema::Type::OneOf{.oneOf = one_of}};

    } else if constexpr (std::is_same<T, ValidationType::Regex>()) {
      return schema::Type{.value = schema::Type::Regex{.pattern = _v.pattern_}};

    } else if constexpr (std::is_same<T, ValidationType::Size>()) {
      auto t = type_to_json_schema_type(_type, _no_required);
      const auto to_size = [](const auto _val) {
        return static_cast<size_t>(_val);
      };
      auto handle_size_variant = [&](auto& _t, const auto& _size_limit) {
        using U = std::remove_cvref_t<decltype(_t)>;
        using V = std::remove_cvref_t<decltype(_size_limit)>;
        if constexpr (std::is_same<U, schema::Type::TypedArray>() ||
                      std::is_same<U, schema::Type::String>()) {
          if constexpr (std::is_same<V, ValidationType::Minimum>()) {
            _t.minSize = _size_limit.value_.visit(to_size);
            return t;

          } else if constexpr (std::is_same<V, ValidationType::Maximum>()) {
            _t.maxSize = _size_limit.value_.visit(to_size);
            return t;

          } else if constexpr (std::is_same<V, ValidationType::EqualTo>()) {
            _t.minSize = _size_limit.value_.visit(to_size);
            _t.maxSize = _size_limit.value_.visit(to_size);
            return t;

          } else if constexpr (std::is_same<V, ValidationType::AnyOf>() ||
                               std::is_same<V, ValidationType::AllOf>()) {
            V v;
            for (const auto& limiter : _size_limit.types_) {
              v.types_.push_back(ValidationType{ValidationType::Size{
                  .size_limit_ = rfl::Ref<ValidationType>::make(limiter)}});
            }
            return handle_validation_type(_type, ValidationType{.variant_ = v},
                                          _no_required);
          }
        }
        return t;
      };

      return rfl::visit(handle_size_variant, t.value, _v.size_limit_->variant_);

    } else if constexpr (std::is_same<T, ValidationType::ExclusiveMaximum>()) {
      return schema::Type{.value = schema::Type::ExclusiveMaximum{
                              .exclusiveMaximum = _v.value_,
                              .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::ExclusiveMinimum>()) {
      return schema::Type{.value = schema::Type::ExclusiveMinimum{
                              .exclusiveMinimum = _v.value_,
                              .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::Maximum>()) {
      return schema::Type{
          .value = schema::Type::Maximum{
              .maximum = _v.value_, .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::Minimum>()) {
      return schema::Type{
          .value = schema::Type::Minimum{
              .minimum = _v.value_, .type = numeric_type_to_string(_type)}};

    } else if constexpr (std::is_same<T, ValidationType::EqualTo>()) {
      const auto maximum = schema::Type{
          .value = schema::Type::Maximum{
              .maximum = _v.value_, .type = numeric_type_to_string(_type)}};
      const auto minimum = schema::Type{
          .value = schema::Type::Minimum{
              .minimum = _v.value_, .type = numeric_type_to_string(_type)}};
      return schema::Type{.value =
                              schema::Type::AllOf{.allOf = {maximum, minimum}}};

    } else if constexpr (std::is_same<T, ValidationType::NotEqualTo>()) {
      const auto excl_maximum =
          schema::Type{.value = schema::Type::ExclusiveMaximum{
                           .exclusiveMaximum = _v.value_,
                           .type = numeric_type_to_string(_type)}};
      const auto excl_minimum =
          schema::Type{.value = schema::Type::ExclusiveMinimum{
                           .exclusiveMinimum = _v.value_,
                           .type = numeric_type_to_string(_type)}};
      return schema::Type{
          .value = schema::Type::AnyOf{.anyOf = {excl_maximum, excl_minimum}}};

    } else {
      static_assert(rfl::always_false_v<T>, "Not all cases were covered.");
    }
  };

  return rfl::visit(handle_variant, _validation_type.variant_);
}

schema::Type type_to_json_schema_type(const parsing::schema::Type& _type,
                                      const bool _no_required) {
  auto handle_variant = [&](const auto& _t) -> schema::Type {
    using T = std::remove_cvref_t<decltype(_t)>;
    using Type = parsing::schema::Type;
    if constexpr (std::is_same<T, Type::Boolean>()) {
      return schema::Type{.value = schema::Type::Boolean{}};

    } else if constexpr (std::is_same<T, Type::Null>()) {
      return schema::Type{.value = schema::Type::Null{}};

    } else if constexpr (std::is_same<T, Type::Int32>() ||
                         std::is_same<T, Type::Int64>() ||
                         std::is_same<T, Type::UInt32>() ||
                         std::is_same<T, Type::UInt64>() ||
                         std::is_same<T, Type::Integer>()) {
      return schema::Type{.value = schema::Type::Integer{}};

    } else if constexpr (std::is_same<T, Type::Float>() ||
                         std::is_same<T, Type::Double>()) {
      return schema::Type{.value = schema::Type::Number{}};

    } else if constexpr (std::is_same<T, Type::String>() ||
                         std::is_same<T, Type::Bytestring>() ||
                         std::is_same<T, Type::Vectorstring>()) {
      return schema::Type{.value = schema::Type::String{}};

    } else if constexpr (std::is_same<T, Type::AnyOf>()) {
      auto any_of = std::vector<schema::Type>();
      for (const auto& t : _t.types_) {
        any_of.emplace_back(type_to_json_schema_type(t, _no_required));
      }
      return schema::Type{.value = schema::Type::AnyOf{.anyOf = any_of}};

    } else if constexpr (std::is_same<T, Type::DefaultVal>()) {
      auto res = type_to_json_schema_type(*_t.type_, _no_required);
      if (!_t.default_value_.has_value()) {
        return res;
      }
      const auto update_prediction = [&](auto _v) -> schema::Type {
        _v.annotations.value_.defaultValue = *_t.default_value_;
        return schema::Type{_v};
      };
      return rfl::visit(update_prediction, res.value);

    } else if constexpr (std::is_same<T, Type::Deprecated>()) {
      auto res = type_to_json_schema_type(*_t.type_, _no_required);
      const auto update_prediction = [&](auto _v) -> schema::Type {
        _v.annotations.value_.description = _t.description_;
        _v.annotations.value_.deprecated = true;
        _v.annotations.value_.deprecationMessage = _t.deprecation_message_;
        return schema::Type{_v};
      };
      return rfl::visit(update_prediction, res.value);

    } else if constexpr (std::is_same<T, Type::Description>()) {
      auto res = type_to_json_schema_type(*_t.type_, _no_required);
      const auto update_prediction = [&](auto _v) -> schema::Type {
        _v.annotations.value_.description = _t.description_;
        return schema::Type{_v};
      };
      return rfl::visit(update_prediction, res.value);

    } else if constexpr (std::is_same<T, Type::FixedSizeTypedArray>()) {
      return schema::Type{
          .value = schema::Type::FixedSizeTypedArray{
              .items = Ref<schema::Type>::make(
                  type_to_json_schema_type(*_t.type_, _no_required)),
              .minItems = _t.size_,
              .maxItems = _t.size_}};

    } else if constexpr (std::is_same<T, Type::Literal>()) {
      return schema::Type{.value =
                              schema::Type::StringEnum{.values = _t.values_}};

    } else if constexpr (std::is_same<T, Type::DescribedLiteral>()) {
      // Convert to OneOf with StringConst for each described value
      auto one_of = std::vector<schema::Type>();
      for (const auto& v : _t.values_) {
        one_of.push_back(schema::Type{
            .value = schema::Type::StringConst{
                .annotations =
                    schema::Type::Annotations{
                        .description =
                            v.description_.empty()
                                ? std::nullopt
                                : std::optional<std::string>(v.description_)},
                .value = v.value_}});
      }
      return schema::Type{.value = schema::Type::OneOf{.oneOf = one_of}};

    } else if constexpr (std::is_same<T, Type::Object>()) {
      auto properties = rfl::Object<schema::Type>();
      auto required = std::vector<std::string>();
      for (const auto& [k, v] : _t.types_) {
        properties[k] = type_to_json_schema_type(v, _no_required);
        if (!is_optional(v) && !_no_required) {
          required.push_back(k);
        }
      }
      auto additional_properties = rfl::visit(
          [&](const auto& _a)
              -> std::optional<
                  rfl::Variant<std::shared_ptr<schema::Type>, bool>> {
            using A = std::remove_cvref_t<decltype(_a)>;
            if constexpr (std::is_same<A, bool>()) {
              return _a;
            } else {
              if (!_a) {
                return std::nullopt;
              }
              return std::make_shared<schema::Type>(
                  type_to_json_schema_type(*_a, _no_required));
            }
          },
          _t.additional_properties_);
      return schema::Type{.value = schema::Type::Object{
                              .properties = properties,
                              .required = required,
                              .additionalProperties = additional_properties}};

    } else if constexpr (std::is_same<T, Type::Optional>()) {
      return schema::Type{
          .value = schema::Type::AnyOf{
              .anyOf = {type_to_json_schema_type(*_t.type_, _no_required),
                        schema::Type{schema::Type::Null{}}}}};

    } else if constexpr (std::is_same<T, Type::Reference>()) {
      return schema::Type{
          .value = schema::Type::Reference{.ref = "#/$defs/" + _t.name_}};

    } else if constexpr (std::is_same<T, Type::StringMap>()) {
      return schema::Type{
          .value = schema::Type::StringMap{
              .additionalProperties = Ref<schema::Type>::make(
                  type_to_json_schema_type(*_t.value_type_, _no_required))}};

    } else if constexpr (std::is_same<T, Type::Tuple>()) {
      auto items = std::vector<schema::Type>();
      for (const auto& t : _t.types_) {
        items.emplace_back(type_to_json_schema_type(t, _no_required));
      }
      return schema::Type{.value = schema::Type::Tuple{.prefixItems = items}};

    } else if constexpr (std::is_same<T, Type::TypedArray>()) {
      return schema::Type{
          .value = schema::Type::TypedArray{
              .items = Ref<schema::Type>::make(
                  type_to_json_schema_type(*_t.type_, _no_required))}};

    } else if constexpr (std::is_same<T, Type::Validated>()) {
      return handle_validation_type(*_t.type_, _t.validation_, _no_required);

    } else {
      static_assert(rfl::always_false_v<T>, "Not all cases were covered.");
    }
  };

  return rfl::visit(handle_variant, _type.variant_);
}

std::string to_schema_internal_schema(
    const parsing::schema::Definition& internal_schema,
    const yyjson_write_flag _flag, const bool _no_required,
    const std::string& comment) {
  auto definitions = std::map<std::string, schema::Type>();
  for (const auto& [k, v] : internal_schema.definitions_) {
    definitions[k] = type_to_json_schema_type(v, _no_required);
  }
  using JSONSchemaType =
      typename TypeHelper<schema::Type::ReflectionType>::JSONSchemaType;
  const auto to_schema = [&](auto&& _root) -> JSONSchemaType {
    using U = std::decay_t<decltype(_root)>;
    return schema::JSONSchema<U>{
        .comment =
            !comment.empty() ? std::optional(std::move(comment)) : std::nullopt,
        .root = std::forward<decltype(_root)>(_root),
        .definitions = definitions,
    };
  };
  auto root = type_to_json_schema_type(internal_schema.root_, _no_required);
  const auto json_schema = rfl::visit(to_schema, std::move(root.value));
  return write(json_schema, _flag);
}

}  // namespace rfl::json
