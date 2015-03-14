/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <cassert>

#include "link_queue.hh"
#include "timestamp.hh"
#include "util.hh"
#include "ezio.hh"

using namespace std;

LinkQueue::LinkQueue( const string & link_name, const string & filename, const string & logfile, const bool repeat, const bool graph_throughput, const bool graph_delay )
    : next_delivery_( 0 ),
      schedule_(),
      base_timestamp_( timestamp() ),
      packet_queue_(),
      log_(),
      throughput_graph_( nullptr ),
      delay_graph_( nullptr ),
      repeat_( repeat )
{
    assert_not_root();

    /* open filename and load schedule */
    ifstream trace_file( filename );

    if ( not trace_file.good() ) {
        throw runtime_error( filename + ": error opening for reading" );
    }

    string line;

    while ( trace_file.good() and getline( trace_file, line ) ) {
        if ( line.empty() ) {
            throw runtime_error( filename + ": invalid empty line" );
        }

        const uint64_t ms = myatoi( line );

        if ( not schedule_.empty() ) {
            if ( ms < schedule_.back() ) {
                throw runtime_error( filename + ": timestamps must be monotonically nondecreasing" );
            }
        }

        schedule_.emplace_back( ms );
    }

    if ( schedule_.empty() ) {
        throw runtime_error( filename + ": no valid timestamps found" );
    }

    if ( schedule_.back() == 0 ) {
        throw runtime_error( filename + ": trace must last for a nonzero amount of time" );
    }

    /* open logfile if called for */
    if ( not logfile.empty() ) {
        log_.reset( new ofstream( logfile ) );
        if ( not log_->good() ) {
            throw runtime_error( logfile + ": error opening for writing" );
        }

        *log_ << "# mahimahi mm-link (" << link_name << ") [" << filename << "] > " << logfile << endl;
        *log_ << "# base timestamp: " << base_timestamp_ << endl;
        const char * prefix = getenv( "MAHIMAHI_SHELL_PREFIX" );
        if ( prefix ) {
            *log_ << "# mahimahi config: " << prefix << endl;
        }
    }

    /* create graphs if called for */
    if ( graph_throughput ) {
        throughput_graph_.reset( new BinnedLiveGraph( link_name + " [" + filename + "]",
                                                      { make_tuple( 1.0, 0.0, 0.0, 0.25, true ),
                                                        make_tuple( 0.0, 0.0, 0.4, 1.0, false ),
                                                        make_tuple( 1.0, 0.0, 0.0, 0.5, false ) },
                                                      "throughput (Mbps)",
                                                      8.0 / 1000000.0,
                                                      true,
                                                      500,
                                                      [] ( int, int & x ) { x = 0; } ) );
    }

    if ( graph_delay ) {
        delay_graph_.reset( new BinnedLiveGraph( link_name + " delay [" + filename + "]",
                                                 { make_tuple( 0.0, 0.25, 0.0, 1.0, false ) },
                                                 "queueing delay (ms)",
                                                 1, false, 250,
                                                 [] ( int, int & x ) { x = -( abs( x ) ); } ) );
    }
}

LinkQueue::QueuedPacket::QueuedPacket( const std::string & s_contents, const uint64_t s_arrival_time )
    : bytes_to_transmit( s_contents.size() ),
      contents( s_contents ),
      arrival_time( s_arrival_time )
{}

void LinkQueue::read_packet( const string & contents )
{
    const uint64_t now = timestamp();

    if ( contents.size() > PACKET_SIZE ) {
        throw runtime_error( "packet size is greater than maximum" );
    }

    packet_queue_.emplace( contents, now );

    /* log it */
    if ( log_ ) {
        *log_ << now << " + " << contents.size() << endl;
    }

    /* meter it */
    if ( throughput_graph_ ) {
        throughput_graph_->add_value_now( 1, contents.size() );
    }
}

uint64_t LinkQueue::next_delivery_time( void ) const
{
    return schedule_.at( next_delivery_ ) + base_timestamp_;
}

void LinkQueue::use_a_delivery_opportunity( void )
{
    /* log the delivery opportunity */
    if ( log_ ) {
        *log_ << next_delivery_time() << " # " << PACKET_SIZE << endl;
    }

    /* meter the delivery opportunity */
    if ( throughput_graph_ ) {
        throughput_graph_->add_value_now( 0, PACKET_SIZE );
    }

    next_delivery_ = (next_delivery_ + 1) % schedule_.size();

    /* wraparound */
    if ( next_delivery_ == 0 ) {
        if ( repeat_ ) {
            base_timestamp_ += schedule_.back();
        } else {
            throw runtime_error( "LinkQueue: reached end of link recording" );
        }
    }
}

void LinkQueue::write_packets( FileDescriptor & fd )
{
    uint64_t now = timestamp();

    while ( next_delivery_time() <= now ) {
        const uint64_t this_delivery_time = next_delivery_time();

        /* burn a delivery opportunity */
        unsigned int bytes_left_in_this_delivery = PACKET_SIZE;
        use_a_delivery_opportunity();

        while ( (bytes_left_in_this_delivery > 0)
                and (not packet_queue_.empty())
                and (packet_queue_.front().arrival_time <= this_delivery_time) ) {
            packet_queue_.front().bytes_to_transmit -= bytes_left_in_this_delivery;
            bytes_left_in_this_delivery = 0;

            if ( packet_queue_.front().bytes_to_transmit <= 0 ) {
                /* restore the surplus bytes beyond what the packet requires */
                bytes_left_in_this_delivery += (- packet_queue_.front().bytes_to_transmit);

                /* log the delivery */
                if ( log_ ) {
                    *log_ << this_delivery_time << " - " << packet_queue_.front().contents.size()
                          << " " << this_delivery_time - packet_queue_.front().arrival_time << endl;
                }

                /* meter the delivery */
                if ( throughput_graph_ ) {
                    throughput_graph_->add_value_now( 2, packet_queue_.front().contents.size() );
                }

                if ( delay_graph_ ) {
                    delay_graph_->set_max_value_now( 0, this_delivery_time - packet_queue_.front().arrival_time );
                }

                /* this packet is ready to go */
                fd.write( packet_queue_.front().contents );
                packet_queue_.pop();
            }
        }
    }
}

void LinkQueue::discard_wasted_opportunities( const uint64_t now )
{
    const uint64_t discard_before = min( now,
                                         packet_queue_.empty()
                                         ? now
                                         : packet_queue_.front().arrival_time - 1 );

    while ( next_delivery_time() <= discard_before ) {
        use_a_delivery_opportunity();
    }
}

unsigned int LinkQueue::wait_time( void )
{
    const auto now = timestamp();

    /* pop wasted PDOs */
    discard_wasted_opportunities( now );

    return max( uint64_t(0), next_delivery_time() - now );
}

bool LinkQueue::pending_output( void )
{
    const auto now = timestamp();

    return (not packet_queue_.empty())
        and (packet_queue_.front().arrival_time <= next_delivery_time())
        and (next_delivery_time() <= now);
}
