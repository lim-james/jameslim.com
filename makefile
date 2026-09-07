CXX      ?= g++
CXXFLAGS ?= -std=c++23 -O3 -Wall -Wextra
LDLIBS   ?= -lpthread
 
TARGET  := main
SRC     := main.cpp
 
.PHONY: all run clean
 
all: $(TARGET)
 
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(LDLIBS)
 
run: $(TARGET)
	./$(TARGET) 127.0.0.1 8081
 
clean:
	rm -f $(TARGET)
