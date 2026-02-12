.PHONY: all clean

CC=gcc
SCRDIR=src
OBJDIR=obj
INCLDIR=include

EXEC=shell

SRCS = $(wildcard $(SCRDIR)/*.c)
OBJS = $(SRCS:$(SCRDIR)/%.c=$(OBJDIR)/%.o)

CFLAGS=-Wall -g -I$(INCLDIR)
LIBS+=-lpthread


ifdef debug
$(shell touch $(SRCS))
CPPFLAGS=-DDEBUG
endif

$(shell mkdir -p $(OBJDIR))

all: $(EXEC)

$(OBJDIR)/%.o: $(SCRDIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/csapp.o: $(SCRDIR)/csapp.c $(INCLDIR)/csapp.h
$(OBJDIR)/builtin.o: $(SCRDIR)/builtin.c $(INCLDIR)/builtin.h $(INCLDIR)/readcmd.h $(INCLDIR)/csapp.h
$(OBJDIR)/execute.o: $(SCRDIR)/execute.c $(INCLDIR)/execute.h $(INCLDIR)/readcmd.h $(INCLDIR)/csapp.h
$(OBJDIR)/readcmd.o: $(SCRDIR)/readcmd.c $(INCLDIR)/readcmd.h
$(OBJDIR)/shell.o: $(SCRDIR)/shell.c $(INCLDIR)/builtin.h $(INCLDIR)/execute.h

$(EXEC): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

clean:
	rm -f $(EXEC) $(OBJS)

