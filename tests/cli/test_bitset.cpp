#include <gtest/gtest.h>

#include <bitset>
#include <rfl/cli.hpp>

namespace test_cli_bitset {

struct Config {
  std::bitset<9> bits;
  std::bitset<0> empty;
};

TEST(cli, bitset) {
  const char* args[] = {"program", "--bits=100000001", "--empty="};
  const auto parsed = rfl::cli::read<Config>(3, const_cast<char**>(args));
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed->bits, std::bitset<9>{257});
  args[1] = "--bits=10000000x";
  EXPECT_FALSE(rfl::cli::read<Config>(3, const_cast<char**>(args)));
  args[1] = "--bits=10000001";
  EXPECT_FALSE(rfl::cli::read<Config>(3, const_cast<char**>(args)));
}

}  // namespace test_cli_bitset
