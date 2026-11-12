module;

#ifdef REFLECTCPP_AVRO
#include "rfl/avro.hpp"
#include "rfl/avro/schema/Type.hpp"
#endif

export module rfl:avro;

export namespace rfl::avro {
    #ifdef REFLECTCPP_AVRO
    using rfl::avro::Parser;
    using rfl::avro::Reader;
    using rfl::avro::Schema;
    using rfl::avro::Writer;
    using rfl::avro::InputObjectType;
    using rfl::avro::InputVarType;

    using rfl::avro::load;
    using rfl::avro::read;
    using rfl::avro::save;
    using rfl::avro::to_json_representation;
    using rfl::avro::to_schema;
    using rfl::avro::write;

    namespace schema {
        using rfl::avro::schema::Type;
    }
    #endif
}
