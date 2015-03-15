#define CAIRO_HAS_XCB_SHM_FUNCTIONS 1

#include <stdexcept>
#include <mutex>
#include <cairo-xcb.h>

#include "cairo_objects.hh"
#include "display.hh"

using namespace std;

void Cairo::Surface::Deleter::operator() ( cairo_surface_t * x ) const
{
  cairo_surface_destroy( x );
}

void Cairo::Context::Deleter::operator() ( cairo_t * x ) const
{
  cairo_destroy( x );
}

void Cairo::Pattern::Deleter::operator() ( cairo_pattern_t * x ) const
{
  cairo_pattern_destroy( x );
}

void Pango::Font::Deleter::operator() ( PangoFontDescription * x ) const
{
  pango_font_description_free( x );
}

void Pango::Text::Deleter::operator() ( cairo_path_t * x ) const
{
  cairo_path_destroy( x );
}

Cairo::Cairo( XPixmap & pixmap )
  : surface_( pixmap ),
    context_( surface_ )
{
  check_error();
}

const pair<unsigned int, unsigned int> & Cairo::size( void ) const
{
  return surface_.size;
}

Cairo::Surface::Surface( XPixmap & pixmap )
  : size( pixmap.size() ),
    surface( cairo_xcb_surface_create( pixmap.xcb_connection(),
				       pixmap.xcb_pixmap(),
				       pixmap.xcb_visual(),
				       size.first,
				       size.second ) )
{
  check_error();
}

Cairo::Context::Context( Surface & surface )
  : context( cairo_create( surface.surface.get() ) )
{
  check_error();
}

void Cairo::Surface::check_error( void )
{
  const cairo_status_t surface_result = cairo_surface_status( surface.get() );
  if ( surface_result ) {
    throw runtime_error( string( "cairo surface error: " ) + cairo_status_to_string( surface_result ) );
  }
}

void Cairo::Context::check_error( void )
{
  const cairo_status_t context_result = cairo_status( context.get() );
  if ( context_result ) {
    throw runtime_error( string( "cairo context error: " ) + cairo_status_to_string( context_result ) );
  }
}

void Cairo::check_error( void )
{
  context_.check_error();
  surface_.check_error();
}

Pango::Pango( Cairo & cairo )
  : context_( pango_cairo_create_context( cairo ) ),
    layout_( pango_layout_new( *this ) )
{}

Pango::Font::Font( const string & description )
  : font( pango_font_description_from_string( description.c_str() ) )
{}

void Pango::set_font( const Pango::Font & font )
{
  pango_layout_set_font_description( *this, font );
}

mutex & global_pango_mutex( void )
{
  static mutex global_pango_mutex_;

  return global_pango_mutex_;
}

Pango::Text::Text( Cairo & cairo, Pango & pango, const Font & font, const string & text )
  : path_(),
    extent_( { 0, 0, 0, 0 } )
{
  unique_lock<mutex> ul { global_pango_mutex() };

  cairo_identity_matrix( cairo );
  cairo_new_path( cairo );

  pango.set_font( font );

  pango_layout_set_text( pango, text.data(), text.size() );

  pango_cairo_layout_path( cairo, pango );

  path_.reset( cairo_copy_path( cairo ) );

  /* get logical extents */
  PangoRectangle logical;
  pango_layout_get_extents( pango, nullptr, &logical );
  extent_ = { logical.x / double( PANGO_SCALE ),
	      logical.y / double( PANGO_SCALE ),
	      logical.width / double( PANGO_SCALE ),
	      logical.height / double( PANGO_SCALE ) };
}

void Pango::Text::draw_centered_at( Cairo & cairo, const double x, const double y, const double max_width ) const
{
  cairo_identity_matrix( cairo );
  cairo_new_path( cairo );
  Cairo::Extent<true> my_extent = extent().to_device( cairo );

  double center_x = x - my_extent.x - my_extent.width / 2;
  double center_y = y - my_extent.y - my_extent.height / 2;

  if ( my_extent.width > max_width ) {
    const double scale_factor = max_width / my_extent.width;
    cairo_scale( cairo, scale_factor, scale_factor );
    center_x = x - my_extent.x - my_extent.width * scale_factor / 2;
    center_y = y - my_extent.y - my_extent.height * scale_factor / 2;
  }

  cairo_device_to_user( cairo, &center_x, &center_y );

  cairo_translate( cairo, center_x, center_y );

  cairo_append_path( cairo, path_.get() );
}

void Pango::Text::draw_centered_rotated_at( Cairo & cairo, const double x, const double y ) const
{
  cairo_identity_matrix( cairo );
  cairo_new_path( cairo );

  cairo_rotate( cairo, - 3.1415926 / 2.0 );

  Cairo::Extent<true> my_extent = extent().to_device( cairo );

  double center_x = x - my_extent.x - my_extent.width / 2;
  double center_y = y - my_extent.y - my_extent.height / 2;

  cairo_device_to_user( cairo, &center_x, &center_y );
  cairo_translate( cairo, center_x, center_y );
  cairo_append_path( cairo, path_.get() );
}

Cairo::Pattern::Pattern( cairo_pattern_t * pattern )
  : pattern_( pattern )
{
  const cairo_status_t pattern_result = cairo_pattern_status( pattern_.get() );
  if ( pattern_result ) {
    throw runtime_error( string( "cairo pattern error: " ) + cairo_status_to_string( pattern_result ) );
  }  
}
