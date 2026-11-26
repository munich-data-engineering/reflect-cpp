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

#include "rfl/capnproto/to_schema.hpp"

#include <map>
#include <sstream>
#include <string>

#include "rfl/capnproto/is_named_type.hpp"
#include "rfl/capnproto/schema/CapnProtoTypes.hpp"
#include "rfl/capnproto/schema/Type.hpp"
// #include "rfl/internal/strings/strings.hpp"
// #include "rfl/json.hpp"
#include "rfl/parsing/schemaful/tuple_to_object.hpp"

namespace rfl::capnproto {

enum struct Parent { is_top_level, is_a_struct, is_not_a_struct };

schema::Type type_to_capnproto_schema_type(
    const parsing::schema::Type& _type,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types);

schema::Type any_of_to_capnproto_schema_type(
    const parsing::schema::Type::AnyOf& _any_of,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types) {
  auto value = schema::Type::Union{};
  size_t i = 1;
  for (const auto& type : _any_of.types_) {
    value.fields.push_back(
        std::make_pair(std::string("Opt" + std::to_string(i++)),
                       type_to_capnproto_schema_type(type, _definitions,
                                                     _parent, _cnp_types)));
  }
  if (_parent == Parent::is_a_struct) {
    return schema::Type{.value = value};
  } else {
    const auto name =
        std::string("Union") + std::to_string(_cnp_types->unions_.size() + 1);
    _cnp_types->unions_[name] = schema::Type{.value = value};
    return schema::Type{.value = schema::Type::Reference{name}};
  }
}

schema::Type literal_to_capnproto_schema_type(
    const parsing::schema::Type::Literal& _literal,
    const std::map<std::string, parsing::schema::Type>& /*_definitions*/,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types) {
  const auto enum_schema = schema::Type::Enum{.fields = _literal.values_};
  if (_parent == Parent::is_top_level) {
    return schema::Type{.value = enum_schema};
  } else {
    const auto name =
        std::string("Enum") + std::to_string(_cnp_types->enums_.size() + 1);
    _cnp_types->enums_[name] = schema::Type{.value = enum_schema};
    return schema::Type{.value = schema::Type::Reference{name}};
  }
}

schema::Type object_to_capnproto_schema_type(
    const parsing::schema::Type::Object& _obj,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types) {
  schema::Type::Struct struct_schema;
  for (const auto& [k, v] : _obj.types_) {
    struct_schema.fields.push_back(std::make_pair(
        k, type_to_capnproto_schema_type(v, _definitions, Parent::is_a_struct,
                                         _cnp_types)));
  }
  if (_parent == Parent::is_top_level) {
    return schema::Type{.value = struct_schema};
  } else {
    const auto name =
        std::string("Tuple") + std::to_string(_cnp_types->tuples_.size() + 1);
    _cnp_types->tuples_[name] = schema::Type{.value = struct_schema};
    return schema::Type{.value = schema::Type::Reference{name}};
  }
}

schema::Type optional_to_capnproto_schema_type(
    const parsing::schema::Type::Optional& _optional,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types) {
  const auto value = schema::Type::Union{
      .fields =
          std::vector({std::make_pair(std::string("Some"),
                                      type_to_capnproto_schema_type(
                                          *_optional.type_, _definitions,
                                          _parent, _cnp_types)),
                       std::make_pair(std::string("None"),
                                      schema::Type{schema::Type::Void{}})})};
  if (_parent == Parent::is_a_struct) {
    return schema::Type{.value = value};
  } else {
    const auto name =
        std::string("Union") + std::to_string(_cnp_types->unions_.size() + 1);
    _cnp_types->unions_[name] = schema::Type{.value = value};
    return schema::Type{.value = schema::Type::Reference{name}};
  }
}

schema::Type reference_to_capnproto_schema_type(
    const parsing::schema::Type::Reference& _reference,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types) {
  const auto it = _definitions.find(_reference.name_);
  if (it == _definitions.end() || is_named_type(it->second)) {
    return schema::Type{
        .value = schema::Type::Reference{.type_name = _reference.name_}};
  } else {
    return type_to_capnproto_schema_type(it->second, _definitions, _parent,
                                         _cnp_types);
  }
}

schema::Type string_map_to_capnproto_schema_type(
    const parsing::schema::Type::StringMap& _map,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    schema::CapnProtoTypes* _cnp_types) {
  auto value_type = type_to_capnproto_schema_type(
      *_map.value_type_, _definitions, Parent::is_not_a_struct, _cnp_types);
  const bool is_scalar = value_type.reflection().visit([&](const auto& _value) {
    using T = std::remove_cvref_t<decltype(_value)>;
    using Type = schema::Type;
    if constexpr (std::is_same_v<T, Type::Reference>) {
      const auto it = _definitions.find(_value.type_name);
      return _cnp_types->enums_.contains(_value.type_name) ||
             (it != _definitions.end() &&
              it->second.variant_.visit([](const auto& _definition) {
                return std::is_same_v<
                    std::remove_cvref_t<decltype(_definition)>,
                    parsing::schema::Type::Literal>;
              }));
    } else {
      return !std::is_same_v<T, Type::Data> && !std::is_same_v<T, Type::Text> &&
             !std::is_same_v<T, Type::List> && !std::is_same_v<T, Type::Map> &&
             !std::is_same_v<T, Type::Struct> &&
             !std::is_same_v<T, Type::Union>;
    }
  });
  if (is_scalar) {
    // Cap'n Proto generic parameters must be pointer types. Keep scalar values
    // directly in a concrete Entry struct instead of instantiating Map(Value).
    const auto name = std::string("ScalarMap") +
                      std::to_string(_cnp_types->scalar_maps_.size() + 1);
    _cnp_types->scalar_maps_.emplace(name, std::move(value_type));
    return schema::Type{.value = schema::Type::Reference{name}};
  }
  _cnp_types->has_maps_ = true;
  return schema::Type{
      .value = schema::Type::Map{
          .type = Ref<schema::Type>::make(std::move(value_type))}};
}

schema::Type type_to_capnproto_schema_type(
    const parsing::schema::Type& _type,
    const std::map<std::string, parsing::schema::Type>& _definitions,
    const Parent _parent, schema::CapnProtoTypes* _cnp_types) {
  auto handle_variant = [&](const auto& _t) -> schema::Type {
    using T = std::remove_cvref_t<decltype(_t)>;
    using Type = parsing::schema::Type;
    if constexpr (std::is_same<T, Type::Boolean>()) {
      return schema::Type{.value = schema::Type::Bool{}};

    } else if constexpr (std::is_same<T, Type::Null>()) {
      return schema::Type{.value = schema::Type::Void{}};

    } else if constexpr (std::is_same<T, Type::Bytestring>() ||
                         std::is_same<T, Type::Vectorstring>()) {
      return schema::Type{.value = schema::Type::Data{}};

    } else if constexpr (std::is_same<T, Type::Int32>() ||
                         std::is_same<T, Type::Integer>()) {
      return schema::Type{.value = schema::Type::Int32{}};

    } else if constexpr (std::is_same<T, Type::Int64>()) {
      return schema::Type{.value = schema::Type::Int64{}};

    } else if constexpr (std::is_same<T, Type::UInt32>()) {
      return schema::Type{.value = schema::Type::UInt32{}};

    } else if constexpr (std::is_same<T, Type::UInt64>()) {
      return schema::Type{.value = schema::Type::UInt64{}};

    } else if constexpr (std::is_same<T, Type::Float>()) {
      return schema::Type{.value = schema::Type::Float32{}};

    } else if constexpr (std::is_same<T, Type::Double>()) {
      return schema::Type{.value = schema::Type::Float64{}};

    } else if constexpr (std::is_same<T, Type::String>()) {
      return schema::Type{.value = schema::Type::Text{}};

    } else if constexpr (std::is_same<T, Type::AnyOf>()) {
      return any_of_to_capnproto_schema_type(_t, _definitions, _parent,
                                             _cnp_types);

    } else if constexpr (std::is_same<T, Type::Deprecated>()) {
      return type_to_capnproto_schema_type(*_t.type_, _definitions, _parent,
                                           _cnp_types);

    } else if constexpr (std::is_same<T, Type::Description>()) {
      return type_to_capnproto_schema_type(*_t.type_, _definitions, _parent,
                                           _cnp_types);

    } else if constexpr (std::is_same<T, Type::FixedSizeTypedArray>()) {
      return schema::Type{
          .value = schema::Type::List{
              .type = Ref<schema::Type>::make(type_to_capnproto_schema_type(
                  *_t.type_, _definitions, Parent::is_not_a_struct,
                  _cnp_types))}};

    } else if constexpr (std::is_same<T, Type::Literal>()) {
      return literal_to_capnproto_schema_type(_t, _definitions, _parent,
                                              _cnp_types);

    } else if constexpr (std::is_same<T, Type::DescribedLiteral>()) {
      auto values = std::vector<std::string>();
      for (const auto& v : _t.values_) {
        values.push_back(v.value_);
      }
      return literal_to_capnproto_schema_type(
          Type::Literal{.values_ = values}, _definitions, _parent, _cnp_types);

    } else if constexpr (std::is_same<T, Type::Object>()) {
      return object_to_capnproto_schema_type(_t, _definitions, _parent,
                                             _cnp_types);

    } else if constexpr (std::is_same<T, Type::DefaultVal>()) {
      return type_to_capnproto_schema_type(*_t.type_, _definitions, _parent,
                                           _cnp_types);

    } else if constexpr (std::is_same<T, Type::Optional>()) {
      return optional_to_capnproto_schema_type(_t, _definitions, _parent,
                                               _cnp_types);

    } else if constexpr (std::is_same<T, Type::Reference>()) {
      return reference_to_capnproto_schema_type(_t, _definitions, _parent,
                                                _cnp_types);

    } else if constexpr (std::is_same<T, Type::StringMap>()) {
      return string_map_to_capnproto_schema_type(_t, _definitions, _cnp_types);

    } else if constexpr (std::is_same<T, Type::Tuple>()) {
      return type_to_capnproto_schema_type(
          Type{parsing::schemaful::tuple_to_object(_t)}, _definitions, _parent,
          _cnp_types);

    } else if constexpr (std::is_same<T, Type::TypedArray>()) {
      return schema::Type{
          .value = schema::Type::List{
              .type = Ref<schema::Type>::make(type_to_capnproto_schema_type(
                  *_t.type_, _definitions, Parent::is_not_a_struct,
                  _cnp_types))}};

    } else if constexpr (std::is_same<T, Type::Validated>()) {
      // Cap'n Proto knows no validation.
      return type_to_capnproto_schema_type(*_t.type_, _definitions, _parent,
                                           _cnp_types);
    } else {
      static_assert(rfl::always_false_v<T>, "Not all cases were covered.");
    }
  };

  return rfl::visit(handle_variant, _type.variant_);
}

std::string to_string_representation(
    const parsing::schema::Definition& _internal_schema) {
  schema::CapnProtoTypes cnp_types;
  for (const auto& [name, def] : _internal_schema.definitions_) {
    if (!is_named_type(def)) {
      continue;
    }
    cnp_types.structs_[name] = type_to_capnproto_schema_type(
        def, _internal_schema.definitions_, Parent::is_top_level, &cnp_types);
  }

  std::stringstream sstream;
  sstream << cnp_types;
  return sstream.str();
}

}  // namespace rfl::capnproto
