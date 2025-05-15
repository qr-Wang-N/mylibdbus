AR := ar
CC := g++
STRIP := strip

CP := @cp -f -u
RM := @rm -f

LIB_PATH := ./lib

ARFLAGS := rcu
CFLAGS  := -ggdb3 -Wall -O2 -shared -fPIC -I. -I/usr/include
LDFLAGS := -L./lib -L. -L/usr/lib -ldbus-1 -ldl -lrt -lpthread -ltinyxml

TARGET	:= ${LIB_PATH}/libdbus.so
SRCS    := $(wildcard *.cpp )
OBJS    := $(SRCS:%.cpp=%.o)

.PHONY: clean all

$(TARGET): $(OBJS)
	$(CC) ${CFLAGS} -o $@ $(OBJS) ${LDFLAGS}

%.o:%.cpp
	$(CC) -o $@ -c $< ${CFLAGS}

clean:
	rm -f $(OBJS) $(TARGET)

