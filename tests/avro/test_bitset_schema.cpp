// Include the schema entry point alone: it must select the packed parser.
#include <gtest/gtest.h>

#include <bitset>
#include <rfl/avro/to_schema.hpp>

namespace test_bitset_schema_avro {

struct Record {
  std::bitset<9> bits;
};

TEST(avro, bitset_standalone_schema) {
  const auto schema = rfl::avro::to_schema<Record>();
  EXPECT_NE(schema.str().find(R"("bytes")"), std::string::npos);
}

}  // namespace test_bitset_schema_avro
