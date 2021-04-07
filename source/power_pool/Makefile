CC = gcc
ODIR = build

CFLAGS = -std=c99 -Wall
LIBS = -lzmq -lpthread -lm -lbsd -latomic -ldl
DEPS = ctx.h power_client.h rapl.h

all: power_pool
debug: CFLAGS += -DVERBOSE
debug: power_pool
test: CFLAGS += -DVERBOSE
test: CFLAGS += -DTEST
test: power_pool

_OBJ = ctx.o power_client.o rapl.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	@mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

power_pool: power_pool.c $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o power_pool
