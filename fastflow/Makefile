# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -O3 -std=c++20 \
			

# The build target 
TARGET = ffhuffman
FILE = ../data/asciiText2_compressed
PARAMS = "../data/dataset.txt" "../data/asciiText2_compressed_FF_1.txt" 


all: $(TARGET)

$(TARGET): $(TARGET).cpp
			$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)
	$(RM) $(FILE).txt

restart:
	$(RM) $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp
	./$(TARGET)

run: 
	./$(TARGET) $(PARAMS)

