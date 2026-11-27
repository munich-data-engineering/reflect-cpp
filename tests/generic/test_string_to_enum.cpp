#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <rfl.hpp>
#include <string>
#include <string_view>

namespace test_string_to_enum {

using namespace std::string_view_literals;

enum class Color { red = 1, green = 2, blue = 3 };

enum Location { left = 0, center = 1, right = 2 };

enum class SignedEnum : int { neg = -5, zero = 0, pos = 5 };

enum class Signed64Enum : int64_t { neg = -1, zero = 0, pos = 1 };

enum class Unsigned32Enum : uint32_t { a = 1, b = 2 };

enum class NarrowEnum : uint8_t { low = 1, high = 2 };

enum class BoolEnum : bool { no = false, yes = true };

enum class Perm { read = 1, write = 2, exec = 4 };

inline Perm operator|(Perm a, Perm b) noexcept {
  return static_cast<Perm>(static_cast<int>(a) | static_cast<int>(b));
}

enum class Flags64 : uint64_t {
  none = 0,
  flag_a = 1ULL << 0,
  flag_b = 1ULL << 1
};

inline Flags64 operator|(Flags64 a, Flags64 b) noexcept {
  return static_cast<Flags64>(static_cast<uint64_t>(a) |
                              static_cast<uint64_t>(b));
}

TEST(generic, test_string_to_enum_string_view_literals) {
  const auto red = rfl::string_to_enum<Color>("red"sv);
  ASSERT_TRUE(red.has_value());
  EXPECT_EQ(*red, Color::red);

  const auto green = rfl::string_to_enum<Color>("green"sv);
  ASSERT_TRUE(green.has_value());
  EXPECT_EQ(*green, Color::green);

  const auto loc = rfl::string_to_enum<Location>("center"sv);
  ASSERT_TRUE(loc.has_value());
  EXPECT_EQ(*loc, Location::center);

  const auto invalid = rfl::string_to_enum<Color>("purple"sv);
  EXPECT_FALSE(invalid.has_value());
  EXPECT_EQ(invalid.error().what(),
            "Invalid enum value: 'purple'. Must be one of [red, green, blue].");

  const auto empty = rfl::string_to_enum<Color>(""sv);
  EXPECT_FALSE(empty.has_value());
  EXPECT_EQ(empty.error().what(),
            "Invalid enum value: ''. Must be one of [red, green, blue].");

  const auto default_sv = rfl::string_to_enum<Color>(std::string_view{});
  EXPECT_FALSE(default_sv.has_value());
  EXPECT_EQ(default_sv.error().what(),
            "Invalid enum value: ''. Must be one of [red, green, blue].");
}

TEST(generic, test_string_to_enum_non_null_terminated_substring) {
  // Raw character array with no null terminator.
  const char name_buf[] = {'g', 'r', 'e', 'e', 'n', 'X'};
  const auto from_buf =
      rfl::string_to_enum<Color>(std::string_view(name_buf, 5));
  ASSERT_TRUE(from_buf.has_value());
  EXPECT_EQ(*from_buf, Color::green);

  // Substring slice from a larger string_view.
  const std::string_view full = "prefix_blue_suffix";
  const auto from_sub = rfl::string_to_enum<Color>(full.substr(7, 4));
  ASSERT_TRUE(from_sub.has_value());
  EXPECT_EQ(*from_sub, Color::blue);

  // Numeric substring immediately followed by more digits in memory.
  const char num_buf[] = {'4', '2', '9', '9'};
  const auto from_num_sub =
      rfl::string_to_enum<Color>(std::string_view(num_buf, 2));
  ASSERT_TRUE(from_num_sub.has_value());
  EXPECT_EQ(static_cast<int>(*from_num_sub), 42);

  // Flag enum substring slice.
  const std::string_view full_flags = "XXread|writeYY";
  const auto from_flag_sub =
      rfl::string_to_enum<Perm>(full_flags.substr(2, 10));
  ASSERT_TRUE(from_flag_sub.has_value());
  EXPECT_EQ(*from_flag_sub, Perm::read | Perm::write);

  // Invalid substring reports only the sliced characters.
  const std::string_view full_invalid = "XXbadYY";
  const auto invalid_sub = rfl::string_to_enum<Color>(full_invalid.substr(2, 3));
  EXPECT_FALSE(invalid_sub.has_value());
  EXPECT_EQ(invalid_sub.error().what(),
            "Invalid enum value: 'bad'. Must be one of [red, green, blue].");
}

TEST(generic, test_string_to_enum_flag_enums) {
  const auto single = rfl::string_to_enum<Perm>("read"sv);
  ASSERT_TRUE(single.has_value());
  EXPECT_EQ(*single, Perm::read);

  const auto pair = rfl::string_to_enum<Perm>("read|write"sv);
  ASSERT_TRUE(pair.has_value());
  EXPECT_EQ(*pair, Perm::read | Perm::write);

  const auto all = rfl::string_to_enum<Perm>("read|write|exec"sv);
  ASSERT_TRUE(all.has_value());
  EXPECT_EQ(*all, Perm::read | Perm::write | Perm::exec);

  const auto with_numeric = rfl::string_to_enum<Perm>("read|8"sv);
  ASSERT_TRUE(with_numeric.has_value());
  EXPECT_EQ(static_cast<int>(*with_numeric), 9);

  const auto invalid_flag = rfl::string_to_enum<Perm>("read|invalid"sv);
  EXPECT_FALSE(invalid_flag.has_value());
  EXPECT_EQ(invalid_flag.error().what(),
            "Invalid enum value: 'invalid'. Must be one of [read, write, exec].");

  EXPECT_FALSE(rfl::string_to_enum<Perm>(""sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Perm>("|"sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Perm>("|read"sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Perm>("read|"sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Perm>("read||write"sv).has_value());

  // 64-bit flag enum with high bit beyond 32-bit int range.
  const auto high_bit =
      rfl::string_to_enum<Flags64>("flag_a|1099511627776"sv);
  ASSERT_TRUE(high_bit.has_value());
  EXPECT_EQ(static_cast<uint64_t>(*high_bit), (1ULL << 0) | (1ULL << 40));
}

TEST(generic, test_string_to_enum_numeric_fallback) {
  const auto zero = rfl::string_to_enum<SignedEnum>("0"sv);
  ASSERT_TRUE(zero.has_value());
  EXPECT_EQ(*zero, SignedEnum::zero);

  const auto neg = rfl::string_to_enum<SignedEnum>("-5"sv);
  ASSERT_TRUE(neg.has_value());
  EXPECT_EQ(*neg, SignedEnum::neg);

  const auto neg_undeclared = rfl::string_to_enum<SignedEnum>("-42"sv);
  ASSERT_TRUE(neg_undeclared.has_value());
  EXPECT_EQ(static_cast<int>(*neg_undeclared), -42);

  // Full 64-bit signed range.
  const auto s64_min_str =
      std::to_string(std::numeric_limits<int64_t>::min());
  const auto s64_min = rfl::string_to_enum<Signed64Enum>(s64_min_str);
  ASSERT_TRUE(s64_min.has_value());
  EXPECT_EQ(static_cast<int64_t>(*s64_min),
            std::numeric_limits<int64_t>::min());

  // Unsigned 32-bit range beyond INT_MAX, and rejection of negative values.
  const auto u32_high = rfl::string_to_enum<Unsigned32Enum>("3000000000"sv);
  ASSERT_TRUE(u32_high.has_value());
  EXPECT_EQ(static_cast<uint32_t>(*u32_high), 3000000000U);
  EXPECT_FALSE(rfl::string_to_enum<Unsigned32Enum>("-1"sv).has_value());

  // Out-of-range values are rejected.
  EXPECT_FALSE(
      rfl::string_to_enum<Color>("99999999999999999999999999"sv).has_value());
  EXPECT_FALSE(
      rfl::string_to_enum<Signed64Enum>("99999999999999999999999999"sv)
          .has_value());

  // Trailing garbage, leading whitespace, and leading '+' are rejected.
  EXPECT_FALSE(rfl::string_to_enum<Color>("1abc"sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Color>("1 "sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Color>(" 1"sv).has_value());
  EXPECT_FALSE(rfl::string_to_enum<Color>("+1"sv).has_value());

  // Permissive narrow enum preserves legacy int-cast behavior.
  const auto narrow = rfl::string_to_enum<NarrowEnum>("300"sv);
  ASSERT_TRUE(narrow.has_value());
  EXPECT_EQ(static_cast<int>(*narrow), 44);

  // Enum with bool underlying type works for both names and numeric fallback.
  const auto bool_name = rfl::string_to_enum<BoolEnum>("yes"sv);
  ASSERT_TRUE(bool_name.has_value());
  EXPECT_EQ(*bool_name, BoolEnum::yes);

  const auto bool_num = rfl::string_to_enum<BoolEnum>("1"sv);
  ASSERT_TRUE(bool_num.has_value());
  EXPECT_EQ(*bool_num, BoolEnum::yes);

  EXPECT_FALSE(rfl::string_to_enum<BoolEnum>("invalid"sv).has_value());
}

TEST(generic, test_string_to_enum_names_only) {
  const auto valid = rfl::string_to_enum<Color, true>("green"sv);
  ASSERT_TRUE(valid.has_value());
  EXPECT_EQ(*valid, Color::green);

  EXPECT_FALSE((rfl::string_to_enum<Color, true>("2"sv).has_value()));
  EXPECT_FALSE((rfl::string_to_enum<Color, true>("4"sv).has_value()));
  EXPECT_FALSE((rfl::string_to_enum<Color, true>("-1"sv).has_value()));

  const auto valid_flags = rfl::string_to_enum<Perm, true>("read|exec"sv);
  ASSERT_TRUE(valid_flags.has_value());
  EXPECT_EQ(*valid_flags, Perm::read | Perm::exec);

  EXPECT_FALSE((rfl::string_to_enum<Perm, true>("read|8"sv).has_value()));
}

TEST(generic, test_string_to_enum_caller_types) {
  // const char* literal and variable.
  const char* cstr = "red";
  ASSERT_TRUE(rfl::string_to_enum<Color>("red").has_value());
  EXPECT_EQ(*rfl::string_to_enum<Color>("red"), Color::red);
  ASSERT_TRUE(rfl::string_to_enum<Color>(cstr).has_value());
  EXPECT_EQ(*rfl::string_to_enum<Color>(cstr), Color::red);

  // std::string lvalue, const reference, and rvalue.
  std::string str = "green";
  const std::string& cref = str;
  ASSERT_TRUE(rfl::string_to_enum<Color>(str).has_value());
  EXPECT_EQ(*rfl::string_to_enum<Color>(str), Color::green);
  ASSERT_TRUE(rfl::string_to_enum<Color>(cref).has_value());
  EXPECT_EQ(*rfl::string_to_enum<Color>(cref), Color::green);
  const auto from_rvalue =
      rfl::string_to_enum<Color>(rfl::enum_to_string(Color::blue));
  ASSERT_TRUE(from_rvalue.has_value());
  EXPECT_EQ(*from_rvalue, Color::blue);

  // std::string_view lvalue.
  const std::string_view sv = "blue";
  ASSERT_TRUE(rfl::string_to_enum<Color>(sv).has_value());
  EXPECT_EQ(*rfl::string_to_enum<Color>(sv), Color::blue);
}

}  // namespace test_string_to_enum
