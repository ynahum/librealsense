// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/flexible/flexible-reader.h>

#include <realdds/dds-topic-reader.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <librealsense2/utilities/string/shorten-json-string.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/time/timer.h>
#include <third-party/json.hpp>


namespace realdds {
namespace topics {


flexible_reader::flexible_reader( std::shared_ptr< dds_topic > const & topic )
    : _reader( std::make_shared< dds_topic_reader >( topic ) )
{
    _reader->on_subscription_matched( [this]( eprosima::fastdds::dds::SubscriptionMatchedStatus const & status ) {
        this->on_subscription_matched( status );
    } );
    _reader->on_data_available( [this]() { this->on_data_available(); } );
    _reader->run();

    // By default, unless someone else overrides with on_data(), we assume users will be waiting on the data:
    on_data( [&]( data_t & data ) {
        {
            std::lock_guard< std::mutex > lock( _data_mutex );
            _data.emplace( std::move( data ) );
        }
        _data_cv.notify_one();
    } );
}


std::string const & flexible_reader::name() const
{
    return _reader->topic()->get()->get_name();
}


void flexible_reader::wait_for_writers( int n_writers, std::chrono::seconds timeout )
{
    utilities::time::timer timer( timeout );
    while( _n_writers < n_writers )
    {
        if( timer.has_expired() )
            DDS_THROW( runtime_error, name() + " timed out waiting for " + std::to_string( n_writers ) + " writers" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
}


void flexible_reader::wait_for_data()
{
    std::unique_lock< std::mutex > lock( _data_mutex );
    if( empty() )
        _data_cv.wait( lock );
}


void flexible_reader::wait_for_data( std::chrono::seconds timeout )
{
    std::unique_lock< std::mutex > lock( _data_mutex );
    if( empty()  &&  std::cv_status::timeout == _data_cv.wait_for( lock, timeout ) )
        DDS_THROW( runtime_error, "timed out waiting for data" );
}


flexible_reader::data_t flexible_reader::read()
{
    wait_for_data();
    return pop_data();
}


flexible_reader::data_t flexible_reader::pop_data()
{
    std::lock_guard< std::mutex > lock( _data_mutex );
    data_t data = std::move( _data.front() );
    _data.pop();
    return data;
}


flexible_reader::data_t flexible_reader::read( std::chrono::seconds timeout )
{
    wait_for_data( timeout );
    return pop_data();
}


void flexible_reader::on_subscription_matched( eprosima::fastdds::dds::SubscriptionMatchedStatus const & status )
{
    _n_writers += status.current_count_change;
    LOG_DEBUG( name() << ".on_subscription_matched " << ( status.current_count_change > 0 ? "+" : "" )
                      << status.current_count_change << " -> " << _n_writers );
}


void flexible_reader::on_data_available()
{
    auto notify_time = now();
    bool got_something = false;
    while( 1 )
    {
        data_t data;
        if( ! flexible_msg::take_next( *_reader, &data.msg, &data.sample ) )
            assert( ! data.msg.is_valid() );
        if( ! data.msg.is_valid() )
        {
            if( ! got_something )
                DDS_THROW( runtime_error, "expected message not received!" );
            break;
        }
        auto & received = data.sample.reception_timestamp;
        LOG_DEBUG( name() << ".on_data_available @" << timestr( received.to_ns(), timestr::no_suffix )
                          << timestr( notify_time, received, timestr::no_suffix ) << timestr( now(), notify_time ) << " "
                          << utilities::string::shorten_json_string( data.msg.json_string() ));
        got_something = true;
        _on_data( data );
    }
}


}  // namespace topics
}  // namespace realdds
