CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=199309L -std=c89 -pedantic
EXECUTABLE= master
SOURCE= source
TAXI= taxi

all: $(EXECUTABLE) $(SOURCE) $(TAXI)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

master: master.o functions.o
	$(CC) $(CFLAGS) -o $(EXECUTABLE) master.o functions.o

source: source.o functions.o
	$(CC) $(CFLAGS) -o $(SOURCE) source.o functions.o

taxi: taxi.o functions.o
	$(CC) $(CFLAGS) -o $(TAXI) taxi.o functions.o

clean: 
	$(RM) -f *.o $(EXECUTABLE)
	$(RM) -f $(EXECUTABLE) $(SOURCE) $(TAXI)

run: $(EXECUTABLE)
	$(EXECUTABLE)
