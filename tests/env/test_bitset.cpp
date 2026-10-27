#include <bitset>
#include <cstdlib>
#include <rfl/env.hpp>

#include "write_and_read.hpp"

namespace test_env_bitset {

struct Config {
  std::bitset<9> bits;
};

TEST(env, bitset) {
  const test_env::ScopedEnvironment environment;
  const Config input{std::bitset<9>{257}};
  rfl::env::write(input);
  const auto parsed = rfl::env::read<Config>();
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->bits, input.bits);
  test_env::portable_setenv("BITS", "10000000x");
  EXPECT_FALSE(rfl::env::read<Config>());
  test_env::portable_setenv("BITS", "10000001");
  EXPECT_FALSE(rfl::env::read<Config>());
}

TEST(env, bitset_zero_length) {
  struct EmptyConfig {
    std::bitset<0> empty;
  };
  const test_env::ScopedEnvironment environment;
  test_env::portable_setenv("EMPTY", "previous");
  rfl::env::write(EmptyConfig{});
  const auto parsed = rfl::env::read<EmptyConfig>();
#ifdef _WIN32
  // The Windows CRT removes environment variables assigned an empty string.
  EXPECT_EQ(std::getenv("EMPTY"), nullptr);
  ASSERT_FALSE(parsed);
  EXPECT_NE(parsed.error().what().find("EMPTY"), std::string::npos);
#else
  ASSERT_NE(std::getenv("EMPTY"), nullptr);
  EXPECT_STREQ(std::getenv("EMPTY"), "");
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->empty, std::bitset<0>{});
#endif
  test_env::portable_setenv("EMPTY", "0");
  EXPECT_FALSE(rfl::env::read<EmptyConfig>());
}

}  // namespace test_env_bitset
