CC = gcc
CFLAGS = -Wall
LIBS = -lm

# 可执行文件的名称
TARGET = proj3

# 默认目标，构建可执行文件
all: $(TARGET) clean

# 构建可执行文件
$(TARGET): proj3.o socket_methods.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# 编译 proj2.c 文件
proj2.o: proj3.c socket_methods.h
	$(CC) $(CFLAGS) -c $<

# 编译 methods.c 文件
methods.o: socket_methods.c socket_methods.h
	$(CC) $(CFLAGS) -c $<

# 清理中间文件和可执行文件
clean:
	rm -f *.o
