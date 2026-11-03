module;

#ifdef REFLECTCPP_BSON
#include "rfl/bson.hpp"
#endif

export module rfl:bson;

export namespace rfl::bson {
    #ifdef REFLECTCPP_BSON
    using rfl::bson::Parser;
    using rfl::bson::Reader;
    using rfl::bson::Writer;
    using rfl::bson::InputObjectType;
    using rfl::bson::InputVarType;

    using rfl::bson::load;
    using rfl::bson::read;
    using rfl::bson::save;
    using rfl::bson::write;
    #endif
}
