// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "op-writer.h"

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/time/timer.h>


namespace poc {


op_writer::op_writer( std::shared_ptr< realdds::dds_topic > const & topic )
    : _writer( std::make_shared< realdds::dds_topic_writer >( topic ) )
{
    _writer->on_publication_matched( [this]( eprosima::fastdds::dds::PublicationMatchedStatus const & status ) {
        this->on_publication_matched( status );
    } );
    _writer->run();
}


std::string const & op_writer::name() const
{
    return _writer->topic()->get()->get_name();
}


void op_writer::wait_for_readers( int n_readers, std::chrono::seconds timeout )
{
    utilities::time::timer timer( timeout );
    while( _n_readers < n_readers )
    {
        if( timer.has_expired() )
            DDS_THROW( runtime_error, name() + " timed out waiting for " + std::to_string( n_readers ) + " readers" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
}


void op_writer::write( op_payload::op_t op,
                       uint64_t id,
                       uint64_t p0,
                       uint64_t p1,
                       uint64_t p2,
                       uint64_t p3,
                       uint64_t p4 )
{
    op_payload msg;
    msg._op = ( uint64_t ) op;
    msg._id = id;
    msg._data[0] = p0;
    msg._data[1] = p1;
    msg._data[2] = p2;
    msg._data[3] = p3;
    msg._data[4] = p4;

    auto write_time = realdds::now();
    using realdds::timestr;
    msg.write_to( *_writer );
    LOG_DEBUG( name() << ".write " << op << " " << id << "(" << p0 << "," << p1 << "," << p2 << "," << p3 << "," << p4 << ")"
                      << " @" << timestr( write_time, timestr::no_suffix ) << timestr( realdds::now(), write_time ) );
}


void op_writer::on_publication_matched( eprosima::fastdds::dds::PublicationMatchedStatus const & status )
{
    _n_readers += status.current_count_change;
    LOG_DEBUG( name() << ".on_publication_matched " << ( status.current_count_change > 0 ? "+" : "" )
                      << status.current_count_change << " -> " << _n_readers );
}


}  // namespace poc
