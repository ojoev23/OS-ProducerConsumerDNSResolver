CFLAGS = -Wextra -Wall -g -std=gnu99
MAIN = multi-lookup
LIBS =
LDFLAGS = -L./ -lpthread
SRCS = multi-lookup.c array.c
HDRS = multi-lookup.h array.h
OBJS = $(SRCS:.c=.o)

all: $(MAIN)

$(MAIN): $(OBJS) $(LIBS)
	cc $(CFLAGS) -o $(MAIN) $(OBJS) $(LDFLAGS)

%.o: %.c $(HDRS)
	cc $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(MAIN)

.PHONY: all clean
