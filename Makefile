# CartForge build file (Linux / macOS / MSYS2).
# Requires the MySQL client library headers (libmysqlclient-dev on Ubuntu,
# `brew install mysql-client` on macOS).

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
MYSQL_CFLAGS := $(shell mysql_config --cflags)
MYSQL_LIBS   := $(shell mysql_config --libs)

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
BIN := cartforge

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(MYSQL_LIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) $(MYSQL_CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(BIN)

.PHONY: all clean
