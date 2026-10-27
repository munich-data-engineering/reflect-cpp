// Include the schema entry point alone: it must select the packed parser.
#include <gtest/gtest.h>

#include <bitset>
#include <rfl/capnproto/to_schema.hpp>

namespace test_bitset_schema_capnproto {

struct Record {
  std::bitset<9> bits;
};

TEST(capnproto, bitset_standalone_schema) {
  const auto schema = rfl::capnproto::to_schema<Record>();
  EXPECT_NE(schema.str().find(R"(Data)"), std::string::npos);
}

}  // namespace test_bitset_schema_capnproto
