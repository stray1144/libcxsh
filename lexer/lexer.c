#include <cxtoolchain/libcxsh.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

bool handleComment(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(strncmp(string, "//", 2) != 0) return false;

	uint64_t consumed = strcspn(string, "\n");
	
	token->kind = token_comment;
	token->string = (slice_t) {string, consumed};

	advanceLexer(lexer, consumed);

	return true;
}

bool handleIdentifier(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(!isalpha(*string) && *string != '_') return false;
	
	uint64_t consumed = strspn(string, "_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

	token->kind = token_identifier;
	token->string = (slice_t) {string, consumed};

	advanceLexer(lexer, consumed);

	return true;
}



bool handleHex(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(strncasecmp(string, "0x", 2) != 0) return false;

	char *end = 0;

	token->kind = token_hex_literal;
	token->number = strtoll(string, &end, 16);

	uint64_t consumed = end - string;
	advanceLexer(lexer, consumed);

	return true;
}

bool handleInteger(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(!isdigit(*string) && *string != '+' && *string != '-') return false;

	char *end = 0;

	token->kind = token_integer_literal;
	token->number = strtoll(string, &end, 10);

	uint64_t consumed = end - string;
	advanceLexer(lexer, consumed);

	return true;
}

// handleFloat...

bool handleString(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(*string != '\"') return false;

	uint64_t consumed = strcspn(string + 1, "\"") + 2;
	
	token->kind = token_string_literal;
	token->string = (slice_t) {string, consumed};

	advanceLexer(lexer, consumed);

	return true;
}

bool handlePunctuation(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(!ispunct(*string)) return false;
	
	token->kind = token_punctuation;
	token->character = *string;

	advanceLexer(lexer, 1);

	return true;
}

bool handleNewline(lexer_t *lexer, token_t *token) {
	char *string = getLexerString(lexer);

	if(*string != '\n') return false;
	
	token->kind = token_newline;
	token->character = *string;

	advanceLexer(lexer, 1);

	return true;
}


bool initLexer(lexer_t *lexer, char *data) {
	if(!lexer) return false;

	memset(lexer, 0, sizeof(lexer_t));

	lexer->data = data;

	configurateLexerBehaviour(lexer, token_comment, handleComment);
	configurateLexerBehaviour(lexer, token_identifier, handleIdentifier);

	configurateLexerBehaviour(lexer, token_hex_literal, handleHex);
	configurateLexerBehaviour(lexer, token_integer_literal, handleInteger);
	// ...
	configurateLexerBehaviour(lexer, token_string_literal, handleString);

	configurateLexerBehaviour(lexer, token_punctuation, handlePunctuation);
	configurateLexerBehaviour(lexer, token_newline, handleNewline);

	return true;
}

bool triggerLexerBehaviour(lexer_t *lexer, token_t *token, tokenKind_t kind) {
	if(!kind || kind >= TOKEN_KIND_COUNT) return false;

	lexerBehaviour_t *behaviour = lexer->behaviours[kind];
	if(!behaviour) return false;

	return behaviour(lexer, token);
}

void configurateLexerBehaviour(lexer_t *lexer, tokenKind_t set, lexerBehaviour_t *behaviour) {
	if(!set || set >= TOKEN_KIND_COUNT) return;
	lexer->behaviours[set] = behaviour;
}

void skipNoise(lexer_t *lexer) {
	advanceLexer(lexer, strspn(getLexerString(lexer), " \t\r"));
}

bool lex(lexer_t *lexer, token_t *token) {
	memset(token, 0, sizeof(token_t));

	if(!*getLexerString(lexer)) return false;

	skipNoise(lexer);

	for(int i = 1; i < TOKEN_KIND_COUNT; i++) if(triggerLexerBehaviour(lexer, token, i)) return true;

	return false;
}
