#include <cxtoolchain/libcxsh.h>

#include "test.h"

#include <string.h>

#define TOKENS 9

static tokenKind_t kindChecks[9] = {
	token_comment, token_newline, token_identifier, 
	token_punctuation, token_hex_literal, token_integer_literal, 
	token_integer_literal, token_string_literal, token_newline
};


static char *data = {
	"\t// this is a test\n"
	"hello, 0x10 +20 -20 \"hello world\"\n"
};

void __lex(lexer_t *lexer, token_t *token, tokenKind_t kind) {
	test_assert(lex(lexer, token) != 0, "Failed to lex!");
	test_assert(token->kind == kind, "Kind mismatch!");
}

int main(void) {
	lexer_t lexer = {0};
	token_t tokens[TOKENS] = {0};

	initLexer(&lexer, data);

	for(int i = 0; i < 9; i++) __lex(&lexer, &tokens[i], kindChecks[i]);
	
	test_assert(strncmp(tokens[0].string.base, "// this is a test", tokens[0].string.length) == 0, "Token 1 (comment) is wrong");
	test_assert(strncmp(tokens[2].string.base, "hello", tokens[2].string.length) == 0, "Token 3 (identifier) is wrong");
	test_assert(tokens[3].character == ',', "Token 4 (punctuation) is wrong");
	test_assert(tokens[4].number == 0x10, "Token 5 (hex number) is wrong");
	test_assert(tokens[5].number == +20, "Token 6 (positive integer) is wrong");
	test_assert(tokens[6].number == -20, "Token 7 (negative integer) is wrong");
	test_assert(strncmp(tokens[7].string.base, "\"hello world\"", tokens[7].string.length) == 0, "Token 8 (string) is wrong");

	return 0;
}
