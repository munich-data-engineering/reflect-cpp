#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_unknown_options {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

struct Database {
  std::string host;
  int port;
  std::optional<bool> ssl;
};

struct Query {
  std::string sql;
  std::optional<bool> verbose;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_unknown_option_no_extra_fields) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"--sql=x", "--nope", "1", "in.txt"},
           {"--sql=x", "--nope=1", "in.txt"},
           {"--sql=x", "in.txt", "--nope"},
           {"--nope", "--sql=x", "in.txt"}}) {
    const auto result = read<Query, rfl::NoExtraFields>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()), "Unknown option '--nope'.");
  }
}

struct WithDatabase {
  std::string sql;
  Database database;
};

TEST(cli, test_long_unknown_nested_option_no_extra_fields) {
  const auto result = read<WithDatabase, rfl::NoExtraFields>(
      {"--sql=x", "--database.host=h", "--database.port=1", "--database.nope",
       "1"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Unknown option '--database.nope'.");
}

struct Flags {
  bool verbose;
  std::optional<bool> dry_run;
  rfl::Short<"q", std::optional<bool>> quiet;
  rfl::Description<"Colorize output", std::optional<bool>> color;
  Database database;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_long_known_options_no_extra_fields) {
  const auto result = read<Flags, rfl::NoExtraFields>(
      {"--verbose", "in.txt", "--database.host=h", "--database.port=1", "-q"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().verbose);
  EXPECT_EQ(result.value().quiet(), true);
  EXPECT_EQ(result.value().database.host, "h");
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

struct Server {
  std::string host;
  int port;
};

struct WithMaps {
  std::map<std::string, std::string> labels;
  std::map<std::string, Server> servers;
  rfl::Positional<std::string> input_file;
};

TEST(cli, test_unknown_option_map_keys) {
  // Any key of a map is an option, but the fields of a struct in a map are
  // known.
  const auto result = read<WithMaps, rfl::NoExtraFields>(
      {"--labels.env=prod", "--servers.eu.host=h", "--servers.eu.port=1",
       "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().labels.at("env"), "prod");
  EXPECT_EQ(result.value().servers.at("eu").port, 1);

  const auto unknown = read<WithMaps, rfl::NoExtraFields>(
      {"--servers.eu.nope=1", "in.txt"});
  ASSERT_FALSE(unknown);
  EXPECT_EQ(std::string(unknown.error().what()),
            "Unknown option '--servers.eu.nope'.");
}

struct WithExtraFields {
  std::string name;
  rfl::ExtraFields<std::string> extra;
};

TEST(cli, test_extra_fields_no_extra_fields) {
  // rfl::ExtraFields takes every option that matches no other field.
  const auto result = read<WithExtraFields, rfl::NoExtraFields>(
      {"--name=a", "--color=red"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().name, "a");
  EXPECT_EQ(result.value().extra.at("color"), "red");
}

struct Node {
  std::string name;
  std::optional<rfl::Box<Node>> child;
};

TEST(cli, test_unknown_option_recursive_struct) {
  const auto known = read<Node, rfl::NoExtraFields>(
      {"--name=a", "--child.name=b"});
  ASSERT_TRUE(known) << known.error().what();
  EXPECT_EQ(known.value().child.value()->name, "b");

  const auto unknown =
      read<Node, rfl::NoExtraFields>({"--name=a", "--nope=1"});
  ASSERT_FALSE(unknown);
  EXPECT_EQ(std::string(unknown.error().what()), "Unknown option '--nope'.");
}

struct WithDatabaseAndExtra {
  Database database;
  rfl::ExtraFields<std::string> extra;
};

TEST(cli, test_unknown_subkey_of_scalar_and_map_and_struct) {
  // Dotted sub-key under a scalar field must not bypass NoExtraFields.
  const auto sub_scalar = read<Query, rfl::NoExtraFields>(
      {"--sql=x", "--sql.nope=1", "in.txt"});
  ASSERT_FALSE(sub_scalar);
  EXPECT_EQ(std::string(sub_scalar.error().what()),
            "Unknown option '--sql.nope'.");

  // Dotted sub-key under a scalar map value must not bypass NoExtraFields.
  const auto sub_map = read<WithMaps, rfl::NoExtraFields>(
      {"--labels.env=prod", "--labels.env.nope=1", "in.txt"});
  ASSERT_FALSE(sub_map);
  EXPECT_EQ(std::string(sub_map.error().what()),
            "Unknown option '--labels.env.nope'.");

  // Bare nested-struct option must be rejected under NoExtraFields.
  const auto bare_struct = read<WithDatabase, rfl::NoExtraFields>(
      {"--sql=x", "--database=garbage", "--database.host=h",
       "--database.port=1"});
  ASSERT_FALSE(bare_struct);
  EXPECT_EQ(std::string(bare_struct.error().what()),
            "Unknown option '--database'.");

  // Unknown sub-key of a sibling nested struct next to ExtraFields must be
  // rejected by name, not swallowed by ExtraFields.
  const auto sibling_struct = read<WithDatabaseAndExtra, rfl::NoExtraFields>(
      {"--database.host=h", "--database.port=1", "--database.nope", "1"});
  ASSERT_FALSE(sibling_struct);
  EXPECT_EQ(std::string(sibling_struct.error().what()),
            "Unknown option '--database.nope'.");

  // Sub-key under a scalar field beside ExtraFields<rfl::Generic> must be
  // rejected by name, not swallowed by ExtraFields.
  struct ScalarAndGenericExtra {
    int port = 80;
    rfl::ExtraFields<rfl::Generic> extra;
  };
  const auto scalar_sub = read<ScalarAndGenericExtra, rfl::NoExtraFields>(
      {"--port=80", "--port.foo=1"});
  ASSERT_FALSE(scalar_sub);
  EXPECT_EQ(std::string(scalar_sub.error().what()),
            "Unknown option '--port.foo'.");

  // Unknown sub-key of a nested struct that itself contains ExtraFields must
  // also be rejected by name, not swallowed by the inner ExtraFields.
  struct OuterWithNestedExtra {
    WithDatabaseAndExtra inner;
  };
  const auto nested_extra = read<OuterWithNestedExtra, rfl::NoExtraFields>(
      {"--inner.database.host=h", "--inner.database.port=1",
       "--inner.database.nope=1"});
  ASSERT_FALSE(nested_extra);
  EXPECT_EQ(std::string(nested_extra.error().what()),
            "Unknown option '--inner.database.nope'.");
}

}  // namespace test_unknown_options
