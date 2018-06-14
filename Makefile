all: app

doom_text.o: doom_text.c
	gcc -c doom_text.c

app: doom_text.o
	gcc doom_text.o -o app -lncurses -lm

clean:
	rm -f app *.o
