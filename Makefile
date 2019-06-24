# Target library
lib := libuthread.a
objs := preempt.o uthread.o context.o queue.o
executable := api
CC := gcc
CFLAGS := -Wall -Werror
CFLAGS += -g
DEPFLAGS = -MMD -MF $(@:.o=.d)

ifneq ($(V),1)
Q = @
endif

all: $(executable)

## TODO: Phase 1.1
$(executable): $(objs) $(lib)
	@echo "CC $@"
	$(Q) $(CC) $(CFLAGS) -o $@ $(objs) -L. -luthread

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

$(lib): $(objs)
	ar rcs $(lib) $(objs)

libs: $(lib)

clean:
	@echo "clean"
	$(Q)rm -f $(targets) $(objs) $(lib) $(executable) *.d
