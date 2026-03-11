CC ?= clang
AR ?= ar

BUILD := build
CFLAGS := -Wall -Wextra -Werror -g -Iinclude -fPIC -std=c23

PREFIX ?= /usr

INCLUDE_INSTALL := $(PREFIX)/include
LIB_INSTALL := $(PREFIX)/lib64

SOURCE := source/lexer.c source/buffer.c source/reo.c source/semantizer.c
HEADERS := include/cxtoolchain/libcxsh.h
OBJECTS := $(patsubst source/%.c, build/objects/%.o, $(SOURCE))

.PHONY: test install

all: $(BUILD)/libcxsh.so $(BUILD)/libcxsh.a test

$(BUILD)/objects/%.o: source/%.c $(HEADERS) | build 
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)/objects
	@mkdir -p $(BUILD)/test

$(BUILD)/libcxsh.so: $(OBJECTS)
	@clang $? -shared -o $@

$(BUILD)/libcxsh.a: $(OBJECTS)
	@$(AR) rcs $@ $^

test: $(OBJECTS) 
	@clang $(CFLAGS) $? tests/lex.c -o $(BUILD)/test/lex
	@clang $(CFLAGS) $? tests/reotest.c -o $(BUILD)/test/reotest
	@clang $(CFLAGS) $? tests/buffer.c -o $(BUILD)/test/buffer
	@clang $(CFLAGS) $? tests/semantize.c -o $(BUILD)/test/semantize
	@./scripts/test.sh $(BUILD)

install: all
	@mkdir -p $(DESTDIR)/$(LIB_INSTALL)
	@mkdir -p $(DESTDIR)/$(INCLUDE_INSTALL)/cxtoolchain
	@install -m 755 $(BUILD)/libcxsh.so $(DESTDIR)/$(LIB_INSTALL)
	@install -m 644 include/cxtoolchain/libcxsh.h $(DESTDIR)/$(INCLUDE_INSTALL)/cxtoolchain
