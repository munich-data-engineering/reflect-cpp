module;

#ifdef REFLECTCPP_YAML
#include "rfl/yaml.hpp"
#endif

export module rfl:yaml;

export namespace rfl::yaml {
    #ifdef REFLECTCPP_YAML
    using rfl::yaml::Parser;
    using rfl::yaml::Reader;
    using rfl::yaml::Writer;
    using rfl::yaml::InputVarType;

    using rfl::yaml::load;
    using rfl::yaml::read;
    using rfl::yaml::save;
    using rfl::yaml::write;
    #endif
}
