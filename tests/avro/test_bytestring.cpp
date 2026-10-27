#include <rfl.hpp>

#include "write_and_read.hpp"

namespace test_bytestring {

struct TestStruct {
  rfl::Bytestring bytestring;
};

TEST(avro, test_bytestring) {
  const auto test =
      TestStruct{.bytestring = rfl::Bytestring({std::byte{13}, std::byte{14},
                                                std::byte{15}, std::byte{16}})};

  write_and_read(test);
}

TEST(avro, test_empty_bytestring) {
  const auto bytes = rfl::avro::write(TestStruct{});
  const auto decoded = rfl::avro::read<TestStruct>(bytes);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_TRUE(decoded->bytestring.empty());
}
}  // namespace test_bytestring
