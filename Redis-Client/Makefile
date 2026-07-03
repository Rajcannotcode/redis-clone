# Define the compiler
CXX = g++

# Define compiler flags (include the 'include' directory)
CXXFLAGS = -I./include -Wall -std=c++17

# Define the final executable name
TARGET = redis_client

# Define the source files
SRCS = src/main.cpp src/CLI.cpp src/RedisClient.cpp src/CommandHandler.cpp src/ResponseParser.cpp

# The default target that runs when you type 'make'
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

# Rule to clean up compiled files
clean:
	rm -f $(TARGET)