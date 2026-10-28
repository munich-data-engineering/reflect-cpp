#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cbor.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_error_messages {

struct Person {
  rfl::Rename<"firstName", std::string> first_name;
  rfl::Rename<"lastName", std::string> last_name;
  rfl::Timestamp<"%Y-%m-%d"> birthday;
  std::vector<Person> children;
};

TEST(cbor, test_empty_field_error_messages) {
  // Use JSON input to generic parser for convenient flexible test input
  const std::string faulty_string = R"({})";
  const auto faulty_generic = rfl::json::read<rfl::Generic>(faulty_string);
  const auto faulty_cbor = rfl::cbor::write(faulty_generic);

  rfl::Result<Person> result = rfl::error("result didn't get set");
  EXPECT_NO_THROW({ result = rfl::cbor::read<Person>(faulty_cbor); });

  const std::string expected = R"(Found 4 errors:
1) Field named 'firstName' not found.
2) Field named 'lastName' not found.
3) Field named 'birthday' not found.
4) Field named 'children' not found.)";
  EXPECT_EQ(result.error().what(), expected);

  EXPECT_FALSE(result.has_value());
}

TEST(cbor, test_field_type_error_messages) {
  // Use JSON input to generic parser for convenient flexible test input
  const std::string faulty_string =
      R"({"firstName":"Homer","lastName":12345,"birthday":"04/19/1987"})";
  const auto faulty_generic = rfl::json::read<rfl::Generic>(faulty_string);
  const auto faulty_cbor = rfl::cbor::write(faulty_generic);

  rfl::Result<Person> result = rfl::error("result didn't get set");
  EXPECT_NO_THROW({ result = rfl::cbor::read<Person>(faulty_cbor); });

  // Order of errors is different than input JSON because rfl::Generic doesn't
  // preserve order
  const std::string expected = R"(Found 3 errors:
1) Failed to parse field 'birthday': String '04/19/1987' did not match format '%Y-%m-%d'.
2) Failed to parse field 'lastName': Could not cast to string.
3) Field named 'children' not found.)";

  EXPECT_FALSE(result.has_value());

  EXPECT_EQ(result.error().what(), expected);
}

TEST(cbor, test_decode_error_without_exception) {
  const Person homer{"Homer", "Simpson", "1987-04-19"};
  auto faulty_cbor = rfl::cbor::write(homer);
  faulty_cbor[1] = '\xfe';  // Corrupt structure of CBOR encoding

  rfl::Result<Person> result = rfl::error("result didn't get set");
  EXPECT_NO_THROW({ result = rfl::cbor::read<Person>(faulty_cbor); });

  // A proposal: A generic prefix, followed by the underlying library's error
  // output
  const std::string expected =
      R"(Could not parse CBOR: Unknown type in input at position 1)";
  EXPECT_EQ(result.error().what(), expected);

  EXPECT_FALSE(result.has_value());
}

}  // namespace test_error_messages
