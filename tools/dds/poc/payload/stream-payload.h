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
class STREAM_payloadPubSubType;
}


namespace poc {


class stream_payload
{
public:
    uint64_t _stream_id;
    uint64_t _frame_number;
    std::vector< uint8_t > _data;

public:
    using type = realsense::STREAM_payloadPubSubType;

    // By default, invalid
    stream_payload() = default;

    bool is_valid() const { return ! _data.empty(); }
    void invalidate() { _data.clear(); }

    // Disable copy
    stream_payload( const stream_payload & ) = delete;
    stream_payload & operator=( const stream_payload & ) = delete;

    // Move is OK
    stream_payload( stream_payload && ) = default;
    stream_payload( realsense::STREAM_payload && );
    stream_payload & operator=( stream_payload && ) = default;
    stream_payload & operator=( realsense::STREAM_payload && );

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
                           stream_payload * output,
                           eprosima::fastdds::dds::SampleInfo * optional_info = nullptr );

    // WARNING: this moves the message content!
    realsense::STREAM_payload to_raw();
    // WARNING: this moves the message content!
    void write_to( realdds::dds_topic_writer & );
};


}  // namespace poc
