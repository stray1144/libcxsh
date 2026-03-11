#include <cxtoolchain/libcxsh.h>

#include "test.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


// 1 + 1
// number plus number
// expression
// basic expression evaluator, too big for the test, too little for the API

static char *data = "(50 * 2) / 4";
// (50 * 2) / 4
// (expr) / 4
// expr / 4
// expr

enum semantizer_unit_kind_e : uint32_t {
	SEMANTIC_INVALID,
	SEMANTIC_NUMBER,
	SEMANTIC_PLUS,
	SEMANTIC_MINUS,
	SEMANTIC_ASTERISK,
	SEMANTIC_SLASH,
	SEMANTIC_LEFT_PARENTHESIS,
	SEMANTIC_RIGHT_PARENTHESIS,

	// COMPOUNDS GO HERE!
	SEMANTIC_EXPRESSION
};

static const char *semantic_names[9] = {
	"SEMANTIC_INVALID",
	"SEMANTIC_NUMBER",
	"SEMANTIC_PLUS",
	"SEMANTIC_MINUS",
	"SEMANTIC_ASTERISK",
	"SEMANTIC_SLASH",
	"SEMANTIC_LEFT_PARENTHESIS",
	"SEMANTIC_RIGHT_PARENTHESIS",
	
	// COMPOUNDS GO HERE!
	"SEMANTIC_EXPRESSION"
};

bool number_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_INTEGER_LITERAL) && (token->kind != LEXER_TOKEN_HEXADECIMAL_LITERAL)) return false;

	semantizer_unit_init(unit, SEMANTIC_NUMBER, &token->number, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

bool plus_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_PUNCTUATION) || (token->character != '+')) return false;

	semantizer_unit_init(unit, SEMANTIC_PLUS, nullptr, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

bool minus_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_PUNCTUATION) || (token->character != '-')) return false;

	semantizer_unit_init(unit, SEMANTIC_MINUS, nullptr, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

bool asterisk_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_PUNCTUATION) || (token->character != '*')) return false;

	semantizer_unit_init(unit, SEMANTIC_ASTERISK, nullptr, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

bool slash_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_PUNCTUATION) || (token->character != '/')) return false;

	semantizer_unit_init(unit, SEMANTIC_SLASH, nullptr, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

bool left_parenthesis_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_PUNCTUATION) || (token->character != '(')) return false;

	semantizer_unit_init(unit, SEMANTIC_LEFT_PARENTHESIS, nullptr, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

bool right_parenthesis_handle(semantizer_unit_t *unit, lexer_token_t *token) {
	if((token->kind != LEXER_TOKEN_PUNCTUATION) || (token->character != ')')) return false;

	semantizer_unit_init(unit, SEMANTIC_RIGHT_PARENTHESIS, nullptr, SEMANTIZER_DATA_FREE_NONE);
	semantizer_token_trace(unit, token);

	return true;
}

#define FORGE_CALLBACKS 7
semantizer_forge_callback_t *forge_callbacks[FORGE_CALLBACKS] = {
	number_handle,
	plus_handle,
	minus_handle,
	asterisk_handle,
	slash_handle,
	left_parenthesis_handle,
	right_parenthesis_handle
};

// instruction -> mnemonic


// ler (lparen-expression-rparen)
bool ler_match(semantizer_t *semantizer, size_t index) {
	return  semantizer_stream_match(semantizer, index, SEMANTIC_LEFT_PARENTHESIS) && 
		semantizer_stream_match(semantizer, index + 1, SEMANTIC_EXPRESSION) && 
		semantizer_stream_match(semantizer, index + 2, SEMANTIC_RIGHT_PARENTHESIS);
}

size_t ler_reduct(semantizer_t *semantizer, semantizer_unit_t *unit, size_t start) {
	semantizer_unit_init(unit, SEMANTIC_EXPRESSION, 0, SEMANTIZER_DATA_FREE_NONE);
	semantizer_stream_steal(semantizer, start + 1, &unit->data, &unit->data_free);
	semantizer_stream_trace(semantizer, unit, start + 1);

	return 3;
}

// xox (expression-operator-expression) 
// [number operator number]
//
typedef enum expression_kind_e {
	EXPRESSION_ADD,
	EXPRESSION_SUBSTRACT,
	EXPRESSION_MULTIPLY,
	EXPRESSION_DIVIDE
} expression_kind_t;

typedef struct expression_s {
	expression_kind_t kind;
	uint64_t lhs;
	uint64_t rhs;
} expression_t;

expression_t *expression_create(expression_kind_t kind, uint64_t lhs, uint64_t rhs) {
	expression_t *expression = malloc(sizeof(expression_t));

	expression->kind = kind;
	expression->lhs = lhs;
	expression->rhs = rhs;

	return expression;
}

uint64_t expression_evaluate(expression_t *expression) {
	uint64_t result = 0;

	if(expression->kind == EXPRESSION_ADD) result = expression->lhs + expression->rhs;
	if(expression->kind == EXPRESSION_SUBSTRACT) result = expression->lhs - expression->rhs;
	if(expression->kind == EXPRESSION_MULTIPLY) result = expression->lhs * expression->rhs;
	if(expression->kind == EXPRESSION_DIVIDE) result = expression->lhs / expression->rhs;

	return result;
}

bool xox_match(semantizer_t *semantizer, size_t index) {
	bool lhs_is_valid = (semantizer_stream_match(semantizer, index, SEMANTIC_NUMBER) || semantizer_stream_match(semantizer, index, SEMANTIC_EXPRESSION));
	
	bool operator_is_valid = semantizer_stream_match(semantizer, index + 1, SEMANTIC_PLUS) ||
		                 semantizer_stream_match(semantizer, index + 1, SEMANTIC_MINUS) ||
				 semantizer_stream_match(semantizer, index + 1, SEMANTIC_ASTERISK) ||
				 semantizer_stream_match(semantizer, index + 1, SEMANTIC_SLASH);

	bool rhs_is_valid = (semantizer_stream_match(semantizer, index + 2, SEMANTIC_NUMBER) || semantizer_stream_match(semantizer, index + 2, SEMANTIC_EXPRESSION));

	return lhs_is_valid && operator_is_valid && rhs_is_valid;
}

uint64_t value_get(semantizer_t *semantizer, size_t index) {
	uint64_t result = 0;

	if(semantizer_stream_match(semantizer, index, SEMANTIC_NUMBER)) {
		uint64_t *pointer = nullptr;
		semantizer_stream_steal(semantizer, index, (void *)&pointer, nullptr);
		result = *pointer;
	}
	if(semantizer_stream_match(semantizer, index, SEMANTIC_EXPRESSION)) {
		expression_t *pointer = nullptr;
		semantizer_stream_steal(semantizer, index, (void *)&pointer, nullptr);
		result = expression_evaluate(pointer);
		free(pointer);
	}

	return result;
}

// 
size_t xox_reduct(semantizer_t *semantizer, semantizer_unit_t *unit, size_t start) {
	uint64_t lhs = value_get(semantizer, start);
	uint64_t rhs = value_get(semantizer, start + 2);

	expression_t *expression = nullptr;

	if(semantizer_stream_match(semantizer, start + 1, SEMANTIC_PLUS)) expression = expression_create(EXPRESSION_ADD, lhs, rhs);
	if(semantizer_stream_match(semantizer, start + 1, SEMANTIC_MINUS)) expression = expression_create(EXPRESSION_SUBSTRACT, lhs, rhs);
	if(semantizer_stream_match(semantizer, start + 1, SEMANTIC_ASTERISK)) expression = expression_create(EXPRESSION_MULTIPLY, lhs, rhs);
	if(semantizer_stream_match(semantizer, start + 1, SEMANTIC_SLASH)) expression = expression_create(EXPRESSION_DIVIDE, lhs, rhs);

	semantizer_unit_init(unit, SEMANTIC_EXPRESSION, expression, free);
	semantizer_stream_trace(semantizer, unit, start);

	return 3;
}

#define PATTERNS 2
semantizer_pattern_t patterns[PATTERNS] = {
	{ler_match, ler_reduct, 0},
	{xox_match, xox_reduct, 0}
};

void tokens_get(buffer_t *buffer, char *string) {
	lexer_t lexer = {0};

	lexer_init(&lexer, string);

	while(true) {
		lexer_token_t token = {0};
		bool status = lex(&lexer, &token);
		if(status == false) break;
		buffer_append(buffer, &token, 1);
	}

	lexer_clear(&lexer);
}

void debug(void *context, uint32_t level, char *message) {
	(void) (context);
	(void) (level);
	printf("[DEBUG] %s\n", message);
}

int main(void) {
	buffer_t buffer = {0};
	buffer_init(&buffer, sizeof(lexer_token_t));

	tokens_get(&buffer, data);

	semantizer_t semantizer = {0};
	semantizer_init(&semantizer);

	semantizer_forge_setup(&semantizer, forge_callbacks, FORGE_CALLBACKS);
	semantizer_forge_result_t status = semantizer_forge_atomize(&semantizer, buffer_get(&buffer, 0), buffer.used);
	
	test_assert(status.status == FORGE_SUCCESS, "atomizing failed");

	semantizer_debug_setup(&semantizer, 
			false, // set to true to get debugging
			(void *)semantic_names, 9, 
			(void *)debug, nullptr);

	semantizer_pattern_setup(&semantizer, patterns, PATTERNS, 1);
	semantize(&semantizer);

	expression_t *expression = nullptr;
	semantizer_stream_steal(&semantizer, 0, (void *)&expression, nullptr);
	uint64_t x = expression_evaluate(expression);

	semantizer_clear(&semantizer);
	buffer_clear(&buffer);

	test_assert(x == 25, "failed evaluation :p");

	free(expression);
}
