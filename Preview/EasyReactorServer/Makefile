# .PHONY: all
# all: test

# test: test.cpp
# 	g++ -o $@ $^ -lpthread

# .PHONY: clean
# clean:
# 	rm -f test


.PHONY: all
all: server client

server: server.cpp
	g++ -std=c++11 -o $@ $^ -lpthread

client: client.cpp
	g++ -o $@ $^ -lpthread

.PHONY: clean
clean:
	rm -f server client