#ifndef DISPLAY_HH
#define DISPLAY_HH

#include <xcb/xcb.h>

#include <string>
#include <memory>

class XCBObject
{
public:
  typedef std::shared_ptr<xcb_connection_t> connection_type;
  XCBObject();
  XCBObject( XCBObject & original );
  XCBObject( XCBObject && original );
  virtual ~XCBObject() {}

  xcb_connection_t * xcb_connection( void ) { return connection_.get(); }

private:
  connection_type connection_;

protected:
  void check_noreply( const std::string & context, const xcb_void_cookie_t & cookie );
  const xcb_screen_t * default_screen( void ) const;
  const connection_type & connection( void ) const { return connection_; }
};

class XPixmap;

class XWindow : public XCBObject
{
private:
  xcb_window_t window_ = xcb_generate_id( connection().get() );
  uint32_t complete_event_ = xcb_generate_id( connection().get() );
  uint32_t idle_event_ = xcb_generate_id( connection().get() );
  bool complete_ = true, idle_ = true;

  void event_loop( void );

public:
  XWindow( const unsigned int width, const unsigned int height );
  XWindow( XCBObject & original, const unsigned int width, const unsigned int height );
  ~XWindow();

  /* set the name of the window */
  void set_name( const std::string & name );

  /* map the window on the screen */
  void map( void );

  /* present a pixmap */
  void present( const XPixmap & pixmap, const unsigned int divisor, const unsigned int remainder );

  /* flush XCB */
  void flush( void );

  /* get the window's size */
  std::pair<unsigned int, unsigned int> size( void ) const;

  /* get the underlying window */
  const xcb_window_t & xcb_window( void ) const { return window_; }

  /* get the window's visual */
  xcb_visualtype_t * xcb_visual( void );

  /* prevent copying */
  XWindow( const XWindow & other ) = delete;
  XWindow & operator=( const XWindow & other ) = delete;
};

class XPixmap : public XCBObject
{
private:
  xcb_pixmap_t pixmap_;
  xcb_visualtype_t * visual_;
  std::pair<unsigned int, unsigned int> size_;

public:
  XPixmap( XWindow & window );
  ~XPixmap();

  /* get the underlying pixmap */
  const xcb_pixmap_t & xcb_pixmap( void ) const { return pixmap_; }

  /* get the pixmap's visual */
  xcb_visualtype_t * xcb_visual( void ) const { return visual_; }

  /* get the pixmap's size */
  std::pair<unsigned int, unsigned int> size( void ) const { return size_; }

  /* prevent copying */
  XPixmap( const XPixmap & other ) = delete;
  XPixmap & operator=( const XPixmap & other ) = delete;

  /* allow moving */
  XPixmap( XPixmap && other );
  XPixmap & operator=( XPixmap && other );
};

#endif
