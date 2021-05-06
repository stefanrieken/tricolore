all: tricolore assembler processor

tricolore:
	gcc tricolore.c -o tricolore `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

assembler:
	gcc parse.c assembler.c -o assembler

processor:
	gcc parse.c processor.c -o processor

clean:
	rm -f *.o tricolore assembler processor
