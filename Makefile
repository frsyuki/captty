CFLAGS = -Wall -I. -g -DMP_SYSTEM=select

.PHONY: all
all: partty-server partty-host partty-gate partty-scale

%.o: %.c
	$(CC) -c $< $(CFLAGS)

%.o: %.cc
	$(CXX) -c $< $(CFLAGS)

partty-server: server.o emtelnet.o multiplexer.o cmd_server.o uniext.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

partty-host: cmd_host.o host.o ptyshell.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

partty-gate: gate.o emtelnet.o cmd_gate.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

partty-scale: scale.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

connect: connect.c
	gcc connect.c -o connect -lresolv

clean:
	$(RM) emtelnet.o ptyshell.o uniext.o
	$(RM) server.o multiplexer.o host.o gate.o scale.o
	$(RM) cmd_host.o cmd_server.o cmd_gate.o


depend:
	makedepend *.cc


