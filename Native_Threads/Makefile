# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -O3 -std=c++20 -pthread \
       		-ftree-vectorize \
       		-fopt-info-vec-optimized 
			
# The build target 
TARGET = Huffman_NT
PARAMS = "../data/dataset.txt" "../data/dataset_compressed_NT.txt" 

all: $(TARGET)

$(TARGET): $(TARGET).cpp
			$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
	$(RM) $(TARGET)

restart:
	$(RM) $(TARGET)
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp
	./$(TARGET)

run: 
	./$(TARGET) $(PARAMS)

