# Network Traffic & Routing Simulator
# Linux, macOS, and Windows (MinGW/MSYS2).

CC       = gcc
CFLAGS  += -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -D_POSIX_C_SOURCE=200809L
LDFLAGS +=
LDLIBS  +=

CORE_SRCS := src/graph.c src/heap.c src/routing.c src/sim.c src/queue.c \
             src/packet.c src/events.c src/traffic.c src/tcp.c \
             src/telemetry.c src/io.c src/scenarios.c
GUI_SRCS  := src/main.c src/ui_canvas.c src/ui_panels.c src/ui_charts.c src/ui_fonts.c
CORE_OBJS := $(CORE_SRCS:.c=.o)
GUI_OBJS  := $(GUI_SRCS:.c=.o)

TEST_BIN := tests/test_core
TARGET   := netsim

ifeq ($(OS),Windows_NT)
  TARGET := netsim.exe
  LDLIBS += -lraylib -lopengl32 -lgdi32 -lwinmm
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    CFLAGS  += $(shell pkg-config --cflags raylib 2>/dev/null)
    LDLIBS  += $(shell pkg-config --libs raylib 2>/dev/null)
    ifeq ($(strip $(LDLIBS)),)
      LDLIBS += -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
    LDLIBS += -lm
  endif
  ifeq ($(UNAME_S),Darwin)
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
    ifneq ($(BREW_PREFIX),)
      CFLAGS += -I$(BREW_PREFIX)/include
      LDFLAGS += -L$(BREW_PREFIX)/lib
    endif
    LDLIBS += -lraylib -framework OpenGL -framework Cocoa -framework IOKit \
              -framework CoreAudio -framework CoreVideo
  endif
endif

.PHONY: all run test clean headless-demo

all: $(TARGET)

$(TARGET): $(CORE_OBJS) $(GUI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BIN): tests/test_core.c $(CORE_SRCS)
	$(CC) $(CFLAGS) -o $@ tests/test_core.c $(CORE_SRCS) -lm

run: $(TARGET)
	./$(TARGET)

test: $(TEST_BIN)
	./$(TEST_BIN)

headless-demo: $(TARGET)
	./$(TARGET) --headless --scenario normal_traffic --seed 1 --out results/normal_traffic.csv
	./$(TARGET) --headless --scenario heavy_traffic --seed 1 --out results/heavy_traffic.csv
	./$(TARGET) --headless --scenario link_failure --seed 1 --out results/link_failure.csv

clean:
	rm -f $(CORE_OBJS) $(GUI_OBJS) $(TARGET) $(TEST_BIN)
	rm -f src/*.o demo4.csv
