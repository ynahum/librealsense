// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include "realsense_types.h"

#include <librealsense2/utilities/string/slice.h>
#include <memory>
#include <array>


namespace eprosima {
namespace fastdds {
namespace dds {
struct SampleInfo;
}
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {
class dds_participant;
class dds_topic;
class dds_topic_reader;
class dds_topic_writer;
}  // namespace realdds


namespace realsense {
class OP_payloadPubSubType;
}


namespace poc {


class op_payload
{
public:
    uint64_t _op;
    uint64_t _id;
    std::array< uint64_t, 5 > _data;

public:
    using type = realsense::OP_payloadPubSubType;

    enum op_t : unsigned
    {
        NOOP,
        ERROR,
        SYNC,
        EXIT
    };

    // By default, invalid
    op_payload()
        : _op( NOOP )
    {
    }

    bool is_valid() const { return _op != NOOP; }
    void invalidate() { _op = NOOP; }

    // Disable copy
    op_payload( const op_payload & ) = delete;
    op_payload & operator=( const op_payload & ) = delete;

    // Move is OK
    op_payload( op_payload && ) = default;
    op_payload( realsense::OP_payload && );
    op_payload & operator=( op_payload && ) = default;
    op_payload & operator=( realsense::OP_payload && );

    // Topic creation
    static std::shared_ptr< realdds::dds_topic >
        create_topic( std::shared_ptr< realdds::dds_participant > const & participant, std::string const & topic_name );

    // This helper method will take the next sample from a reader.
    //
    // Returns true if successful. Make sure you still check is_valid() in case the sample info isn't!
    // Returns false if no more data is available.
    // Will throw if an unexpected error occurs.
    //
    static bool take_next( realdds::dds_topic_reader &,
                           op_payload * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    // WARNING: this moves the message content!
    realsense::OP_payload to_raw();
    // WARNING: this moves the message content!
    void write_to( realdds::dds_topic_writer & );
};


}  // namespace poc
