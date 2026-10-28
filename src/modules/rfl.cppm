module;

#include "rfl.hpp"

export module rfl;

#ifdef REFLECTCPP_AVRO
export import :avro;
#endif
#ifdef REFLECTCPP_BSON
export import :bson;
#endif
#ifdef REFLECTCPP_CAPNPROTO
export import :capnproto;
#endif
#ifdef REFLECTCPP_CBOR
export import :cbor;
#endif
#ifdef REFLECTCPP_CSV
export import :csv;
#endif
#ifdef REFLECTCPP_FLEXBUFFERS
export import :flexbuf;
#endif
#ifdef REFLECTCPP_JSON
export import :json;
#endif
#ifdef REFLECTCPP_MSGPACK
export import :msgpack;
#endif
#ifdef REFLECTCPP_PARQUET
export import :parquet;
#endif
#ifdef REFLECTCPP_TOML
export import :toml;
#endif
#ifdef REFLECTCPP_UBJSON
export import :ubjson;
#endif
#ifdef REFLECTCPP_XML
export import :xml;
#endif
#ifdef REFLECTCPP_YAML
export import :yaml;
#endif

export namespace rfl {
using rfl::AddNamespacedTagsToVariants;
using rfl::AddStructName;
using rfl::AddTagsToVariants;
using rfl::AllOf;
using rfl::AllowRawPtrs;
using rfl::AlphaNumeric;
using rfl::AnyOf;
using rfl::Attribute;
using rfl::Base64Encoded;
using rfl::Binary;
using rfl::Box;
using rfl::Bytestring;
using rfl::Copyability;
using rfl::CopyableBox;
using rfl::Default;
using rfl::DefaultIfMissing;
using rfl::DefaultVal;
using rfl::Description;
using rfl::Email;
using rfl::EqualTo;
using rfl::Error;
using rfl::ExclusiveMaximum;
using rfl::ExclusiveMinimum;
using rfl::ExtraFields;
using rfl::Field;
using rfl::Flatten;
using rfl::Generic;
using rfl::Hex;
using rfl::Literal;
using rfl::LiteralHelper;
using rfl::Maximum;
using rfl::Minimum;
using rfl::NamedTuple;
using rfl::NoExtraFields;
using rfl::NoFieldNames;
using rfl::NoOptionals;
using rfl::NotEqualTo;
using rfl::Nothing;
using rfl::Object;
using rfl::Oct;
using rfl::OneOf;
using rfl::Pattern;
using rfl::PatternValidator;
using rfl::PossibleTags;
using rfl::Processors;
using rfl::Ref;
using rfl::Rename;
using rfl::Result;
using rfl::Size;
using rfl::Skip;
using rfl::SkipDeserialization;
using rfl::SkipSerialization;
using rfl::SnakeCaseToCamelCase;
using rfl::SnakeCaseToPascalCase;
using rfl::TaggedUnion;
using rfl::TaggedUnionBased;
using rfl::Timestamp;
using rfl::UnderlyingEnums;
using rfl::Unexpected;
using rfl::UUIDv1;
using rfl::UUIDv2;
using rfl::UUIDv3;
using rfl::UUIDv4;
using rfl::Validator;
using rfl::Variant;
using rfl::VariantBased;
using rfl::Vectorstring;

using rfl::apply;
using rfl::as;
using rfl::enum_to_string;
using rfl::error;
using rfl::fields;
using rfl::from_generic;
using rfl::from_named_tuple;
using rfl::get;
using rfl::get_enum_range;
using rfl::get_enumerator_array;
using rfl::get_enumerators;
using rfl::get_if;
using rfl::get_underlying_enumerator_array;
using rfl::get_underlying_enumerators;
using rfl::holds_alternative;
using rfl::make_box;
using rfl::make_copyable_box;
using rfl::make_field;
using rfl::make_from_tuple;
using rfl::make_named_tuple;
using rfl::make_ref;
using rfl::name_of;
using rfl::replace;
using rfl::string_to_enum;
using rfl::to_array;
using rfl::to_bool;
using rfl::to_double;
using rfl::to_generic;
using rfl::to_int;
using rfl::to_int64;
using rfl::to_named_tuple;
using rfl::to_null;
using rfl::to_object;
using rfl::to_string;
using rfl::to_view;
using rfl::tuple_cat;
using rfl::value_of;
using rfl::visit;

using rfl::always_false_v;
using rfl::default_value;
using rfl::define_literal_t;
using rfl::define_named_tuple_t;
using rfl::define_tagged_union_t;
using rfl::define_variant_t;
using rfl::extract_discriminators_t;
using rfl::field_type_t;
using rfl::name_t;
using rfl::named_tuple_t;
using rfl::possible_tags_t;
using rfl::remove_fields_t;
using rfl::type_name_t;
using rfl::variant_alternative;
using rfl::variant_alternative_t;
using rfl::variant_size;
using rfl::variant_size_v;
using rfl::view_t;

using rfl::operator<=>;
using rfl::operator<<;
using rfl::operator*;
using rfl::operator==;

namespace atomic {
using rfl::atomic::is_atomic_v;
using rfl::atomic::remove_atomic_t;
using rfl::atomic::set_atomic;
}  // namespace atomic

namespace concepts {
using rfl::concepts::BackInsertableByteContainer;
using rfl::concepts::ByteLike;
using rfl::concepts::ByteSpanLike;
using rfl::concepts::ContiguousByteContainer;
using rfl::concepts::MutableContiguousByteContainer;
}  // namespace concepts

namespace generic {
using rfl::generic::Parser;
using rfl::generic::Reader;
using rfl::generic::Writer;

using rfl::generic::read;
using rfl::generic::write;
}  // namespace generic

}  // namespace rfl

export namespace std {
using std::hash;
using std::swap;

#ifdef REFLECTCPP_USE_STD_EXPECTED
using std::bad_expected_access;
#endif
}  // namespace std
