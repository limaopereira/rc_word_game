CC		= gcc
CFLAGS	= -O3 -Wall -Wno-unused-result -Wno-stringop-truncation
TARGET	= GS

SOURCES	= udp_server.c tcp_server.c server.c
DEPS	= server.h Makefile

all: $(TARGET)

$(TARGET): $(SOURCES) $(DEPS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ 

clean:
	rm -f $(TARGET)
