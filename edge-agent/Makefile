CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS ?= -pthread

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
SYSTEMD_UNIT_DIR ?= /etc/systemd/system

SRCS    := src/main.c src/metrics_cpu.c src/metrics_mem.c src/metrics_temp.c \
           src/telemetry.c src/http_post.c src/config.c
OBJS    := $(SRCS:.c=.o)
TARGET  := edge-telemetry

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(SYSTEMD_UNIT_DIR)
	install -m 644 systemd/edge-telemetry.service $(DESTDIR)$(SYSTEMD_UNIT_DIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(SYSTEMD_UNIT_DIR)/edge-telemetry.service
