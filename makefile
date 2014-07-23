send_ping recv_ping: recv_ping.c recv_ping.c
	gcc recv_ping.c -o recv_ping
	gcc send_ping.c -o send_ping

clean:
	rm send_ping recv_ping
