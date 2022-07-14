all: tricolore assembler processor

tricolore: tricolore.c
	gcc tricolore.c -o tricolore `pkg-config --libs --cflags cairo gdk-3.0 gtk+-3.0`

assembler: assembler.c
	gcc assembler.c -o assembler

processor: processor.c
	gcc processor.c -o processor

clean:
	rm -f *.o *.out tricolore assembler processor
