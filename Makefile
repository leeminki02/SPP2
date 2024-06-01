FILE ?= main
CC= g++
CFLAGS= `pkg-config --cflags --libs opencv4` -l wiringPi

# C++ 소스 파일을 컴파일하기 위한 규칙
%.o: %.cpp
	$(CC) -c $< -o $@ $(CFLAGS)

# C 소스 파일을 컴파일하기 위한 규칙
%.o: %.c
	gcc -c $< -o $@ $(CFLAGS)

# 기본 타겟: main.cpp와 qr.cpp를 링크하여 실행 파일 생성
runfile: $(FILE).o qr.o
	$(CC) $^ -o $(FILE) $(CFLAGS)

# C 파일만을 위한 타겟
c: $(FILE).o qr.o
	g++ $^ -o $(FILE) `pkg-config --cflags --libs opencv4` -l wiringPi
clean:
	rm -f runfile $(FILE) *.o

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