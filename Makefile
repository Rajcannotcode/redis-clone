# Define the compiler
CXX = g++

# Define compiler flags (include the 'include' directory)
CXXFLAGS = -I./include -Wall -std=c++17

# Define the final executable name
TARGET = redis_server

# Define the source files based on your src/ folder
SRCS = src/main.cpp src/RedisCommandHandler.cpp src/RedisDatabase.cpp src/RedisServer.cpp

# The default target that runs when you type 'make'
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Rule to clean up compiled files
clean:
	rm -f $(TARGET)