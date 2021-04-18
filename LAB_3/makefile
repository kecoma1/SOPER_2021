all: clean stream_ui

stream_ui:
	gcc stream-ui.c -o stream-ui -lrt

valgrind_test: stream_ui
	valgrind --leak-check=full ./stream-ui

clean:
	rm -f stream-ui *.o