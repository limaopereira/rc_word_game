CC		= gcc
CFLAGS	= -O3 -Wall
TARGET	= player

SOURCES	= player.c functions.c player_functions.c 


all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
