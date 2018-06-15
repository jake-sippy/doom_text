all: app

doom_text.o: doom_text.c
	gcc -c -g -Wall doom_text.c

app: doom_text.o
	gcc -g -Wall doom_text.o -o app -lncurses -lm

clean:
	rm -f app *.o
