#include <rfl/capnproto.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_capnproto {

const auto write = [](const auto& value) {
  return rfl::capnproto::write(value);
};
const auto read = []<class T>(const auto& value) {
  return rfl::capnproto::read<T>(value);
};

TEST(capnproto, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(capnproto, bitset_nested) {
  test_bitset_formats::check_nested(write, read);
}

TEST(capnproto, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_capnproto
