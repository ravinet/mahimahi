#ifndef GRAPH_HH
#define GRAPH_HH

#include <deque>
#include <vector>
#include <mutex>

#include "display.hh"
#include "cairo_objects.hh"

class Graph
{
  struct GraphicContext
  {
    XPixmap pixmap;
    Cairo cairo;
    Pango pango;

    GraphicContext( XWindow & window );
  };

  XWindow window_;
  std::array<GraphicContext, 3> gcs_;
  unsigned int current_gc_;

  GraphicContext & current_gc( void );

  Pango::Font tick_font_;
  Pango::Font label_font_;

  struct YLabel
  {
    double height;
    Pango::Text text;
    float intensity;
  };

  std::deque<std::pair<int, Pango::Text>> x_tick_labels_;
  std::vector<YLabel> y_tick_labels_;
  std::vector<std::tuple<float, float, float, float, bool>> styles_;
  std::vector<std::deque<std::pair<float, float>>> data_points_;

  Pango::Text x_label_;
  Pango::Text y_label_;

  std::string info_string_;
  Pango::Text info_;

  float target_min_y_, target_max_y_;
  float bottom_, top_;

  float project_height( const float x ) const { return ( x - bottom_ ) / ( top_ - bottom_ ); }
  float chart_height( const float x, const unsigned int window_height ) const
  {
    return (window_height - 40) * (.825*(1-project_height( x ))+.025) + (.825 * 40);
  }

  Cairo::Pattern horizontal_fadeout_;

  std::mutex data_mutex_;

  void begin_line( const float t, const float x, const float y, const float logical_width );
  void add_segment( const float t, const float x, const float y, const float logical_width );
  void end_line( const float t, const float x, const float logical_width, const bool fill );

public:
  typedef std::vector<std::tuple<float, float, float, float, bool>> StylesType;

  Graph( const unsigned int initial_width, const unsigned int initial_height,
	 const std::string & title,
	 const float min_y, const float max_y,
	 const StylesType & styles,
	 const std::string & x_label,
	 const std::string & y_label );

  void add_data_point( const unsigned int num, const float t, const float y ) {
    std::unique_lock<std::mutex> ul { data_mutex_ };

    data_points_.at( num ).emplace_back( t, y );
  }

  bool blocking_draw( const float t, const float logical_width,
		      const std::vector<float> & current_values, const double current_weight );

  std::pair<unsigned int, unsigned int> size( void ) const { return window_.size(); }
};

#endif /* GRAPH_HH */
