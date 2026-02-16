#include <cxtoolchain/libcxsh.h>

#include "test.h"

#include <string.h>

#define TOKENS 11

static lexer_token_kind_t kind_checks[TOKENS] = {
	LEXER_TOKEN_COMMENT, LEXER_TOKEN_NEWLINE, LEXER_TOKEN_IDENTIFIER, 
	LEXER_TOKEN_PUNCTUATION, LEXER_TOKEN_HEXADECIMAL_LITERAL, LEXER_TOKEN_INTEGER_LITERAL, 
	LEXER_TOKEN_INTEGER_LITERAL, LEXER_TOKEN_STRING_LITERAL, LEXER_TOKEN_NEWLINE, LEXER_TOKEN_FLOAT_LITERAL, LEXER_TOKEN_FLOAT_LITERAL
};

static char *data = {
	"\t// this is a test\n"
	"hello, 0x10 +20 -20 \"hello world\"\n"
	"+1.0 -1.0\n"
};



void __lex(lexer_t *lexer, lexer_token_t *token, lexer_token_kind_t kind) {
	test_assert(lex(lexer, token) != 0, "Failed to lex!");
	test_assert(token->kind == kind, "Kind mismatch!");
}

int main(void) {
	lexer_t lexer = {0};
	lexer_token_t tokens[TOKENS] = {0};

	lexer_init(&lexer, data);

	for(int i = 0; i < TOKENS; i++) __lex(&lexer, &tokens[i], kind_checks[i]);
	
	test_assert(strncmp(tokens[0].string.base, "// this is a test", tokens[0].string.length) == 0, "Token 1 (comment) is wrong");
	test_assert(strncmp(tokens[2].string.base, "hello", tokens[2].string.length) == 0, "Token 3 (identifier) is wrong");
	test_assert(tokens[3].character == ',', "Token 4 (punctuation) is wrong");
	test_assert(tokens[4].number == 0x10, "Token 5 (hex number) is wrong");
	test_assert(tokens[5].number == +20, "Token 6 (positive integer) is wrong");
	test_assert(tokens[6].number == -20, "Token 7 (negative integer) is wrong");
	test_assert(strncmp(tokens[7].string.base, "\"hello world\"", tokens[7].string.length) == 0, "Token 8 (string) is wrong");

	test_assert(tokens[9]._float == +1.0, "Token 9 (positive float) is wrong");
	test_assert(tokens[10]._float == -1.0, "Token 10 (negative float) is wrong");

	return 0;
}
