#ifndef CHECK_BITSET_HPP_
#define CHECK_BITSET_HPP_

#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <rfl/Bytestring.hpp>
#include <string>
#include <variant>
#include <vector>

namespace test_bitset_formats {

template <class T>
struct Record {
  T bits;
  int tail = 42;
  bool operator==(const Record&) const = default;
};

template <class T, class Write, class Read>
void roundtrip(const T& input, Write write, Read read) {
  const auto decoded = read.template operator()<T>(write(input));
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
}

struct Nested {
  std::bitset<0> empty;
  std::array<std::bitset<7>, 2> fixed;
  std::vector<std::bitset<256>> descriptors;
  std::map<std::string, std::bitset<9>> lookup;
  std::optional<std::bitset<1>> optional;
  std::variant<int, std::bitset<64>> choice;
  int tail;
  bool operator==(const Nested&) const = default;
};

template <class Write, class Read>
void check_nested(Write write, Read read) {
  const Nested input{{},
                     {std::bitset<7>{1}, std::bitset<7>{64}},
                     {std::bitset<256>{}.set(255), std::bitset<256>{}.set()},
                     {{"a", std::bitset<9>{257}}, {"b", {}}},
                     std::bitset<1>{1},
                     std::bitset<64>{}.set(63),
                     42};
  roundtrip(input, write, read);
}

// Verify compatibility independently of reflect-cpp's reader/writer pair.
template <std::size_t N, class Write, class Read, class NativeWrite,
          class NativeRead>
void check_native(Write write, Read read, NativeWrite native_write,
                  NativeRead native_read) {
  using Bits = std::bitset<N>;
  std::vector<Bits> patterns{Bits{}, Bits{}.set()};
  for (std::size_t i = 0; i < N; ++i) patterns.push_back(Bits{}.set(i));
  Bits alternating;
  for (std::size_t i = 0; i < N; i += 2) alternating.set(i);
  patterns.push_back(alternating);
  patterns.push_back(~alternating);
  for (const auto& bits : patterns) {
    SCOPED_TRACE(bits.to_string());
    const auto expected = native_write(bits);
    const auto actual = write(bits);
    EXPECT_EQ(actual, expected);
    const auto decoded = read.template operator()<Bits>(expected);
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_EQ(*decoded, bits);
    EXPECT_EQ(native_read.template operator()<Bits>(actual), bits);
  }
  const auto bytes = write(Bits{}.set());
  for (std::size_t size = 0; size < bytes.size(); ++size) {
    SCOPED_TRACE(size);
    auto truncated = bytes;
    truncated.resize(size);
    EXPECT_FALSE(read.template operator()<Bits>(truncated));
  }
}

template <class Write, class Read, class NativeWrite, class NativeRead>
void check_native_sizes(Write write, Read read, NativeWrite native_write,
                        NativeRead native_read) {
  check_native<0>(write, read, native_write, native_read);
  check_native<1>(write, read, native_write, native_read);
  check_native<7>(write, read, native_write, native_read);
  check_native<8>(write, read, native_write, native_read);
  check_native<9>(write, read, native_write, native_read);
  check_native<64>(write, read, native_write, native_read);
  check_native<256>(write, read, native_write, native_read);
}

template <std::size_t N, class Write, class Read>
void check_size(Write write, Read read) {
  SCOPED_TRACE(N);
  using Bits = std::bitset<N>;
  std::vector<Bits> patterns{Bits{}, Bits{}.set()};
  Bits alternating;
  for (std::size_t i = 0; i < N; i += 2) alternating.set(i);
  patterns.push_back(alternating);
  patterns.push_back(~alternating);
  for (std::size_t i = 0; i < N; ++i) patterns.push_back(Bits{}.set(i));
  for (const auto& bits : patterns) {
    SCOPED_TRACE(bits.to_string());
    roundtrip(Record<Bits>{bits}, write, read);
  }
}

template <class Write, class Read>
void check_sizes(Write write, Read read) {
  check_size<0>(write, read);
  check_size<1>(write, read);
  check_size<7>(write, read);
  check_size<8>(write, read);
  check_size<9>(write, read);
  check_size<64>(write, read);
  check_size<256>(write, read);
}

template <class Write, class Read>
void check_text(Write write, Read read) {
  const Record<std::bitset<9>> bits{std::bitset<9>{258}};
  const auto text = read.template operator()<Record<std::string>>(write(bits));
  ASSERT_TRUE(text) << text.error().what();
  EXPECT_EQ(text->bits, "100000010");
  for (const std::string bad : {"", "1", "1000000010", "10000000x", " 00000001",
                                "10000000\n", "100000001\n"}) {
    EXPECT_FALSE(read.template operator()<Record<std::bitset<9>>>(
        write(Record<std::string>{bad})))
        << bad;
  }
  EXPECT_FALSE(read.template operator()<Record<std::bitset<0>>>(
      write(Record<std::string>{"0"})));
}

template <class Representation, class Write, class Read>
void check_packed(Write write, Read read) {
  using Byte = typename Representation::value_type;
  const Record<std::bitset<9>> bits{std::bitset<9>{258}};
  const auto packed =
      read.template operator()<Record<Representation>>(write(bits));
  ASSERT_TRUE(packed) << packed.error().what();
  const Representation expected{static_cast<Byte>(0x40),
                                static_cast<Byte>(0x80)};
  EXPECT_EQ(packed->bits, expected);
  const auto restored = read.template operator()<Record<std::bitset<9>>>(
      write(Record<Representation>{expected}));
  ASSERT_TRUE(restored) << restored.error().what();
  EXPECT_EQ(*restored, bits);

  auto bad_padding = expected;
  bad_padding.back() |= static_cast<Byte>(1);
  EXPECT_FALSE(read.template operator()<Record<std::bitset<9>>>(
      write(Record<Representation>{bad_padding})));
  if constexpr (requires(Representation value) { value.resize(0); }) {
    for (const auto size : {0u, 1u, 3u}) {
      Representation bad(size);
      EXPECT_FALSE(read.template operator()<Record<std::bitset<9>>>(
          write(Record<Representation>{bad})));
    }
    const Record<std::bitset<256>> descriptor{
        std::bitset<256>{}.set(0).set(7).set(8).set(254).set(255)};
    Representation expected_descriptor(32);
    expected_descriptor[0] = static_cast<Byte>(0x81);
    expected_descriptor[1] = static_cast<Byte>(0x80);
    expected_descriptor[31] = static_cast<Byte>(0x03);
    const auto big =
        read.template operator()<Record<Representation>>(write(descriptor));
    ASSERT_TRUE(big) << big.error().what();
    EXPECT_EQ(big->bits, expected_descriptor);
    roundtrip(descriptor, write, read);
    for (const auto size : {31u, 33u}) {
      EXPECT_FALSE(read.template operator()<Record<std::bitset<256>>>(
          write(Record<Representation>{Representation(size)})));
    }
    EXPECT_FALSE(read.template operator()<Record<std::bitset<0>>>(
        write(Record<Representation>{Representation(1)})));
  }
}

}  // namespace test_bitset_formats

#endif
