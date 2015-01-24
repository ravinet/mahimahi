#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include <xcb/present.h>

#include "display.hh"

using namespace std;

template <typename T>
inline T * notnull( const string & context, T * const x )
{
  return x ? x : throw runtime_error( context + ": returned null pointer" );
}

struct free_deleter
{
  template <typename T>
  void operator() ( T * x ) const { free( x ); }
};

void XCBObject::check_noreply( const string & context, const xcb_void_cookie_t & cookie )
{
  unique_ptr<xcb_generic_error_t, free_deleter> error { xcb_request_check( connection_.get(), cookie ) };
  if ( error ) {
    throw runtime_error( context + ": returned error " + to_string( error->error_code ) );
  }
}

const xcb_screen_t * XCBObject::default_screen( void ) const
{
  return notnull( "xcb_setup_roots_iterator",
		  xcb_setup_roots_iterator( notnull( "xcb_get_setup",
						     xcb_get_setup( connection_.get() ) ) ).data );  
}

XCBObject::XCBObject( void )
  : connection_( notnull( "xcb_connect", xcb_connect( nullptr, nullptr ) ),
		 [] ( xcb_connection_t * connection ) { xcb_disconnect( connection ); } )
{}

XCBObject::XCBObject( XCBObject & original )
  : connection_( original.connection() )
{}


XCBObject::XCBObject( XCBObject && original )
  : connection_( move( original.connection_ ) )
{}

XWindow::XWindow( const unsigned int width, const unsigned int height )
{
  const auto screen = default_screen();

  check_noreply( "xcb_create_window_checked",
		 xcb_create_window_checked( connection().get(),
					    XCB_COPY_FROM_PARENT,
					    window_,
					    screen->root,
					    0, 0, /* top-left corner of the window */
					    width, height,
					    0, /* border size */
					    XCB_WINDOW_CLASS_INPUT_OUTPUT,
					    screen->root_visual,
					    0, nullptr ) ); /* value_mask, value_list */

  check_noreply( "xcb_present_select_input_checked",
		 xcb_present_select_input_checked( connection().get(),
						   complete_event_,
						   window_,
						   XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY ) );

  check_noreply( "xcb_present_select_input_checked",
		 xcb_present_select_input_checked( connection().get(),
						   idle_event_,
						   window_,
						   XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY ) );
}

void XWindow::map( void ) {
  check_noreply( "xcb_map_window_checked",
		 xcb_map_window_checked( connection().get(), window_ ) );
}

void XWindow::flush( void ) {
  if ( xcb_flush( connection().get() ) <= 0 ) {
    throw runtime_error( "xcb_flush: failed" );
  }
}

XWindow::~XWindow( void )
{
  try {
    check_noreply( "xcb_destroy_window_checked",
		   xcb_destroy_window_checked( connection().get(), window_ ) );
  } catch ( const exception & e ) {
    cerr << e.what() << endl;
  }
}

void XWindow::set_name( const string & name )
{
  check_noreply( "xcb_change_property_checked",
		 xcb_change_property_checked( connection().get(),
					      XCB_PROP_MODE_REPLACE,
					      window_,
					      XCB_ATOM_WM_NAME,
					      XCB_ATOM_STRING,
					      8, /* 8-bit string */
					      name.length(),
					      name.data() ) );
}

xcb_visualtype_t * XWindow::xcb_visual( void )
{
  /* get the visual ID of the window */
  xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes( connection().get(), window_ );
  unique_ptr<xcb_get_window_attributes_reply_t, free_deleter> attributes { notnull( "xcb_get_attributes_reply",
										    xcb_get_window_attributes_reply( connection().get(), cookie, nullptr ) ) };

  /* now find the corresponding visual */
  const auto screen = default_screen();

  for ( xcb_depth_iterator_t depth_it = xcb_screen_allowed_depths_iterator( screen );
	depth_it.rem;
	xcb_depth_next( &depth_it ) ) {
    for ( xcb_visualtype_iterator_t visualtype_it = xcb_depth_visuals_iterator( depth_it.data );
	  visualtype_it.rem;
	  xcb_visualtype_next( &visualtype_it ) ) {
      if ( attributes->visual == visualtype_it.data->visual_id ) {
	return visualtype_it.data;
      }
    }
  }

  throw runtime_error( "xcb_visual: no matching visualtype_t found" );
}

pair<unsigned int, unsigned int> XWindow::size( void ) const
{
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry( connection().get(), window_ );
  unique_ptr<xcb_get_geometry_reply_t, free_deleter> geometry { notnull( "xcb_get_geometry_reply",
									 xcb_get_geometry_reply( connection().get(), cookie, nullptr ) ) };
  return make_pair( geometry->width, geometry->height );
}

XPixmap::XPixmap( XWindow & window )
  : XCBObject( window ),
    pixmap_( xcb_generate_id( connection().get() ) ),
    visual_( window.xcb_visual() ),
    size_( window.size() )
{
  check_noreply( "xcb_create_pixmap_checked",
		 xcb_create_pixmap_checked( connection().get(),
					    default_screen()->root_depth,
					    pixmap_,
					    window.xcb_window(),
					    size_.first, size_.second ) );
}

XPixmap::XPixmap( XPixmap && other )
  : XCBObject( move( other ) ),
    pixmap_( other.pixmap_ ),
    visual_( other.visual_ ),
    size_( other.size_ )
{}

XPixmap & XPixmap::operator=( XPixmap && other )
{
  this->~XPixmap();
  new (this) XPixmap( move( other ) );
  return *this;
}

XPixmap::~XPixmap()
{
  if ( not connection() ) {
    /* moved away */
    return;
  }

  try {
    check_noreply( "xcb_free_pixmap_checked",
		   xcb_free_pixmap_checked( connection().get(), pixmap_ ) );
  } catch ( const exception & e ) {
    cerr << e.what() << endl;
  }
}

void XWindow::present( const XPixmap & pixmap, const unsigned int divisor, const unsigned int remainder )
{
  while ( not complete_ ) {
    event_loop();
  }

  check_noreply( "xcb_present_pixmap_checked",
		 xcb_present_pixmap_checked( connection().get(),
					     window_,
					     pixmap.xcb_pixmap(),
					     0, /* serial */
					     0, /* valid */
					     0, /* update */
					     0, 0, /* offsets */
					     0, /* target_crtc */
					     0, /* wait_fence */
					     0, /* idle_fence */
					     0, /* options */
					     0, /* target_msc */
					     divisor, /* divisor */
					     remainder, /* remainder */
					     0, /* notifies_len */
					     nullptr /* notifies */ ) );

  complete_ = false;
  idle_ = false;

  while ( not idle_ ) {
    event_loop();
  }
}

void XWindow::event_loop( void )
{
  xcb_generic_event_t * event = notnull( "xcb_wait_for_event",
					 xcb_wait_for_event( connection().get() ) );
  if ( event->response_type == XCB_GE_GENERIC ) {
    const uint16_t event_type = reinterpret_cast<xcb_ge_generic_event_t *>( event )->event_type;
    if ( event_type == (complete_event_ & 0xffff) ) {
      complete_ = true;
    } else if ( event_type == (idle_event_ & 0xffff) ) {
      idle_ = true;
    } else {
      throw runtime_error( "unexpected present event" );
    }
  } else {
    // throw runtime_error( "unexpected event" );
    // This is happening (but very rarely). Let's ignore for now. (KJW 1/23/2015)
  }
}
