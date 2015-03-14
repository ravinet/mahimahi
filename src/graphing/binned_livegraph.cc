/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cmath>
#include <cassert>

#include "binned_livegraph.hh"
#include "timestamp.hh"
#include "exception.hh"

using namespace std;

BinnedLiveGraph::BinnedLiveGraph( const string & name,
                                  const Graph::StylesType & styles,
                                  const string & y_label,
                                  const double multiplier,
                                  const bool rate_quantity,
                                  const unsigned int bin_width_ms,
                                  const function<void(int,int&)> initialize_new_bin )
    : graph_( 640, 480, name, 0, 1, styles, "time (s)", y_label ),
      bin_width_ms_( bin_width_ms ),
      value_this_bin_( styles.size() ),
      current_bin_( timestamp() / bin_width_ms_ ),
      multiplier_( multiplier ),
      rate_quantity_( rate_quantity ),
      mutex_(),
      halt_( false ),
      animation_thread_exception_(),
      animation_thread_( [&] () {
              try {
                  animation_loop();
              } catch ( ... ) {
                  animation_thread_exception_ = current_exception();
              } } ),
      initialize_new_bin_( initialize_new_bin )
{
    for ( unsigned int i = 0; i < value_this_bin_.size(); i++ ) {
        graph_.add_data_point( i, 0, 0 );
    }
}

double BinnedLiveGraph::logical_width( void ) const
{
    return max( 5.0, graph_.size().first / 100.0 );
}

void BinnedLiveGraph::animation_loop( void )
{
    while ( not halt_ ) {
        const uint64_t ts = advance();

        /* calculate "current" estimate based on partial bin */
        const double bin_width_so_far = ts % bin_width_ms_;
        vector<float> current_estimates;
        current_estimates.reserve( value_this_bin_.size() );
        for ( const auto & x : value_this_bin_ ) {
            double current_estimate = x * multiplier_;
            if ( rate_quantity_ ) {
                current_estimate /= (bin_width_so_far / 1000.0);
            }
            current_estimates.emplace_back( current_estimate );
        }

        const double bin_fraction = bin_width_so_far / double( bin_width_ms_ );
        const double confidence = pow( 1 - cos( bin_fraction * 3.14159 / 2.0 ), 2 );

        graph_.blocking_draw( ts / 1000.0, logical_width(),
                              current_estimates,
                              confidence );
    }
}

uint64_t BinnedLiveGraph::advance( void )
{
    unique_lock<mutex> ul { mutex_ };

    const uint64_t now = timestamp();

    const uint64_t now_bin = now / bin_width_ms_;

    while ( current_bin_ < now_bin ) {
        for ( unsigned int i = 0; i < value_this_bin_.size(); i++ ) {
            double value = value_this_bin_[ i ] * multiplier_;
            if ( rate_quantity_ ) {
                value /= (bin_width_ms_ / 1000.0);
            }
            graph_.add_data_point( i,
                                   (current_bin_ + 1) * bin_width_ms_ / 1000.0,
                                   value );
            initialize_new_bin_( bin_width_ms_, value_this_bin_[ i ] );
        }
        current_bin_++;
    }

    return now;
}

void BinnedLiveGraph::add_value_now( const unsigned int num, const unsigned int amount )
{
    advance();

    unique_lock<mutex> ul { mutex_ };

    if ( value_this_bin_.at( num ) < 0 ) {
        throw runtime_error( "BinnedLiveGraph: attempt to add to a default value" );
    }

    value_this_bin_.at( num ) += amount;
}

void BinnedLiveGraph::set_max_value_now( const unsigned int num, const unsigned int amount )
{
    advance();

    unique_lock<mutex> ul { mutex_ };

    if ( value_this_bin_.at( num ) < 0 ) {
        value_this_bin_.at( num ) = amount;
    } else {
        value_this_bin_.at( num ) = max( unsigned( value_this_bin_.at( num ) ), amount );
    }
}

BinnedLiveGraph::~BinnedLiveGraph()
{
    halt_ = true;
    animation_thread_.join();

    if ( animation_thread_exception_ != exception_ptr() ) {
        try {
            rethrow_exception( animation_thread_exception_ );
        } catch ( const exception & e ) {
            cerr << "BinnedLiveGraph exited from exception: ";
            print_exception( e );
        }
    }
}
