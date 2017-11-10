CC = gcc
CFLAGS += -g -O2  -Wall
AR = ar rcu

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
	REGEX_SHARE_LIB = libregex.so
	CFLAGS += -std=gnu89
endif

ifeq ($(UNAME_S), Linux)
	REGEX_SHARE_LIB = libregex.so
endif

_REGEX_LIB = \
reg_malloc.c \
reg_parse.c \
reg_stream.c \
reg_list.c \
reg_state.c \
state_gen.c \
state_match.c \
ds_queue.c \
regex.c \
reg_error.c 
REGEX_LIB = $(addprefix src/, $(_REGEX_LIB))


_TEST_CAST = \
test_parse.c \
test_edge.c \
test_list.c \
test_error.c  \
test_api.c
TEST_CAST = $(addprefix test/, $(_TEST_CAST))

REGEX_LIB_OBJ = $(foreach s, $(REGEX_LIB), $(basename $(s)).o)
TEST_CAST_OUT = $(foreach s, $(TEST_CAST), $(basename $(s)))

all: $(REGEX_LIB_OBJ) $(REGEX_SHARE_LIB) $(TEST_CAST_OUT)

$(REGEX_LIB_OBJ): $(REGEX_LIB)
	$(CC) $(CFLAGS) -c $(basename $@).c -fPIC -o $@

$(REGEX_SHARE_LIB): $(REGEX_LIB_OBJ)
	$(CC) -shared  -o $@ $(REGEX_LIB_OBJ)

$(TEST_CAST_OUT): $(TEST_CAST) $(REGEX_SHARE_LIB)
	$(CC) $(CFLAGS) -o $@  $@.c $(REGEX_SHARE_LIB)

.PHONY : clean
clean:
	rm -rf $(REGEX_LIB_OBJ)
	rm -rf $(TEST_CAST_OUT)
	rm -rf $(REGEX_SHARE_LIB)

