FILE ?= linedetection
CC= g++
runfile: $(FILE).cpp
	$(CC) $(FILE).cpp -o $(FILE) `pkg-config --cflags --libs opencv4` -l wiringPi

c: $(FILE).c
	gcc $(FILE).c -o $(FILE) `pkg-config --cflags --libs opencv4` -l wiringPi

clean:
	rm -f runfile
# # Define the file name


# # Define the compiler
# CC = g++


# # Define the target file
# TARGET = $(FILE)

# all: $(TARGET)

# runfile: $(TARGET).cpp
#     $(CC) $(TARGET).cpp -o $(TARGET) `pkg-config --cflags --libs opencv4`

# clean:
#     $(RM) $(TARGET)