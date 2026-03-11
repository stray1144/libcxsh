CC ?= clang
AR ?= ar

BUILD := build
CFLAGS := -Wall -Wextra -Werror -g -Iinclude -fPIC -std=c23

PREFIX ?= /usr

INCLUDE_INSTALL := $(PREFIX)/include
LIB_INSTALL := $(PREFIX)/lib64

LEXER_SOURCE := source/lexer.c
BUFFER_SOURCE := source/buffer.c
REO_SOURCE := source/reo.c
SEMANTIZER_SOURCE := source/semantizer.c

OBJECTS := $(BUILD)/out/lexer.o $(BUILD)/out/buffer.o $(BUILD)/out/reo.o $(BUILD)/out/semantizer.o


.PHONY: init test clean install

all: init $(BUILD)/libcxsh.so $(BUILD)/libcxsh.a test

$(BUILD)/out/lexer.o: $(LEXER_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/out/buffer.o: $(BUFFER_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/out/reo.o: $(REO_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/out/semantizer.o: $(SEMANTIZER_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/libcxsh.so: $(OBJECTS)
	clang $? -shared -o $@

$(BUILD)/libcxsh.a: $(OBJECTS)
	$(AR) rcs $@ $^

clean:
	@rm -rf $(BUILD)

test: $(OBJECTS) 
	clang $(CFLAGS) $? tests/lex.c -o $(BUILD)/test/lex
	clang $(CFLAGS) $? tests/reotest.c -o $(BUILD)/test/reotest
	clang $(CFLAGS) $? tests/buffer.c -o $(BUILD)/test/buffer
	clang $(CFLAGS) $? tests/semantize.c -o $(BUILD)/test/semantize

	./scripts/test.sh $(BUILD)
	

init: clean
	@mkdir -p $(BUILD)/out
	@mkdir -p $(BUILD)/test

install: all
	mkdir -p $(DESTDIR)/$(LIB_INSTALL)
	mkdir -p $(DESTDIR)/$(INCLUDE_INSTALL)/cxtoolchain

	install -m 755 $(BUILD)/libcxsh.so $(DESTDIR)/$(LIB_INSTALL)
	install -m 644 include/cxtoolchain/libcxsh.h $(DESTDIR)/$(INCLUDE_INSTALL)/cxtoolchain
