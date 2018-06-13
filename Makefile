all: app

app.o: app.c
	gcc -c app.c

app: app.o
	gcc app.o -o app -lncurses -lm

clean:
	rm -f app *.o
