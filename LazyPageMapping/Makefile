CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
TARGET = page_fault_test

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET)