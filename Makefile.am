bin_PROGRAMS = partty-host partty-server partty-gate partty-scale

partty_host_SOURCES = host.cc cmd_host.cc ptyshell.cc
partty_server_SOURCES = server.cc emtelnet.cc multiplexer.cc cmd_server.cc uniext.cc
partty_gate_SOURCES = gate.cc raw_gate.cc emtelnet.cc cmd_gate.cc
partty_scale_SOURCES = scale.c

noinst_HEADERS = emtelnet.h fdtransport.h gate.h host.h multiplexer.h partty.h pty_make_raw.h ptyshell.h raw_gate.h server.h uniext.h \
	kazuhiki/basic.h kazuhiki/basic_impl.h kazuhiki/network.h kazuhiki/network_impl.h kazuhiki/parser.h \
	mp/dispatch.h mp/event.h mp/event_class.h mp/event_impl.h mp/functional.h mp/io.h mp/ios.h mp/mempool.h mp/sparse_array.h \
	mp/ios/net.h mp/ios/rw.h mp/ios/send.h mp/system/buffered_kqueue.h mp/system/buffered_kqueue_impl.h mp/system/epoll.h mp/system/epoll_impl.h mp/system/kqueue.h mp/system/kqueue_impl.h mp/system/poll.h mp/system/poll_impl.h mp/system/select.h mp/system/select_impl.h
