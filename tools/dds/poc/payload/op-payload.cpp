// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "op-payload.h"
#include "realsense_typesPubSubTypes.h"

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic-writer.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/Topic.hpp>


namespace poc {


op_payload::op_payload( realsense::OP_payload && rhs )
    : _op( std::move( rhs.op() ) )
    , _id( std::move( rhs.id() ) )
    , _data( std::move( rhs.data() ) )
{
}


op_payload & op_payload::operator=( realsense::OP_payload && rhs )
{
    _op = std::move( rhs.op() );
    _id = std::move( rhs.id() );
    _data = std::move( rhs.data() );
    return *this;
}


/*static*/ std::shared_ptr< realdds::dds_topic >
op_payload::create_topic( std::shared_ptr< realdds::dds_participant > const & participant,
                          std::string const & topic_name )
{
    return std::make_shared< realdds::dds_topic >( participant,
                                                   eprosima::fastdds::dds::TypeSupport( new op_payload::type ),
                                                   topic_name.c_str() );
}


/*static*/ bool op_payload::take_next( realdds::dds_topic_reader & reader,
                                       op_payload * output,
                                       eprosima::fastdds::dds::SampleInfo * info )
{
    realsense::OP_payload raw_data;
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
    DDS_API_CALL_THROW( "op_payload::take_next", status );
}


realsense::OP_payload op_payload::to_raw()
{
    realsense::OP_payload raw_msg;
    raw_msg.op( _op );
    raw_msg.id( _id );
    raw_msg.data( _data );
    return raw_msg;
}


void op_payload::write_to( realdds::dds_topic_writer & writer )
{
    auto raw_msg = to_raw();
    bool success = DDS_API_CALL( writer.get()->write( &raw_msg ) );
    if( ! success )
    {
        LOG_ERROR( "Error writing message" );
    }
}


}  // namespace poc
