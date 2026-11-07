#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_tagged_union_copies {

struct CopyCounter {
  static inline int copies = 0;

  CopyCounter() = default;
  CopyCounter(const CopyCounter&) { ++copies; }
  CopyCounter(CopyCounter&&) noexcept = default;
  CopyCounter& operator=(const CopyCounter&) {
    ++copies;
    return *this;
  }
  CopyCounter& operator=(CopyCounter&&) noexcept = default;
  ~CopyCounter() = default;
};

struct Circle {
  rfl::Literal<"Circle"> shape;
  double radius;
  rfl::Skip<CopyCounter> counter;
};

struct Square {
  rfl::Literal<"Square"> shape;
  double width;
  rfl::Skip<CopyCounter> counter;
};

using Shapes = rfl::TaggedUnion<"shape", Circle, Square>;

TEST(json, test_tagged_union_copies_nothing_when_writing) {
  const auto circle = Shapes(Circle{.radius = 2.0});
  CopyCounter::copies = 0;
  const auto json_string = rfl::json::write(circle);
  EXPECT_EQ(json_string, R"({"shape":"Circle","radius":2.0})");
  EXPECT_EQ(CopyCounter::copies, 0);
}

}  // namespace test_tagged_union_copies
