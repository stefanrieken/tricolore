all:
	gcc *.c -o tricolore `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

clean:
	rm -f *.o tricolore
