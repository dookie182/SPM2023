# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -O3 \
       		-ftree-vectorize -march=native -mtune=native \
       		-fopt-info-vec-optimized 
			

# The build target 
TARGET = app
PARAMS = "../data/dataset.txt" "../data/asciiText2_compressed_FF_1.txt" 

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

