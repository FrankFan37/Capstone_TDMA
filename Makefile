
CFLAGS += -I. -Og -g -Wall -Werror

BUILD_PATH = $(abspath ./build)
SOURCES = tap.c net_management.c
HEADERS = tap.h net_management.h

OBJS := $(patsubst %.c, $(BUILD_PATH)/%.o, $(SOURCES))

.PHONY: all

all: tap

tap_headers:


tap: $(OBJS) $(HEADERS)
	gcc $(OBJS) $(CFLAGS) -o $@

$(BUILD_PATH)/%.o: %.c
	mkdir -p "$(shell dirname $@)"
	gcc $< $(CFLAGS) -c -o $@

clean:
	rm -rf build tap
