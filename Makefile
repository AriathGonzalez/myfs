CC	   = gcc
OBJS   = main.o implementation.o
# -W* for warnings, -g3 for maximum debug, -O3 for max optimization
CFLAGS = -g3 -O3 -Wall -Wextra -Wundef -Wshadow -Wwrite-strings -Wcast-align -Wstrict-prototypes -Waggregate-return -Wcast-qual \
        -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code -Wfloat-equal -Wno-visibility -Wno-unused-parameter

all: main

main: $(OBJS)
		$(CC) $(CFLAGS) -o $@ $^

run: main
		./main

# Deletes files generated by compilation
clean:
	rm -f *.o main myfs test.myfs


main.o: implementation.h
implementation.o: implementation.h