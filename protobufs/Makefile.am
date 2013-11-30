source = http_record.proto

AM_CPPFLAGS = $(protobuf_CFLAGS)

SUFFIXES = .proto .pb.cc

.proto.pb.cc:
	$(AM_V_GEN)$(PROTOC) --cpp_out=. -I$(srcdir) $<

noinst_LIBRARIES = libhttprecordprotos.a

libhttprecordprotos_a_SOURCES = $(source)
nodist_libhttprecordprotos_a_SOURCES = $(source:.proto=.pb.cc) $(source:.proto=.pb.h)

BUILT_SOURCES = $(source:.proto=.pb.cc)
CLEANFILES = $(source:.proto=.pb.cc) $(source:.proto=.pb.h)

