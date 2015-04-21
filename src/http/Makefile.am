AM_CPPFLAGS = -I$(srcdir)/../util -I../protobufs $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libhttp.a

libhttp_a_SOURCES = http_request_parser.hh \
        http_request.hh http_request.cc \
        http_response.hh http_response.cc \
        http_header.hh http_header.cc \
        http_response_parser.hh http_response_parser.cc \
        tokenize.hh mime_type.hh mime_type.cc \
        body_parser.hh \
        chunked_parser.hh chunked_parser.cc \
        http_message.hh http_message.cc \
        http_message_sequence.hh \
        backing_store.hh backing_store.cc
