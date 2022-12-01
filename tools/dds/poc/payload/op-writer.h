// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once


#include "op-payload.h"

#include <string>
#include <memory>
#include <chrono>


namespace eprosima {
namespace fastdds {
namespace dds {
struct PublicationMatchedStatus;
}
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {
class dds_participant;
class dds_topic;
class dds_topic_writer;
}


namespace poc {


class op_writer
{
    std::shared_ptr< realdds::dds_topic_writer > _writer;
    int _n_readers = 0;

public:
    op_writer( std::shared_ptr< realdds::dds_topic > const & topic );
    op_writer( std::shared_ptr< realdds::dds_participant > const & participant, std::string const & topic_name )
        : op_writer( op_payload::create_topic( participant, topic_name ) )
    {
    }

    std::string const & name() const;

    void wait_for_readers( int n_readers, std::chrono::seconds timeout = std::chrono::seconds( 3 ) );

    void write( op_payload::op_t,
                uint64_t id,
                uint64_t p0 = 0,
                uint64_t p1 = 0,
                uint64_t p2 = 0,
                uint64_t p3 = 0,
                uint64_t p4 = 0 );

private:
    void on_publication_matched( eprosima::fastdds::dds::PublicationMatchedStatus const & );
};


}  // namespace poc
