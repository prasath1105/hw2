CC := gcc
SRCD := src
TSTD := tests
BLDD := build
BIND := bin
INCD := include

MAIN  := $(BLDD)/main.o

ALL_SRCF := $(shell find $(SRCD) -type f -name *.c)
ALL_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(ALL_SRCF:.c=.o))
ALL_FUNCF := $(filter-out $(MAIN) $(AUX), $(ALL_OBJF))

TEST_SRC := $(shell find $(TSTD) -type f -name "*.c")

INC := -I $(INCD)

CRITERION_PATH=$(shell brew --prefix criterion)

CFLAGS = -Wall -Werror -g -I/Users/prasath/Project/hw2/include -std=gnu11 -I include -I/opt/homebrew/include
COLORF := -DCOLOR
DFLAGS := -g -DDEBUG -DCOLOR
PGFLAGS := -g -pg
PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO
LDFLAGS = -L/opt/homebrew/lib -lcriterion

STD := -std=gnu11
TEST_LIB := -lcriterion
LIBS :=

CFLAGS += $(STD)

EXEC := sequitur
TEST_EXEC := $(EXEC)_tests

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(BIND)/$(TEST_EXEC)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS) $(COLORF)
debug: all

prof: CFLAGS += $(PGFLAGS)
prof: all

setup: $(BIND) $(BLDD)
$(BIND):
	mkdir -p $(BIND)
$(BLDD):
	mkdir -p $(BLDD)

$(BIND)/$(EXEC): $(ALL_OBJF)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(BIND)/$(TEST_EXEC): $(ALL_FUNCF) $(TEST_SRC)
	$(CC) $(CFLAGS) $(INC) $(ALL_FUNCF) $(TEST_SRC) $(LDFLAGS) $(LIBS) -o $@


$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	rm -rf $(BLDD) $(BIND)

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
