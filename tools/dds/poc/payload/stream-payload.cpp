// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "stream-payload.h"
#include "realsense_typesPubSubTypes.h"

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace poc {


stream_payload::stream_payload( realsense::STREAM_payload && rhs )
    : _stream_id( std::move( rhs.stream_id() ) )
    , _frame_number( std::move( rhs.frame_number() ) )
    , _data( std::move( rhs.payload() ) )
{
}


stream_payload & stream_payload::operator=( realsense::STREAM_payload && rhs )
{
    _stream_id = std::move( rhs.stream_id() );
    _frame_number = std::move( rhs.frame_number() );
    _data = std::move( rhs.payload() );
    return *this;
}


/*static*/ std::shared_ptr< realdds::dds_topic >
stream_payload::create_topic( std::shared_ptr< realdds::dds_participant > const & participant,
                              std::string const & topic_name )
{
    return std::make_shared< realdds::dds_topic >( participant,
                                                   eprosima::fastdds::dds::TypeSupport( new stream_payload::type ),
                                                   topic_name.c_str() );
}


/*static*/ bool stream_payload::take_next( realdds::dds_topic_reader & reader,
                                           stream_payload * output,
                                           eprosima::fastdds::dds::SampleInfo * info )
{
    realsense::STREAM_payload raw_data;
    eprosima::fastdds::dds::SampleInfo info_;
    if( ! info )
        info = &info_;  // use the local copy if the user hasn't provided their own
    auto status = reader->take_next_sample( &raw_data, info );
    if( status == ReturnCode_t::RETCODE_OK )
    {
        // We have data
        if( output )
        {
            // Only samples for which valid_data is true should be accessed
            // valid_data indicates that the instance is still ALIVE and the `take` return an
            // updated sample
            if( ! info->valid_data )
                output->invalidate();
            else
                *output = std::move( raw_data );  // TODO - optimize copy, use dds loans
        }

        return true;
    }
    if( status == ReturnCode_t::RETCODE_NO_DATA )
    {
        // This is an expected return code and is not an error
        return false;
    }
    DDS_API_CALL_THROW( "stream_payload::take_next", status );
}


realsense::STREAM_payload stream_payload::to_raw()
{
    realsense::STREAM_payload raw_msg;
    raw_msg.stream_id( _stream_id );
    raw_msg.frame_number( _frame_number );
    raw_msg.payload( std::move( _data ) );
    return raw_msg;
}


void stream_payload::write_to( realdds::dds_topic_writer & writer )
{
    auto raw_msg = to_raw();
    bool success = DDS_API_CALL( writer.get()->write( &raw_msg ) );
    if( ! success )
    {
        LOG_ERROR( "Error writing STREAM_payload" );
    }
}


}  // namespace poc
