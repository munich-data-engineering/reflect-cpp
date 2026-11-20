module;

#ifdef REFLECTCPP_UBJSON
#include "rfl/ubjson.hpp"
#endif

export module rfl:ubjson;

export namespace rfl::ubjson {
    #ifdef REFLECTCPP_UBJSON
    using rfl::ubjson::Parser;
    using rfl::ubjson::Reader;
    using rfl::ubjson::Writer;
    using rfl::ubjson::InputObjectType;
    using rfl::ubjson::InputVarType;

    using rfl::ubjson::load;
    using rfl::ubjson::read;
    using rfl::ubjson::save;
    using rfl::ubjson::write;
    #endif
}
