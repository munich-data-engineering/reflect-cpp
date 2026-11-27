#include <cassert>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

#include "write_and_read.hpp"

namespace test_literal {

using FirstName = rfl::Literal<"Homer", "Marge", "Bart", "Lisa", "Maggie">;
using LastName = rfl::Literal<"Simpson">;

struct Person {
  rfl::Rename<"firstName", FirstName> first_name;
  rfl::Rename<"lastName", LastName> last_name;
  std::vector<Person> children;
};

TEST(json, test_literal) {
  static_assert(!FirstName::template contains_any<LastName>(),
                "Must be false.");
  static_assert(FirstName::template contains_any<FirstName>(), "Must be true.");
  static_assert(FirstName::template contains_all<FirstName>(), "Must be true.");

  const auto bart = Person{.first_name = FirstName::make<"Bart">()};

  write_and_read(bart,
                 R"({"firstName":"Bart","lastName":"Simpson","children":[]})");
}

TEST(json, test_literal_reports_unsupported_string) {
  const auto res = rfl::json::read<Person>(
      R"({"firstName":"Ned","lastName":"Simpson","children":[]})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 'firstName': Literal does not support "
            "string 'Ned'. The following strings are supported: 'Homer', "
            "'Marge', 'Bart', 'Lisa', 'Maggie'.");
}

TEST(json, test_literal_has_duplicates) {
  static_assert(!FirstName::has_duplicates());
  static_assert(!LastName::has_duplicates());
  static_assert(rfl::Literal<"a", "a">::has_duplicates());
  static_assert(rfl::Literal<"a", "b", "c", "a">::has_duplicates());
  static_assert(rfl::Literal<"z", "m", "a", "m", "b">::has_duplicates());

  using UniqueLarge = rfl::Literal<
      "f15", "f03", "f28", "f00", "f19", "f07", "f24", "f11", "f31", "f02",
      "f16", "f21", "f08", "f26", "f13", "f05", "f29", "f01", "f18", "f10",
      "f25", "f06", "f22", "f14", "f30", "f04", "f17", "f20", "f09", "f27",
      "f12", "f23">;
  using DuplicateLarge = rfl::Literal<
      "f15", "f03", "f28", "f00", "f19", "f07", "f24", "f11", "f31", "f02",
      "f16", "f21", "f08", "f26", "f13", "f05", "f29", "f01", "f18", "f10",
      "f25", "f06", "f22", "f14", "f30", "f04", "f17", "f20", "f09", "f27",
      "f12", "f19">;
  static_assert(!UniqueLarge::has_duplicates());
  static_assert(DuplicateLarge::has_duplicates());

  static_assert(rfl::internal::no_duplicate_field_names<rfl::Tuple<>>());
  static_assert(rfl::internal::no_duplicate_field_names<
                rfl::Tuple<rfl::Field<"a", int>>>());
  static_assert(
      rfl::internal::no_duplicate_field_names<
          rfl::Tuple<rfl::Field<"b", int>, rfl::Field<"a", int>,
                     rfl::Field<"c", int>>>());
  static_assert(
      !rfl::internal::no_duplicate_field_names<
          rfl::Tuple<rfl::Field<"b", int>, rfl::Field<"a", int>,
                     rfl::Field<"b", double>>>());
  SUCCEED();
}
}  // namespace test_literal
