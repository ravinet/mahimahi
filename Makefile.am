AM_CPPFLAGS = -I./protobufs
AM_CXXFLAGS = $(PICKY_CXXFLAGS)
SUBDIRS = protobufs man_pages
common_source = exception.hh ezio.cc ezio.hh                                   \
	file_descriptor.hh netdevice.cc netdevice.hh timestamp.cc timestamp.hh \
	child_process.hh child_process.cc signalfd.hh signalfd.cc              \
	socket.cc socket.hh address.cc address.hh                              \
	system_runner.hh system_runner.cc nat.hh nat.cc                        \
	socket_type.hh util.hh util.cc dns_proxy.hh dns_proxy.cc               \
	interfaces.hh interfaces.cc                                            \
	poller.hh poller.cc bytestream_queue.hh bytestream_queue.cc            \
	read_write_interface.hh

packet_shell_source = ferry.hh ferry.cc packetshell.cc packetshell.hh make_pipe.hh

bin_PROGRAMS = delayshell
delayshell_SOURCES = $(common_source) $(packet_shell_source) delayshell.cc \
	delay_queue.hh delay_queue.cc
delayshell_LDADD = -lrt
delayshell_LDFLAGS = -pthread

bin_PROGRAMS += linkshell
linkshell_SOURCES = $(common_source) $(packet_shell_source) linkshell.cc \
	link_queue.hh link_queue.cc
linkshell_LDADD = -lrt
linkshell_LDFLAGS = -pthread

bin_PROGRAMS += recordshell
recordshell_SOURCES = $(common_source) recordshell.cc \
    http_proxy.hh http_proxy.cc \
    http_request_parser.hh \
    http_request.hh http_request.cc \
    http_response.hh http_response.cc \
    http_header.hh http_header.cc \
    http_response_parser.hh http_response_parser.cc \
    tokenize.hh mime_type.hh mime_type.cc \
    body_parser.hh \
    chunked_parser.hh chunked_parser.cc \
    http_message.hh http_message.cc \
    http_message_sequence.hh \
    secure_socket.hh secure_socket.cc \
    certificate.hh
recordshell_LDADD = -lrt
recordshell_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
recordshell_LDADD += -lssl -lcrypto -ldl
recordshell_LDFLAGS = -pthread

bin_PROGRAMS += localproxy
localproxy_SOURCES = $(common_source) localproxy.cc \
	local_http_proxy.hh local_http_proxy.cc \
	local_http_request_parser.hh \
	local_http_request.hh local_http_request.cc \
	local_http_response.hh local_http_response.cc \
	http_header.hh http_header.cc \
	local_http_response_parser.hh local_http_response_parser.cc \
	tokenize.hh mime_type.hh mime_type.cc \
	local_body_parser.hh \
	local_chunked_parser.hh local_chunked_parser.cc \
	local_http_message.hh local_http_message.cc \
	local_http_message_sequence.hh \
	secure_socket.hh secure_socket.cc \
	certificate.hh \
    bulk_parser.hh bulk_parser.cc \
    archive.hh archive.cc
localproxy_LDADD = -lrt
localproxy_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
localproxy_LDADD += -lssl -lcrypto -ldl
localproxy_LDFLAGS = -pthread

bin_PROGRAMS += replayshell
replayshell_SOURCES = $(common_source) replayshell.cc \
	temp_file.hh temp_file.cc \
	web_server.hh web_server.cc \
	apache_configuration.hh \
	http_header.cc http_header.hh
replayshell_LDADD = -lrt
replayshell_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
replayshell_LDFLAGS = -pthread

bin_PROGRAMS += nph-replayserver.cgi
nph_replayserver_cgi_SOURCES = $(common_source) replayserver.cc \
	http_header.cc http_header.hh \
	http_message.cc http_message.hh
nph_replayserver_cgi_LDADD = -lrt
nph_replayserver_cgi_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
nph_replayserver_cgi_LDFLAGS = -pthread

bin_PROGRAMS += makebulkreply
makebulkreply_SOURCES = $(common_source) makebulkreply.cc http_message.cc http_message.hh \
    http_header.cc http_header.hh
makebulkreply_LDADD = -lrt
makebulkreply_LDADD += protobufs/libhttprecordprotos.a -lm $(protobuf_LIBS)
makebulkreply_LDFLAGS = -pthread

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/delayshell
	chmod u+s $(DESTDIR)$(bindir)/delayshell
	chown root $(DESTDIR)$(bindir)/linkshell
	chmod u+s $(DESTDIR)$(bindir)/linkshell
	chown root $(DESTDIR)$(bindir)/recordshell
	chmod u+s $(DESTDIR)$(bindir)/recordshell
	chown root $(DESTDIR)$(bindir)/localproxy
	chmod u+s $(DESTDIR)$(bindir)/localproxy
	chown root $(DESTDIR)$(bindir)/replayshell
	chmod u+s $(DESTDIR)$(bindir)/replayshell
