CXX      = clang++
CXXFLAGS = -std=c++17 -pthread -I./include
LIBS     =  -lhiredis -luuid -lspdlog -lfmt
TARGET   = api_redis_server

.PHONY: all clean install

all: $(TARGET)

$(TARGET): server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)

install: $(TARGET)
	install -m 755 $(TARGET) $(HOME)/.local/bin/

clean:
	rm -f $(TARGET)