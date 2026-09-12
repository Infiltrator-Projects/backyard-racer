CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic
LDLIBS ?= -lX11

TARGET := build/backyard-racer
SOURCES := src/main.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build
