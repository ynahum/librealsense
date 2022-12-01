// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include "stream-payload.h"

#include <string>
#include <memory>
#include <thread>
#include <atomic>


namespace realdds {
class dds_participant;
class dds_topic;
class dds_topic_writer;
}  // namespace realdds


namespace poc {


class streamer
{
    std::shared_ptr< realdds::dds_topic_writer > _writer;
    std::thread _thread;
    std::atomic< bool > _is_streaming;
    uint64_t _id;

public:
    streamer( std::shared_ptr< realdds::dds_topic > const & topic, uint64_t id );
    streamer( std::shared_ptr< realdds::dds_participant > const & participant,
              std::string const & topic_name,
              uint64_t id )
        : streamer( poc::stream_payload::create_topic( participant, topic_name ), id )
    {
    }
    ~streamer() { stop_streaming(); }

    std::string const & name() const;

private:
    void start_streaming();
    void stop_streaming();
};


}  // namespace poc
