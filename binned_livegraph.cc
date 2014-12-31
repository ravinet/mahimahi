/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cmath>
#include <cassert>

#include "binned_livegraph.hh"
#include "timestamp.hh"

using namespace std;

BinnedLiveGraph::BinnedLiveGraph( const std::string & name, const Graph::StylesType & styles )
    : graph_( 640, 480, name, 0, 1, styles ),
      bin_width_( 500 ),
      bytes_this_bin_( styles.size() ),
      current_bin_( timestamp() / bin_width_ ),
      halt_( false ),
      animation_thread_( [&] () { animation_loop(); } )
{
    for ( unsigned int i = 0; i < bytes_this_bin_.size(); i++ ) {
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
        const double bin_fraction = (ts % bin_width_) / double( bin_width_ );
        vector<float> current_estimates;
        current_estimates.reserve( bytes_this_bin_.size() );
        for ( const auto & x : bytes_this_bin_ ) {
            current_estimates.emplace_back( x / double( bin_width_ ) );
        }
        const double confidence = pow( 1 - cos( bin_fraction * 3.14159 / 2.0 ), 2 );

        graph_.blocking_draw( ts / 1000.0, logical_width(),
                              current_estimates,
                              confidence );
    }
}

uint64_t BinnedLiveGraph::advance( void )
{
    const uint64_t now = timestamp();

    const uint64_t now_bin = now / bin_width_;

    assert( current_bin_ <= now_bin );

    bool advanced = false;
    while ( current_bin_ < now_bin ) {
        for ( unsigned int i = 0; i < bytes_this_bin_.size(); i++ ) {
            graph_.add_data_point( i,
                                   (current_bin_ + 1) * bin_width_ / 1000.0,
                                   (bytes_this_bin_[ i ] * 8.0 / (bin_width_ / 1000.0)) / 1000000.0 );
            bytes_this_bin_[ i ] = 0;
        }
        current_bin_++;
        advanced = true;
    }

    if ( advanced ) {
        graph_.set_window( now / 1000.0, logical_width() );
    }

    return now;
}

void BinnedLiveGraph::add_bytes_now( const unsigned int num, const unsigned int amount )
{
    advance();
    bytes_this_bin_.at( num ) += amount;
}

BinnedLiveGraph::~BinnedLiveGraph()
{
    halt_ = true;
    animation_thread_.join();
}
