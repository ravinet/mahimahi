#include <cmath>
#include <sstream>
#include <locale>
#include <numeric>
#include <limits>
#include <algorithm>
#include <cassert>

#include <iostream>

#include "graph.hh"

using namespace std;

Graph::GraphicContext::GraphicContext( XWindow & window )
  : pixmap( window ),
    cairo( pixmap ),
    pango( cairo )
{}

Graph::GraphicContext & Graph::current_gc( void )
{
  return gcs_[ current_gc_ ];
}

Graph::Graph( const unsigned int num_lines,
	      const unsigned int initial_width, const unsigned int initial_height, const string & title,
	      const float min_y, const float max_y )
  : window_( initial_width, initial_height ),
    gcs_( { window_, window_, window_ } ),
    current_gc_( 0 ),
    tick_font_( "Open Sans Condensed Bold 20" ),
    label_font_( "Open Sans Condensed Bold 20" ),
    x_tick_labels_(),
    y_tick_labels_(),
    styles_( num_lines ),
    data_points_( num_lines ),
    x_label_( current_gc().cairo, current_gc().pango, label_font_, "time (s)" ),
    y_label_( current_gc().cairo, current_gc().pango, label_font_, "throughput (Mbps)" ),
    info_string_(),
    info_( current_gc().cairo, current_gc().pango, label_font_, "" ),
    target_min_y_( min_y ),
    target_max_y_( max_y ),
    bottom_( min_y ),
    top_( max_y ),
    horizontal_fadeout_( cairo_pattern_create_linear( 0, 0, 190, 0 ) ),
    data_mutex_()
{
  cairo_pattern_add_color_stop_rgba( horizontal_fadeout_, 0.0, 1, 1, 1, 1 );
  cairo_pattern_add_color_stop_rgba( horizontal_fadeout_, 0.67, 1, 1, 1, 1 );
  cairo_pattern_add_color_stop_rgba( horizontal_fadeout_, 1.0, 1, 1, 1, 0 );

  window_.set_name( title );
  window_.map();
  window_.flush();
}

void Graph::set_info( const string & info )
{
  if ( info != info_string_ ) {
    info_string_ = info;
    info_ = Pango::Text( current_gc().cairo, current_gc().pango, tick_font_, info );
  }
}

void Graph::set_window( const float t, const float logical_width )
{
  std::unique_lock<std::mutex> ul { data_mutex_ };

  for ( auto & line : data_points_ ) {
    while ( (line.size() >= 2) and (line.front().first < t - logical_width - 1)
	    and (line.at( 1 ).first < t - logical_width - 1) ) {
      line.pop_front();
    }
  }

  while ( (not x_tick_labels_.empty()) and (x_tick_labels_.front().first < t - logical_width - 1) ) {
    x_tick_labels_.pop_front();
  }
}

static int to_int( const float x )
{
  return static_cast<int>( lrintf( x ) );
}

bool Graph::blocking_draw( const float t, const float logical_width,
			   const std::vector<float> & current_values, const double current_weight )
{
  assert( current_weight >= 0 );
  assert( current_weight <= 1 );
  assert( data_points_.size() == current_values.size() );

  {
    /* autoscale graph */
    float max_value = numeric_limits<float>::min();
    std::unique_lock<std::mutex> ul { data_mutex_ }; /* going to read data_points_ */

    /* look at historical data points */
    for ( unsigned int i = 0; i < data_points_.size(); i++ ) {
      for ( const auto & point : data_points_.at( i ) ) {
	if ( point.second > max_value ) {
	  max_value = point.second;
	}

	if ( point.second < target_min_y_ ) {
	  target_min_y_ = point.second; /* not expecting points below 0, but include if necessary */
	}
      }
    }

    /* look at current/provisional data points? */
    if ( current_weight > 0.75 ) {
      for ( const auto & point : current_values ) {
	if ( point > max_value ) {
	  max_value = point;
	}
      }
    }

    /* expansion? */
    if ( max_value * 1.2 > target_max_y_ ) {
      target_max_y_ = max( max_value * 1.4f, target_min_y_ + 1 );
    }

    /* contraction */
    if ( max_value * 1.8 < target_max_y_ ) {
      target_max_y_ = max( max_value * 1.5f, target_min_y_ + 1 );
    }
  }

  /* set scale for this frame (with smoothing) */
  top_ = top_ * .95 + target_max_y_ * 0.05;
  bottom_ = bottom_ * 0.95 + target_min_y_ * 0.05;

  /* get the current window size */
  const auto window_size = window_.size();

  /* do we need to resize? */
  if ( window_size != current_gc().cairo.size() ) {
    current_gc() = GraphicContext( window_ );
  }

  Cairo & cairo_ = current_gc().cairo;
  Pango & pango_ = current_gc().pango;

  /* start a new image */
  cairo_new_path( cairo_ );
  cairo_identity_matrix( cairo_ );
  cairo_rectangle( cairo_, 0, 0, window_size.first, window_size.second );
  cairo_set_source_rgba( cairo_, 1, 1, 1, 1 );
  cairo_fill( cairo_ );

  /* do we need to make a new label? */
  while ( x_tick_labels_.empty() or (x_tick_labels_.back().first < t + 1) ) { /* start when offscreen */
    const int next_label = x_tick_labels_.empty() ? to_int( t ) : x_tick_labels_.back().first + 1;

    /* add commas as appropriate */
    stringstream ss;
    ss.imbue( locale( "" ) );
    ss << fixed << next_label;

    x_tick_labels_.emplace_back( next_label, Pango::Text( cairo_, pango_, tick_font_, ss.str() ) );
  }

  /* draw the labels and vertical grid */
  for ( const auto & x : x_tick_labels_ ) {
    /* position the text in the window */
    const double x_position = window_size.first - (t - x.first) * window_size.first / logical_width;

    x.second.draw_centered_at( cairo_,
			       x_position,
			       window_size.second * 9.0 / 10.0,
			       0.85 * window_size.first / logical_width );

    cairo_set_source_rgba( cairo_, 0, 0, 0.4, 1 );
    cairo_fill( cairo_ );

    /* draw vertical grid line */
    cairo_identity_matrix( cairo_ );
    cairo_set_line_width( cairo_, 2 );
    cairo_move_to( cairo_, x_position, chart_height( bottom_, window_size.second ) );
    cairo_line_to( cairo_, x_position, chart_height( top_, window_size.second ) );
    cairo_set_source_rgba( cairo_, 0, 0, 0.4, 0.25 );
    cairo_stroke( cairo_ );
  }

  /* draw the x-axis label */
  x_label_.draw_centered_at( cairo_, 35 + window_size.first / 2, window_size.second * 9.6 / 10.0 );
  cairo_set_source_rgba( cairo_, 0, 0, 0.4, 1 );
  cairo_fill( cairo_ );

  {
    std::unique_lock<std::mutex> ul { data_mutex_ };

    /* draw the data */
    for ( unsigned int line_no = 0; line_no < data_points_.size(); line_no++ ) {
      const auto & line = data_points_.at( line_no );

      if ( line.empty() ) {
	continue;
      }

      cairo_identity_matrix( cairo_ );
      cairo_set_line_width( cairo_, 3 );

      const double x_position = window_size.first - (t - line.front().first) * window_size.first / logical_width;

      cairo_move_to( cairo_, x_position, chart_height( line.front().second, window_size.second ) );

      for ( unsigned int i = 1; i < line.size(); i++ ) {
	const double x_position = window_size.first - (t - line[ i ].first) * window_size.first / logical_width;
	cairo_line_to( cairo_, x_position, chart_height( line[ i ].second, window_size.second ) );
      }

      cairo_line_to( cairo_, window_size.first - (0) * window_size.first / logical_width,
		     chart_height( current_weight * current_values.at( line_no )
				   + (1 - current_weight) * line.back().second,
				   window_size.second ) );

      cairo_set_source_rgba( cairo_,
			     get<0>( styles_.at( line_no ) ),
			     get<1>( styles_.at( line_no ) ),
			     get<2>( styles_.at( line_no ) ),
			     get<3>( styles_.at( line_no ) ) );

      if ( get<4>( styles_.at( line_no ) ) ) {
	/* fill the curve */
      
	cairo_line_to( cairo_, window_size.first - (0) * window_size.first / logical_width,
		       chart_height( 0, window_size.second ) );
	cairo_line_to( cairo_, window_size.first - (t - line.front().first) * window_size.first / logical_width,
		       chart_height( 0, window_size.second ) );
	cairo_fill( cairo_ );
      } else {
	cairo_stroke( cairo_ );
      }
    }
  }

  /* draw the y-axis labels */

  int label_bottom = to_int( floor( bottom_ ) );
  int label_top = to_int( ceil( top_ ) );
  int label_spacing = 1;

  while ( label_spacing < (label_top - label_bottom) / 6 ) {
    label_spacing *= 2;
  }

  label_bottom = (label_bottom / label_spacing) * label_spacing;
  label_top = (label_top / label_spacing) * label_spacing;

  /* cull old labels */
  {
    auto it = y_tick_labels_.begin();
    while ( it < y_tick_labels_.end() ) {
      if ( it->intensity < 0.01 ) {
	/* delete it */
	auto it_next = it + 1;
	y_tick_labels_.erase( it );
	it = it_next;
      } else {
	it++;
      }
    }
  }

  /* find the labels we actually want on this frame */
  vector<pair<int, bool>> labels_that_belong;

  for ( int val = label_bottom; val <= label_top; val += label_spacing ) {
    if ( project_height( val ) < 0 or project_height( val ) > 1 ) {
      continue;
    }

    labels_that_belong.emplace_back( val, false );
  }

  /* adjust current labels as necessary */
  for ( auto it = y_tick_labels_.begin(); it != y_tick_labels_.end(); it++ ) {
    bool belongs = false;
    for ( auto & y : labels_that_belong ) {
      if ( it->height == y.first ) {
	assert( y.second == false ); /* don't want duplicates */
	y.second = true;
	belongs = true;
	break;
      }
    }

    if ( belongs ) {
      it->intensity = 0.95 * it->intensity + 0.05;
    } else {
      it->intensity = 0.95 * it->intensity;
    }
  }

  /* add new labels if necessary */
  for ( const auto & x : labels_that_belong ) {
    if ( x.second ) {
      /* already found */
      continue;
    }

    stringstream ss;
    ss.imbue( locale( "" ) );
    ss << fixed << x.first;

    y_tick_labels_.emplace_back( YLabel( { x.first, Pango::Text( cairo_, pango_, label_font_, ss.str() ), 0.05 } ) );
  }

  /* draw the horizontal grid lines */
  for ( const auto & x : y_tick_labels_ ) {
    cairo_identity_matrix( cairo_ );
    cairo_set_line_width( cairo_, 1 );
    cairo_move_to( cairo_, 0, chart_height( x.height, window_size.second ) );
    cairo_line_to( cairo_, window_size.first, chart_height( x.height, window_size.second ) );
    cairo_set_source_rgba( cairo_, 0, 0, 0.4, 0.25 * x.intensity );
    cairo_stroke( cairo_ );
  }

  /* draw a box to protect the tick labels */
  cairo_new_path( cairo_ );
  cairo_identity_matrix( cairo_ );
  cairo_rectangle( cairo_, 0, 0, 190, window_size.second );
  cairo_set_source( cairo_, horizontal_fadeout_ );
  cairo_fill( cairo_ );

  /* draw the info */
  info_.draw_centered_at( cairo_, window_size.first / 2, 20, window_size.first - 40 );
  cairo_set_source_rgba( cairo_, 0.4, 0, 0, 1 );
  cairo_fill( cairo_ );

  /* draw the y-axis label */
  y_label_.draw_centered_rotated_at( cairo_, 25, window_size.second * .4375 );
  cairo_set_source_rgba( cairo_, 0, 0, 0.4, 1 );
  cairo_fill( cairo_ );

  /* go through and paint all the labels */
  for ( const auto & x : y_tick_labels_ ) {
    x.text.draw_centered_at( cairo_, 100, chart_height( x.height, window_size.second ) );
    cairo_set_source_rgba( cairo_, 0, 0, 0.4, x.intensity );
    cairo_fill( cairo_ );
  }

  window_.present( current_gc().pixmap, gcs_.size(), current_gc_ );
  current_gc_ = (current_gc_ + 1) % gcs_.size();

  return false;
}

void Graph::set_style( const unsigned int num, const float red, const float green, const float blue,
		       const float alpha, const bool fill )
{
  if ( num < styles_.size() ) {
    styles_.at( num ) = make_tuple( red, green, blue, alpha, fill );
  }
}
