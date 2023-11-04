
CC=gcc

app:app.o
	$(CC) -o app app.o
app.o:app.c
	$(CC) -c app.c -I../
clean:
	rm -f app app.o