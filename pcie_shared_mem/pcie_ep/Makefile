#
# Copyright (C) 2016 NXP Semiconductor
#

include ../../common.mk

LDFLAGS	+= -lpthread
CFLAGS	+= -g
EXE = test_ep

OBJ	:= test_ep.o
DEPS	:= pcie_ops.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -o $(EXE) $^ $(LDFLAGS)

clean:
	rm -rf $(OBJ) $(EXE)
