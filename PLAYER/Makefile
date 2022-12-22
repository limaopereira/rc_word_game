CC		= gcc
CFLAGS	= -O3 -Wall -Wno-unused-variable -Wno-unused-but-set-variable
TARGET	= player

SOURCES	= player.c player_functions.c 


all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
