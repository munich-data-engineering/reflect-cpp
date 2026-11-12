module;

#ifdef REFLECTCPP_TOML
#include "rfl/toml.hpp"
#endif

export module rfl:toml;

export namespace rfl::toml {
    #ifdef REFLECTCPP_TOML
    using rfl::toml::Parser;
    using rfl::toml::Reader;
    using rfl::toml::Writer;
    using rfl::toml::InputVarType;

    using rfl::toml::load;
    using rfl::toml::read;
    using rfl::toml::save;
    using rfl::toml::write;
    #endif
}
