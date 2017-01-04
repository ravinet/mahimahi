AM_CPPFLAGS = -I../protobufs -I$(srcdir)/../util -I$(srcdir)/../packet -I$(srcdir)/../graphing -I$(srcdir)/../http -I$(srcdir)/../httpserver $(XCBPRESENT_CFLAGS) $(PANGOCAIRO_CFLAGS) $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = mm-delay
mm_delay_SOURCES = delayshell.cc delay_queue.hh delay_queue.cc
mm_delay_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_delay_LDFLAGS = -pthread

bin_PROGRAMS += mm-delay-with-nameserver
mm_delay_with_nameserver_SOURCES = delayshell_with_nameserver.cc delay_queue.hh delay_queue.cc
mm_delay_with_nameserver_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_delay_with_nameserver_LDFLAGS = -pthread

bin_PROGRAMS += mm-loss
mm_loss_SOURCES = lossshell.cc loss_queue.hh loss_queue.cc
mm_loss_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_loss_LDFLAGS = -pthread

bin_PROGRAMS += mm-onoff
mm_onoff_SOURCES = onoffshell.cc loss_queue.hh loss_queue.cc
mm_onoff_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_onoff_LDFLAGS = -pthread

bin_PROGRAMS += mm-link
mm_link_SOURCES = linkshell.cc link_queue.hh link_queue.cc
mm_link_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a ../graphing/libgraph.a $(XCBPRESENT_LIBS) $(PANGOCAIRO_LIBS)
mm_link_LDFLAGS = -pthread

bin_PROGRAMS += mm-meter
mm_meter_SOURCES = meter.cc meter_queue.hh meter_queue.cc
mm_meter_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a ../graphing/libgraph.a $(XCBPRESENT_LIBS) $(PANGOCAIRO_LIBS)
mm_meter_LDFLAGS = -pthread

bin_PROGRAMS += mm-webrecord
mm_webrecord_SOURCES = recordshell.cc
mm_webrecord_LDADD = -lrt ../httpserver/libhttpserver.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a ../util/libutil.a $(protobuf_LIBS) $(libcrypto_LIBS) $(libssl_LIBS)
mm_webrecord_LDFLAGS = -pthread

bin_PROGRAMS += mm-phone-webrecord
mm_phone_webrecord_SOURCES = phone_recordshell.cc squid_proxy.hh squid_proxy.cc
mm_phone_webrecord_LDADD = -lrt ../httpserver/libhttpserver.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a ../util/libutil.a $(protobuf_LIBS) $(libcrypto_LIBS) $(libssl_LIBS)
mm_phone_webrecord_LDFLAGS = -pthread

bin_PROGRAMS += mm-delayshell-port-forwarded
mm_delayshell_port_forwarded_SOURCES = delayshell_with_forwarding.cc delay_queue.hh delay_queue.cc
mm_delayshell_port_forwarded_LDADD = -lrt ../util/libutil.a ../packet/libpacket.a
mm_delayshell_port_forwarded_LDFLAGS = -pthread

bin_PROGRAMS += mm-webreplay
mm_webreplay_SOURCES = replayshell.cc web_server.hh web_server.cc
mm_webreplay_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_webreplay_LDFLAGS = -pthread

bin_PROGRAMS += mm-replayserver
mm_replayserver_SOURCES = replayserver.cc
mm_replayserver_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_replayserver_LDFLAGS = -pthread

bin_PROGRAMS += mm-proxyreplay
mm_proxyreplay_SOURCES = replay_nghttp2_shell.cc web_server.hh web_server.cc reverse_proxy.cc squid_proxy.hh squid_proxy.cc
mm_proxyreplay_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_proxyreplay_LDFLAGS = -pthread

bin_PROGRAMS += mm-http1-proxyreplay
mm_http1_proxyreplay_SOURCES = replay_http_one_shell.cc web_server.hh web_server.cc reverse_proxy.cc
mm_http1_proxyreplay_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_http1_proxyreplay_LDFLAGS = -pthread

bin_PROGRAMS += mm-per-packet-delay-replay
mm_per_packet_delay_replay_SOURCES = replay_nghttp2_shell.cc web_server.hh web_server.cc reverse_proxy.cc squid_proxy.hh squid_proxy.cc
mm_per_packet_delay_replay_LDADD = -lrt ../util/libutil.a ../http/libhttp.a ../protobufs/libhttprecordprotos.a $(protobuf_LIBS)
mm_per_packet_delay_replay_LDFLAGS = -pthread

lib_LTLIBRARIES = libmod_deepcgi.la
libmod_deepcgi_la_SOURCES = mod_deepcgi.c replayserver_filename.cc
libmod_deepcgi_la_CFLAGS = -I@APACHE2_INCLUDE@ $(libapr1_CFLAGS)
libmod_deepcgi_la_CPPFLAGS = # empty

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/mm-delay
	chmod u+s $(DESTDIR)$(bindir)/mm-delay
	chown root $(DESTDIR)$(bindir)/mm-delay-with-nameserver
	chmod u+s $(DESTDIR)$(bindir)/mm-delay-with-nameserver
	chown root $(DESTDIR)$(bindir)/mm-loss
	chmod u+s $(DESTDIR)$(bindir)/mm-loss
	chown root $(DESTDIR)$(bindir)/mm-onoff
	chmod u+s $(DESTDIR)$(bindir)/mm-onoff
	chown root $(DESTDIR)$(bindir)/mm-link
	chmod u+s $(DESTDIR)$(bindir)/mm-link
	chown root $(DESTDIR)$(bindir)/mm-meter
	chmod u+s $(DESTDIR)$(bindir)/mm-meter
	chown root $(DESTDIR)$(bindir)/mm-webrecord
	chmod u+s $(DESTDIR)$(bindir)/mm-webrecord
	chown root $(DESTDIR)$(bindir)/mm-webreplay
	chmod u+s $(DESTDIR)$(bindir)/mm-webreplay
	chown root $(DESTDIR)$(bindir)/mm-proxyreplay
	chmod u+s $(DESTDIR)$(bindir)/mm-proxyreplay
	chown root $(DESTDIR)$(bindir)/mm-http1-proxyreplay
	chmod u+s $(DESTDIR)$(bindir)/mm-http1-proxyreplay
	chown root $(DESTDIR)$(bindir)/mm-delayshell-port-forwarded
	chmod u+s $(DESTDIR)$(bindir)/mm-delayshell-port-forwarded
	chown root $(DESTDIR)$(bindir)/mm-phone-webrecord
	chmod u+s $(DESTDIR)$(bindir)/mm-phone-webrecord
	chown root $(DESTDIR)$(bindir)/mm-per-packet-delay-replay
	chmod u+s $(DESTDIR)$(bindir)/mm-per-packet-delay-replay
