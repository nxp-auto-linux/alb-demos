#
# Copyright (C) 2016 NXP Semiconductors
#
# SPDX-License-Identifier:     BSD-3-Clause
#

include ../common.mk

OBJS := server.o
OBJC := client.o
OBJ  := $(OBJC) $(OBJS)
CFLAGS := -Wall

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Make server/client option included, both default
all: server client

server: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
client: $(OBJC)
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -f server client $(OBJ)

install:server client
	if [ -f $(INSTALLDIR) ]; then rm -rf $(INSTALLDIR); fi
	mkdir -p $(INSTALLDIR)
	install server $(INSTALLDIR)
	install client $(INSTALLDIR)
