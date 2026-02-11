BUILD := build
CFLAGS := -Wall -Wextra -Werror -g -Iinclude -fPIC -std=c23

PREFIX ?= /usr

INCLUDE_INSTALL := $(PREFIX)/include
LIB_INSTALL := $(PREFIX)/lib64

LEXER_SOURCE := lexer/lexer.c
REO_CORE_SOURCE := reo/core.c
REO_READ_SOURCE := reo/read.c
REO_WRITE_SOURCE := reo/write.c

OBJECTS := $(BUILD)/out/lexer.o $(BUILD)/out/reocore.o $(BUILD)/out/reoread.o $(BUILD)/out/reowrite.o


.PHONY: init test clean install

all: init $(BUILD)/libcxsh.so test

$(BUILD)/out/lexer.o: $(LEXER_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/out/reocore.o: $(REO_CORE_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/out/reoread.o: $(REO_READ_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/out/reowrite.o: $(REO_WRITE_SOURCE)
	clang $(CFLAGS) $? -c -o $@

$(BUILD)/libcxsh.so: $(OBJECTS)
	clang $? -shared -o $@

clean:
	@rm -rf $(BUILD)

test: $(OBJECTS) 
	clang $(CFLAGS) $? tests/lex.c -o $(BUILD)/test/lex
	clang $(CFLAGS) $? tests/reotest.c -o $(BUILD)/test/reotest

	./scripts/test.sh $(BUILD)
	

init: clean
	@mkdir -p $(BUILD)/out
	@mkdir -p $(BUILD)/test

install: all
	mkdir -p $(DESTDIR)/$(LIB_INSTALL)
	mkdir -p $(DESTDIR)/$(INCLUDE_INSTALL)/cxtoolchain

	install -m 755 $(BUILD)/libcxsh.so $(DESTDIR)/$(LIB_INSTALL)
	install -m 644 include/cxtoolchain/libcxsh.h $(DESTDIR)/$(INCLUDE_INSTALL)/cxtoolchain
