#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <string_view>

#include <rfl.hpp>
#include <rfl/internal/get_type_name.hpp>
#include <rfl/json.hpp>

// transform_camel_case prepends '_' to names starting with uppercase
// File: include/rfl/internal/transform_case.hpp:60-73
// When i==0 and the first character is uppercase, it unconditionally prepends
// '_'. E.g. "MyField" → "_my_field" instead of "my_field".
namespace test_camel_case_leading_underscore {

struct CamelStruct {
  int MyField = 42;
};

TEST(regression, camel_case_to_snake_case_no_leading_underscore) {
  const auto s = CamelStruct{.MyField = 42};
  const auto json = rfl::json::write<rfl::CamelCaseToSnakeCase>(s);
  // Should produce "my_field", not "_my_field"
  EXPECT_EQ(json, R"({"my_field":42})")
      << "CamelCaseToSnakeCase should not prepend underscore. Got: " << json;
}

}  // namespace test_camel_case_leading_underscore

// FieldVariantParser wrong error message for zero fields
// File: include/rfl/parsing/FieldVariantParser.hpp:47-50
// When the input object has zero matching fields, the error says
// "found more than one" which is misleading. Should say something
// like "found none".
namespace test_field_variant_empty_object {

using FieldVar =
    rfl::Variant<rfl::Field<"a", int>, rfl::Field<"b", int>>;

TEST(regression, field_variant_empty_object_error_message) {
  const auto result = rfl::json::read<FieldVar>("{}");
  ASSERT_FALSE(result) << "Parsing empty object as FieldVariant should fail";
  const std::string msg = result.error().what();
  EXPECT_EQ(msg.find("more than one"), std::string::npos)
      << "Error for zero matching fields should NOT say 'more than one'. "
         "Got: " << msg;
}

}  // namespace test_field_variant_empty_object

// json::Writer uses yyjson_mut_strcpy(_name.data()) which reads until '\0',
// but std::string_view::data() does not guarantee null-termination.
// File: src/rfl/json/Writer.cpp:68, :91, :113
// The template method add_value_to_object in Writer.hpp correctly uses
// yyjson_mut_strncpy. The non-template methods in Writer.cpp do not.
namespace test_writer_strcpy_non_null_terminated {

TEST(regression, json_writer_handles_non_null_terminated_field_names) {
  // "nameXXX" — taking substr "name" produces a string_view whose data()
  // points into the middle of "nameXXX", NOT null-terminated after "name".
  const std::string backing = "nameXXX";
  const std::string_view field_name = std::string_view(backing).substr(0, 4);
  // field_name.data() points to "nameXXX", field_name.size() == 4
  // A correct implementation must use size, not rely on '\0'.

  auto w = rfl::json::Writer();
  auto obj = w.object_as_root(1);

  // add_null_to_object uses yyjson_mut_strcpy — the buggy path
  w.add_null_to_object(field_name, &obj);
  w.end_object(&obj);

  const char* json_c_str = yyjson_mut_write(w.doc(), 0, nullptr);
  ASSERT_NE(json_c_str, nullptr);
  const std::string json(json_c_str);
  free(const_cast<char*>(json_c_str));

  // If strcpy reads past string_view, the key will be "nameXXX" (7 chars).
  // Correct behavior: key is "name" (4 chars).
  EXPECT_EQ(json, R"({"name":null})")
      << "Writer should respect string_view length, not read until '\\0'. "
         "Got: " << json;
}

}  // namespace test_writer_strcpy_non_null_terminated

// rfl::internal::get_type_name_str_view<T>() previously returned
// get_type_name<T>().string_view(), which dangled into the temporary
// StringLiteral returned by value and failed constant evaluation.
namespace test_get_type_name_str_view {

struct SampleType {
  int value = 0;
};

constexpr std::string_view kIntName = rfl::internal::get_type_name_str_view<int>();
constexpr std::string_view kStructName =
    rfl::internal::get_type_name_str_view<SampleType>();

static_assert(kIntName == "int");
static_assert(kIntName == rfl::internal::get_type_name<int>().string_view());
static_assert(kStructName ==
              rfl::internal::get_type_name<SampleType>().string_view());

TEST(regression, get_type_name_str_view_is_valid_constexpr) {
  EXPECT_EQ(kIntName, "int");
  EXPECT_EQ(kStructName,
            rfl::internal::get_type_name<SampleType>().string_view());
  EXPECT_NE(kStructName.find("SampleType"), std::string_view::npos);
}

}  // namespace test_get_type_name_str_view

// Parser_string_view / Parser_span memory leak bugs are tested
// in tests/alloc/ — a separate executable with custom operator new[]/delete[]
// that tracks allocations to detect leaks without ASAN.
