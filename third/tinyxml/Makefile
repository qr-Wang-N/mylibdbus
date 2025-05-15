AR := ar
CC := g++
STRIP := strip
RANLIB := ranlib

CP := @cp -f -u
RM := @rm -f

BIN_PATH := ../../bin
LIB_PATH := ../../lib
INC_PATH := ../../include 

ARFLAGS := rcu
CFLAGS  := -ggdb3 -Wall -O2 -shared -fPIC -I../include  
LDFLAGS := -lm -ldl -lrt -lpthread

TARGET	:= $(LIB_PATH)/libtinyxml.so
SRCS    := $(wildcard *.cpp)
OBJS    := $(SRCS:%.cpp=%.o)

.PHONY: clean all

$(TARGET): $(OBJS)
#	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(CC) ${CFLAGS} -o $@ $(OBJS) ${LDFLAGS}
	$(CP) *.h $(INC_PATH)

%.o:%.cpp
	$(CC)  -o $@ -c $< ${CFLAGS}

clean:
	rm -f $(OBJS) $(TARGET)
