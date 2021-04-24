TARGET = test_pipes
CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -g
MKDIR ?= mkdir -p
RMDIR ?= rm -rf

BINDIR ?= ./bin
BUILDDIR ?= ./build

SRCS = $(shell find . -name "*.c" -type f)
OBJS = $(SRCS:%=$(BUILDDIR)/%.o)
DEPS = $(OBJS:.o=.d)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJS)
	$(MKDIR) $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILDDIR)/%.c.o: %.c
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RMDIR) $(BINDIR) $(BUILDDIR)

-include $(DEPS)
