
CFLAGS += -Iinclude -Og -g -Wall -Werror

BUILD_PATH = $(abspath ./build)
SOURCES = tap.c net_management.c
HEADERS = include/tap.h include/net_management.h

OBJS := $(patsubst %.c, $(BUILD_PATH)/%.o, $(SOURCES))

.PHONY: all

all: tap

tap: $(OBJS) $(HEADERS)
	gcc $(OBJS) $(CFLAGS) -o $@

$(BUILD_PATH)/%.o: %.c include/%.h
	mkdir -p "$(shell dirname $@)"
	gcc $< $(CFLAGS) -c -o $@

clean:
	rm -rf build tap
