/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BINNED_LIVEGRAPH_HH
#define BINNED_LIVEGRAPH_HH

#include <vector>
#include <string>

#include "graph.hh"

class BinnedLiveGraph
{
private:
    Graph graph_;

    unsigned int bin_width_;
    std::vector<unsigned int> bytes_this_bin_;
    uint64_t current_bin_;

    uint64_t advance( void );

    double logical_width( void ) const;

public:
    BinnedLiveGraph( const std::string & name, const unsigned int num_lines );

    void set_style( const unsigned int num, const float red, const float green, const float blue,
                    const float alpha, const bool fill );

    void add_bytes_now( const unsigned int num, const unsigned int amount );

    void start_background_animation( void );
};

#endif /* BINNED_LIVEGRAPH_HH */
