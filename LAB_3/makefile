all: clean stream_ui stream_server stream_client

stream_ui:
	gcc stream-ui.c -o stream-ui -lrt

stream_server:
	gcc stream-server.c -o stream-server -lrt

stream_client:
	gcc stream-client.c -o stream-client -lrt

valgrind_test: stream_ui
	valgrind --leak-check=full ./stream-ui a b

clean:
	rm -f stream-ui stream-client stream-server *.o