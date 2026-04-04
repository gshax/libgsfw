CC ?= $(PREFIX)gcc
AR ?= $(PREFIX)ar
TARGET ?= native

CFLAGS ?= -Wall -Wno-unused-variable -Wno-address-of-packed-member -O2
CFLAGS += -Iinclude
ifeq ($(strip $(EMBEDDED)),1)
	CFLAGS += -DLIBGSFW_EMBEDDED
endif

BUILD = build/$(TARGET)

SRCS = $(wildcard src/shared/*.c) $(wildcard src/firmware/*.c) $(wildcard src/firmware/family/*.c) $(wildcard src/bootrom/*.c)
OBJS = $(patsubst src/%,$(BUILD)/obj/libgsfw/%,$(SRCS:.c=.o))

TOOL_SRCS = $(wildcard tools/*.c)
TOOLS = $(patsubst tools/%,$(BUILD)/%,$(TOOL_SRCS:.c=))

LIBGSFW = $(BUILD)/libgsfw.a

.PHONY: all clean tools libgsfw

all: tools

libgsfw: $(LIBGSFW)

$(LIBGSFW): $(OBJS)
	$(AR) rcs $@ $^

# libgsfw core (no stdlib)
$(BUILD)/obj/libgsfw/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -nostdlib -o $@ $<

# tools support (util.c)
$(BUILD)/obj/util.o: src/util.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

tools: $(LIBGSFW) $(TOOLS)

$(BUILD)/%: tools/%.c $(BUILD)/obj/util.o $(LIBGSFW)
	$(CC) $(CFLAGS) -static -o $@ $(BUILD)/obj/util.o $< -L$(BUILD) -lgsfw

clean:
	rm -rf $(BUILD)
