#
# Copyright (C) 2016 NXP Semiconductor
#

include ../../common.mk

LDFLAGS += -lpthread
OBJ	:= test_rc.o
EXE	:= test_rc

all: $(OBJ)
	$(CC) -o $(EXE) $^ $(LDFLAGS)
clean:
	rm -rf $(OBJ) $(EXE)
