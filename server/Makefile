CC=gcc-9
#CROSS_COMPILE=aarch64-none-linux-gnu-
TARGET=aesdsocket
SRC=aesdsocket.c

default: all

all: $(TARGET)

$(TARGET):
	$(CC) -o $(TARGET) $(SRC)
clean :
	rm -f $(TARGET)