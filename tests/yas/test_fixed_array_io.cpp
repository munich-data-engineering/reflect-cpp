#include <rfl/NoFieldNames.hpp>
#include <rfl/yas.hpp>

#include "../check_fixed_array_io.hpp"

namespace test_fixed_array_io {

static_assert(sizeof(rfl::yas::Reader::InputArrayType) ==
              sizeof(rfl::yas::Reader::IArchive*));

TEST(yas, fixed_array_size_and_roundtrip) {
  check_sizes(
      [](const auto& input) { return rfl::yas::write(input); },
      []<class T>(const auto& bytes) { return rfl::yas::read<T>(bytes); },
      sizeof(size_t));
}

TEST(yas, fixed_array_parent_metadata) {
  check_parents(
      [](const auto& input) { return rfl::yas::write(input); },
      []<class T>(const auto& bytes) { return rfl::yas::read<T>(bytes); });
  check_parents(
      [](const auto& input) {
        return rfl::yas::write<rfl::NoFieldNames>(input);
      },
      []<class T>(const auto& bytes) {
        return rfl::yas::read<T, rfl::NoFieldNames>(bytes);
      });
}

}  // namespace test_fixed_array_io
