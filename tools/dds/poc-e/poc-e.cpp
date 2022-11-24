// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "poc-e.h"

#include <realdds/dds-participant.h>
#include <realdds/topics/flexible/flexible-reader.h>
#include <realdds/topics/flexible/flexible-writer.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>
#include <realdds/dds-log-consumer.h>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/json.h>

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>
#include <tclap/SwitchArg.h>




// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/flexible/flexible-writer.h>

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/topic/Topic.hpp>

#include <librealsense2/utilities/concurrency/concurrency.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/time/timer.h>
#include <third-party/json.hpp>


class flexible_streamer
{
    std::shared_ptr< realdds::dds_topic_writer > _writer;
    std::thread _thread;
    std::atomic< bool > _is_streaming;

public:
    flexible_streamer( std::shared_ptr< realdds::dds_topic > const & topic );
    flexible_streamer( std::shared_ptr< realdds::dds_participant > const & participant, std::string const & topic_name )
        : flexible_streamer( realdds::topics::flexible_msg::create_topic( participant, topic_name ) )
    {
    }
    ~flexible_streamer()
    {
        stop_streaming();
    }

    std::string const & name() const;

private:
    void start_streaming();
    void stop_streaming();
};


flexible_streamer::flexible_streamer( std::shared_ptr< realdds::dds_topic > const & topic )
    : _writer( std::make_shared< realdds::dds_topic_writer >( topic ) )
{
    _writer->on_publication_matched( [this]( eprosima::fastdds::dds::PublicationMatchedStatus const & status ) {
        LOG_DEBUG( name() << ".on_publication_matched " << ( status.current_count_change > 0 ? "+" : "" )
                          << status.current_count_change << " -> " << status.current_count );
        if( status.current_count )
            start_streaming();
        else
            stop_streaming();
    } );
    _writer->run();
}


std::string const & flexible_streamer::name() const
{
    return _writer->topic()->get()->get_name();
}


void flexible_streamer::start_streaming()
{
    if( _thread.joinable() )
        return;

    LOG_DEBUG( "starting streaming" );
    _is_streaming = true;
    _thread = std::thread( [this]() {
        try
        {
            std::vector< uint8_t > data;
            data.resize( 2048 );
            for( int i = 0; i < data.size(); ++i )
                data[i] = i;
            int64_t number = 0;
            while( _is_streaming )
            {
                nlohmann::json json
                    = { { "number", number++ }, { "timestamp", realdds::now().to_ns() }, { "data", data } };
                realdds::topics::flexible_msg msg( std::move( json ) );
                msg.write_to( *_writer );
                std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
            }
        }
        catch( std::exception & e )
        {
            LOG_ERROR( "error: " << e.what() );
        }
        LOG_DEBUG( "thread stopping" );
    } );
}


void flexible_streamer::stop_streaming()
{
    if( ! _thread.joinable() )
        return;

    LOG_DEBUG( "stopping streaming" );
    _is_streaming = false;
    _thread.join();
}


int main( int argc, char ** argv ) try
{
    TCLAP::CmdLine cmd( "POC embedded server", ' ' );
    TCLAP::SwitchArg debug_arg( "", "debug", "Enable debug logging", false );
    TCLAP::ValueArg< realdds::dds_domain_id > domain_arg( "d", "domain", "select domain ID to listen on", false, 0, "0-232" );
    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    // Configure the same logger as librealsense, and default to only errors by default...
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, "false" );
    defaultConf.set( el::Level::Error, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.set( el::Level::Warning, el::ConfigurationType::ToStandardOutput, debug_arg.isSet() ? "true" : "false" );
    defaultConf.set( el::Level::Info, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.set( el::Level::Debug, el::ConfigurationType::ToStandardOutput, debug_arg.isSet() ? "true" : "false" );
    defaultConf.setGlobally( el::ConfigurationType::Format, "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])" );
    el::Loggers::reconfigureLogger( "librealsense", defaultConf );
    
    // And set the DDS logger similarly
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( realdds::log_consumer::create() );
    eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );

    realdds::dds_domain_id domain = 0;
    if( domain_arg.isSet() )
    {
        domain = domain_arg.getValue();
        if( domain > 232 )
        {
            LOG_ERROR( "Invalid domain value, enter a value in the range [0, 232]" );
            return EXIT_FAILURE;
        }
    }


    auto participant = std::make_shared< realdds::dds_participant >();
    participant->init( domain, "poc-e" );

    realdds::topics::flexible_writer e2h( participant, "e2h" );
    realdds::topics::flexible_reader h2e( participant, "h2e" );

    flexible_streamer depth( participant, "depth" );
    flexible_streamer ir1( participant, "ir1" );
    flexible_streamer ir2( participant, "ir2" );

    std::map< std::string, std::function< void( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & ) > >
        ops;
    ops.emplace( "exit", []( nlohmann::json const &, eprosima::fastdds::dds::SampleInfo const & ) {
        LOG_INFO( "Exit requested" );
        exit( 0 );
    } );
    ops.emplace( "ping", [&]( nlohmann::json const & json, eprosima::fastdds::dds::SampleInfo const & sample ) {
        LOG_INFO( "Ping" );
        nlohmann::json reply = { { "id", utilities::json::get< int >( json, "id" ) },
                                 { "originate", sample.source_timestamp.to_ns() },
                                 { "receive", sample.reception_timestamp.to_ns() },
                                 { "transmit", realdds::now().to_ns() } };  // not really used
        e2h.write( std::move( reply ));
    } );

    while( 1 )
    {
        auto data = h2e.read();
        auto json = data.msg.json_data();
        auto op = utilities::json::get< std::string >( json, "op" );
        auto it = ops.find( op );
        if( it == ops.end() )
        {
            LOG_ERROR( "invalid op '" << op << "' in message; ignoring" );
            nlohmann::json reply = { { "id", utilities::json::get< int >( json, "id" ) },
                                     { "op", op },
                                     { "status", 1 },
                                     { "error", "invalid op" }};
            e2h.write( std::move( reply ) );
            continue;
        }
        it->second( json, data.sample );
    }

    return EXIT_SUCCESS;
}
catch( const TCLAP::ExitException & )
{
    LOG_ERROR( "Undefined exception while parsing command line arguments" );
    return EXIT_FAILURE;
}
catch( const TCLAP::ArgException & e )
{
    LOG_ERROR( e.what() );
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    LOG_ERROR( e.what() );
    return EXIT_FAILURE;
}


