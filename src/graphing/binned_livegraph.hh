/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BINNED_LIVEGRAPH_HH
#define BINNED_LIVEGRAPH_HH

#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <exception>
#include <mutex>

#include "graph.hh"

class BinnedLiveGraph
{
private:
    Graph graph_;

    unsigned int bin_width_;
    std::vector<unsigned int> value_this_bin_;
    uint64_t current_bin_;

    uint64_t advance( void );

    double logical_width( void ) const;

    void animation_loop( void );

    std::mutex mutex_;
    std::atomic<bool> halt_;

    std::exception_ptr animation_thread_exception_;
    std::thread animation_thread_;

public:
    BinnedLiveGraph( const std::string & name, const Graph::StylesType & styles,
                     const std::string & x_label, const std::string & y_label );
    ~BinnedLiveGraph();

    void add_value_now( const unsigned int num, const unsigned int amount );
};

#endif /* BINNED_LIVEGRAPH_HH */
