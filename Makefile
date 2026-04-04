CC ?= gcc
AR ?= ar
CFLAGS ?= -D_DEFAULT_SOURCE -Wall -Wno-unused-variable -O2
CFLAGS += -Iinclude

SRCS = $(wildcard src/*.c) $(wildcard src/firmware/*.c)
OBJS = $(SRCS:.c=.o)

TOOL_SRCS = $(wildcard tools/*.c)
TOOLS = $(TOOL_SRCS:.c=)

.PHONY: all clean tools

all: tools

libgsfw.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tools: libgsfw.a $(TOOLS)

tools/%: tools/%.c libgsfw.a
	$(CC) $(CFLAGS) -static -o $@ $< -L. -lgsfw

clean:
	rm -f $(OBJS) libgsfw.a $(TOOLS)
