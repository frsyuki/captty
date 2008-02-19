CFLAGS = -Wall -I. -g

.PHONY: all
all: partty-server partty-host partty-gate

%.o: %.cc
	$(CXX) -c $< $(CFLAGS)

partty-server: server.o emtelnet.o multiplexer.o cmd_server.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

partty-host: cmd_host.o host.o ptyshell.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

partty-gate: gate.o emtelnet.o cmd_gate.o
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@

connect: connect.c
	gcc connect.c -o connect -lresolv

clean:
	$(RM) emtelnet.o ptyshell.o
	$(RM) server.o multiplexer.o host.o gate.o
	$(RM) cmd_host.o cmd_server.o cmd_gate.o


depend:
	makedepend *.cc


