CC		= gcc
CFLAGS	= -O3 -Wall
TARGET	= GS

SOURCES	= server.c


all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
