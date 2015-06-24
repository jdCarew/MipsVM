CFLAGS = -Wall -std=c99

all:
	${CC} ${CFLAGS} MipsVM.c -o MipsVM
