CC=gcc
CFLAGS= -Wall -Wextra -pedantic -std=gnu99 -I/local/courses/csse2310/include/ -L/local/courses/csse2310/lib -ltinyexpr -lm
LDFLAGS=
all: uqexpr
uqexpr: uqexpr.c
	$(CC) -o uqexpr $(CFLAGS) uqexpr.c 
	
