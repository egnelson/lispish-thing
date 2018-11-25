INCLUDE = -Isrc
# CC = clang++
CC = g++
CFLAGS = -fPIC -g
LDFLAGS = -shared
CPPFLAGS = --std=c++17 -g -Wall -Wextra -Werror

all: astdump test

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $(INCLUDE) $< -o $@

liblang.so: src/parser.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $^ -o $@

astdump: liblang.so src/astdump.o
	$(CC) -L$(CURDIR) $(CPPFLAGS) -o astdump src/astdump.o -llang

test: liblang.so src/test.o
	$(CC) -L$(CURDIR) $(CPPFLAGS) -o test src/test.o -llang

clean:
	-rm -f src/*.o test *.so astdump main