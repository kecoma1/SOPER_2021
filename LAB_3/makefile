all: clean stream_ui stream_server stream_client

stream_ui:
	gcc stream-ui.c -o stream-ui -lrt -pthread

stream_server:
	gcc stream-server.c -o stream-server -lrt -pthread

stream_client:
	gcc stream-client.c -o stream-client -lrt -pthread

valgrind_test: stream_ui stream_server stream_client
	valgrind --leak-check=full ./stream-ui a.txt b.txt

clean:
	rm -f stream-ui stream-client stream-server *.o