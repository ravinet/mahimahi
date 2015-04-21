AM_CPPFLAGS = -I$(srcdir)/../util -I$(srcdir)/../http -I../protobufs $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libhttpserver.a

libhttpserver_a_SOURCES = http_proxy.hh http_proxy.cc \
        secure_socket.hh secure_socket.cc certificate.hh \
	apache_configuration.hh
