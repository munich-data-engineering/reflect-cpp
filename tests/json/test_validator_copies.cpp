#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_validator_copies {

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

struct Element {
  int value;
  rfl::Skip<CopyCounter> counter;
};

using Elements =
    rfl::Validator<std::vector<Element>, rfl::Size<rfl::Minimum<1>>,
                   rfl::Size<rfl::Maximum<10>>>;

struct Container {
  Elements elements;
};

TEST(json, test_validator_copies_nothing_when_reading) {
  CopyCounter::copies = 0;
  const auto res =
      rfl::json::read<Container>(R"({"elements":[{"value":1},{"value":2}]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->elements->size(), 2);
  EXPECT_EQ(CopyCounter::copies, 0);
}

TEST(json, test_validator_copies_nothing_when_moved_in) {
  CopyCounter::copies = 0;
  auto elements = std::vector<Element>(3);
  const auto res = Elements::from_value(std::move(elements));
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(CopyCounter::copies, 0);
}

TEST(json, test_validator_reports_error_when_reading) {
  const auto res = rfl::json::read<Container>(R"({"elements":[]})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 'elements': Size validation failed: Value "
            "expected to be greater than or equal to 1, but got 0.");
}

}  // namespace test_validator_copies
