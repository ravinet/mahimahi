AM_CPPFLAGS = -I$(srcdir)/../util $(XCBPRESENT_CFLAGS) $(XCB_CFLAGS) $(PANGOCAIRO_CFLAGS) $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libgraph.a

libgraph_a_SOURCES = cairo_objects.hh cairo_objects.cc \
        display.hh display.cc \
        graph.hh graph.cc \
        binned_livegraph.hh binned_livegraph.cc
