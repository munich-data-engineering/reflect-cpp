#include <gtest/gtest.h>

#include <pugixml.hpp>
#include <rfl.hpp>
#include <rfl/xml.hpp>
#include <string>

namespace test_xml_array_and_indent_fix {

struct Person {
  std::string name;
  int age;
};

TEST(xml, test_get_field_from_array_index) {
  pugi::xml_document doc;
  ASSERT_TRUE(doc.load_string("<root><item>a</item><item>b</item><item>c</item></root>"));

  const rfl::xml::Reader reader;
  const auto arr =
      reader.to_array(rfl::xml::Reader::InputVarType(doc.child("root").child("item")));
  ASSERT_TRUE(arr.has_value());

  const auto f0 = reader.get_field_from_array(0, *arr);
  const auto f1 = reader.get_field_from_array(1, *arr);
  const auto f2 = reader.get_field_from_array(2, *arr);
  const auto f3 = reader.get_field_from_array(3, *arr);

  ASSERT_TRUE(f0.has_value());
  ASSERT_TRUE(f1.has_value());
  ASSERT_TRUE(f2.has_value());
  EXPECT_FALSE(f3.has_value());

  EXPECT_EQ(reader.to_basic_type<std::string>(*f0).value(), "a");
  EXPECT_EQ(reader.to_basic_type<std::string>(*f1).value(), "b");
  EXPECT_EQ(reader.to_basic_type<std::string>(*f2).value(), "c");
  EXPECT_NE(f3.error().what().find("out of bounds"), std::string::npos);
}

TEST(xml, test_write_forwards_indent) {
  const Person p{.name = "Homer", .age = 45};
  const std::string two_spaces = rfl::xml::write(p, "  ");
  const std::string compact = rfl::xml::write(p, "");

  EXPECT_NE(two_spaces.find("\n  <name>Homer</name>"), std::string::npos)
      << two_spaces;
  EXPECT_EQ(compact.find("  <name>"), std::string::npos) << compact;
}

}  // namespace test_xml_array_and_indent_fix
