#include <cxtoolchain/libcxsh.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

bool comment_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(strncmp(string, "//", 2) != 0) return false;

	lexer_position_t consumed = strcspn(string, "\n");
	
	token->kind = LEXER_TOKEN_COMMENT;
	token->string = (lexer_span_t) {lexer_string_get(lexer), consumed};

	lexer_advance(lexer, consumed);

	return true;
}

bool identifier_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(!isalpha(*string) && *string != '_') return false;
	
	lexer_position_t consumed = strspn(string, "_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

	token->kind = LEXER_TOKEN_IDENTIFIER;
	token->string = (lexer_span_t) {lexer_string_get(lexer), consumed};

	lexer_advance(lexer, consumed);

	return true;
}

bool hexadecimal_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(strncasecmp(string, "0x", 2) != 0) return false;

	char *end = 0;

	token->kind = LEXER_TOKEN_HEXADECIMAL_LITERAL;
	token->number = strtoll(string, &end, 16);

	lexer_position_t consumed = end - string;
	lexer_advance(lexer, consumed);

	return true;
}

bool integer_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(!isdigit(*string) && *string != '+' && *string != '-') return false;

	char *end = 0;

	int64_t number = strtoll(string, &end, 10);

	lexer_position_t consumed = end - string;
	if(consumed == 0) return false;

	token->kind = LEXER_TOKEN_INTEGER_LITERAL;
	token->number = number;

	lexer_advance(lexer, consumed);

	return true;
}

bool float_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(!isdigit(*string) && *string != '+' && *string != '-') return false;

	char *end = 0;

	float real = strtof(string, &end);

	lexer_position_t consumed = end - string;

	// parenthesis paranoid
	if(consumed == 0 || memchr(string, '.', consumed) == nullptr) return false;

	token->kind = LEXER_TOKEN_FLOAT_LITERAL;
	token->real = real;

	lexer_advance(lexer, consumed);

	return true;
}

bool string_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(*string != '\"') return false;

	lexer_position_t consumed = strcspn(string + 1, "\"") + 2;
	
	token->kind = LEXER_TOKEN_STRING_LITERAL;
	token->string = (lexer_span_t) {lexer_string_get(lexer), consumed};

	lexer_advance(lexer, consumed);

	return true;
}

bool punctuation_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(!ispunct(*string)) return false;
	
	token->kind = LEXER_TOKEN_PUNCTUATION;
	token->character = *string;

	lexer_advance(lexer, 1);

	return true;
}

bool newline_handle(lexer_t *lexer, lexer_token_t *token) {
	char *string = lexer_string_get(lexer);

	if(*string != '\n') return false;
	
	token->kind = LEXER_TOKEN_NEWLINE;
	token->character = *string;

	lexer_advance(lexer, 1);

	return true;
}

char *lexer_string_get(lexer_t *lexer) {
	return lexer->data + lexer->consumed;
}

bool lexer_finished(lexer_t *lexer) {
	return *lexer_string_get(lexer) == '\0';
}

void lexer_advance(lexer_t *lexer, lexer_position_t steps) {
	lexer->consumed += steps;
}

lexer_position_t lexer_position_get(lexer_t *lexer) {
	return lexer->consumed;
}

void lexer_position_resolve(lexer_t *lexer, lexer_position_t position, lexer_position_t *line, lexer_position_t *column) {
	(*line) = 1;
	(*column) = 1;

	for(lexer_position_t i = 0; i < position; i++) {
		char character = lexer->data[i];
		(*column)++;
		
		if(character != '\n') continue;
		(*line)++;
		(*column) = 1;
	}
}

bool lexer_init(lexer_t *lexer, char *data) {
	if(!lexer) return false;
	
	lexer_clear(lexer);

	lexer->data = data;

	lexer_callback_set(lexer, LEXER_TOKEN_COMMENT, comment_handle);
	lexer_callback_set(lexer, LEXER_TOKEN_IDENTIFIER, identifier_handle);

	lexer_callback_set(lexer, LEXER_TOKEN_FLOAT_LITERAL, float_handle);
	lexer_callback_set(lexer, LEXER_TOKEN_HEXADECIMAL_LITERAL, hexadecimal_handle);
	lexer_callback_set(lexer, LEXER_TOKEN_INTEGER_LITERAL, integer_handle);
	lexer_callback_set(lexer, LEXER_TOKEN_STRING_LITERAL, string_handle);

	lexer_callback_set(lexer, LEXER_TOKEN_PUNCTUATION, punctuation_handle);
	lexer_callback_set(lexer, LEXER_TOKEN_NEWLINE, newline_handle);

	return true;
}

void lexer_clear(lexer_t *lexer) {
	if(!lexer) return;

	memset(lexer, 0, sizeof(lexer_t));
}

bool lexer_callback_run(lexer_t *lexer, lexer_token_t *token, lexer_token_kind_t kind) {
	if(kind == 0 || kind >= LEXER_TOKEN_KIND_COUNT) return false;

	lexer_callback_t *callback = lexer->callback[kind];
	if(callback == LEXER_CALLBACK_NONE) return false;

	return callback(lexer, token);
}

void lexer_callback_set(lexer_t *lexer, lexer_token_kind_t set, lexer_callback_t *callback) {
	if(set == 0 || set >= LEXER_TOKEN_KIND_COUNT) return;
	lexer->callback[set] = callback;
}

void noise_skip(lexer_t *lexer) {
	lexer_advance(lexer, strspn(lexer_string_get(lexer), " \t\r"));
}

bool lex(lexer_t *lexer, lexer_token_t *token) {
	memset(token, 0, sizeof(lexer_token_t));

	if(lexer_finished(lexer)) return false;

	noise_skip(lexer);

	lexer_position_t previous = lexer_position_get(lexer);
	// not the best way to do it, but the fastest to implement

	for(int i = 1; i < LEXER_TOKEN_KIND_COUNT; i++) {
		if(lexer_callback_run(lexer, token, i)) {
			token->position = previous;
			return true;
		}
	} // denest

	return false;
}
