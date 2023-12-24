#Makefile to build a target
CC = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
# -O2 optimize for better performance
SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LDFLAGS := $(shell sdl-config --libs) -lSDL_ttf -lstdc++
DEFINES = -DTEST -DTEST_GRAPHICS
CFLAGS = -Wall -I. -g -O2 ${SDL_CFLAGS} ${DEFINES}

TARGET = graphics

SRCS = graphics.cpp

OBJS = ${SRCS:.cpp=.o}

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} ${SDL_LDFLAGS} 

.cpp.o:
	${CC} -c ${CFLAGS} $*.cpp

clean:
	rm -rf core ${OBJS} ${TARGET} *.bak
