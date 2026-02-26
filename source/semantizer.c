#include <cxtoolchain/libcxsh.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool semantizer_unit_init(semantizer_unit_t *unit, semantizer_unit_kind_t kind, void *data, semantizer_data_free_t *data_free) {
	if(unit == nullptr) return false;

	semantizer_unit_clear(unit);

	unit->kind = kind;
	unit->data = data;
	unit->data_free = data_free;

	return false;
}

void semantizer_unit_clear(semantizer_unit_t *unit) {
	if(unit == nullptr) return;

	if(unit->data && unit->data_free) unit->data_free(unit->data);

	memset(unit, 0, sizeof(semantizer_unit_t));
}

semantizer_unit_kind_t semantizer_stream_get(semantizer_t *semantizer, size_t index) {
	semantizer_unit_t *unit = buffer_get(&semantizer->stream, index);
	if(unit == nullptr) return 0;

	return unit->kind;
}

bool semantizer_stream_match(semantizer_t *semantizer, size_t index, semantizer_unit_kind_t kind) {
	return semantizer_stream_get(semantizer, index) == kind;
}

void semantizer_stream_peek(semantizer_t *semantizer, size_t index, void **target, semantizer_data_free_t **target_free) {
	semantizer_unit_t *unit = buffer_get(&semantizer->stream, index);
	if(target != nullptr) *target = unit->data;
	if(target_free != nullptr) *target_free = unit->data_free;
}

void semantizer_stream_steal(semantizer_t *semantizer, size_t index, void **target, semantizer_data_free_t **target_free) {
	semantizer_unit_t *unit = buffer_get(&semantizer->stream, index);

	semantizer_stream_peek(semantizer, index, target, target_free);	

	if(target != nullptr) unit->data = nullptr;
	if(target_free != nullptr) unit->data_free = SEMANTIZER_DATA_FREE_NONE;
}

size_t semantizer_stream_size(semantizer_t *semantizer) {
	return semantizer->stream.used;
}

bool semantizer_init(semantizer_t *semantizer) {
	if(semantizer == nullptr) return false;

	semantizer_clear(semantizer);

	buffer_init(&semantizer->stream, sizeof(semantizer_unit_t));

	return true;
}

void semantizer_clear(semantizer_t *semantizer) {
	if(semantizer == nullptr) return;

	for(size_t i = 0; i < semantizer->stream.used; i++) {
		semantizer_unit_t *unit = buffer_get(&semantizer->stream, i);
		semantizer_unit_clear(unit);
	}

	buffer_clear(&semantizer->stream);

	memset(semantizer, 0, sizeof(semantizer_t));
}

void semantizer_pattern_setup(semantizer_t *semantizer, semantizer_pattern_t *patterns, size_t pattern_count, size_t level_count) {
	semantizer->patterns = patterns;
	semantizer->pattern_count = pattern_count;
	semantizer->level_count = level_count;
}

void semantizer_forge_setup(semantizer_t *semantizer, semantizer_forge_callback_t **callbacks, size_t callback_count) {
	semantizer->forge_callbacks = callbacks;
	semantizer->forge_callback_count = callback_count;
}

bool semantizer_forge_callback_run(semantizer_t *semantizer, size_t index, semantizer_unit_t *unit, lexer_token_t *token) {
	if(index >= semantizer->forge_callback_count) return false; 

	semantizer_forge_callback_t *callback = semantizer->forge_callbacks[index];
	if(callback == SEMANTIZER_FORGE_CALLBACK_NONE) return false;

	return callback(unit, token);
}

semantizer_forge_result_t semantizer_forge_result(semantizer_forge_status_t status, size_t at) {
	return (semantizer_forge_result_t) {status, at};
}

bool semantizer_forge_convert(semantizer_t *semantizer, lexer_token_t *token) {
	for(size_t i = 0; i < semantizer->forge_callback_count; i++) {
		semantizer_unit_t unit = {0};

		if(semantizer_forge_callback_run(semantizer, i, &unit, token) == true) {
			buffer_append(&semantizer->stream, &unit, 1);
			return true;
		}
	}

	return false;
}

semantizer_forge_result_t semantizer_forge_atomize(semantizer_t *semantizer, lexer_token_t *array, size_t array_size) {
	for (size_t i = 0; i < array_size; i++) {
		bool status = semantizer_forge_convert(semantizer, &array[i]);
		if(status == false) return semantizer_forge_result(FORGE_UNHANDLED, i);
	}

	return semantizer_forge_result(FORGE_SUCCESS, 0);
}

bool semantizer_matcher_callback_run(semantizer_t *semantizer, size_t pattern_index, size_t semantic_index) {
	if(pattern_index >= semantizer->pattern_count) return false; 

	semantizer_matcher_callback_t *callback = semantizer->patterns[pattern_index].match;
	if(callback == SEMANTIZER_MATCHER_CALLBACK_NONE) return false;

	return callback(semantizer, semantic_index);
}

size_t semantizer_reductor_callback_run(semantizer_t *semantizer, size_t pattern_index, semantizer_unit_t *unit, size_t semantic_index) {
	if(pattern_index >= semantizer->pattern_count) return false; 

	semantizer_reductor_callback_t *callback = semantizer->patterns[pattern_index].reduct;
	if(callback == SEMANTIZER_REDUCTOR_CALLBACK_NONE) return false;

	return callback(semantizer, unit, semantic_index);
}

bool semantizer_pattern_handle(semantizer_t *semantizer, size_t pattern_index, size_t semantic_index) {
	if(semantizer_matcher_callback_run(semantizer, pattern_index, semantic_index) == false) return false;

	semantizer_unit_t unit = {0};
	size_t size = semantizer_reductor_callback_run(semantizer, pattern_index, &unit, semantic_index);

	for(size_t i = 0; i < size; i++) {
		semantizer_unit_t *element = buffer_get(&semantizer->stream, semantic_index + i);
		semantizer_unit_clear(element);
	}
	buffer_remove(&semantizer->stream, semantic_index, size);
	buffer_insert(&semantizer->stream, semantic_index, &unit, 1);
	return true;
}

bool semantizer_process(semantizer_t *semantizer, size_t index) {
	for(size_t i = 0; i < semantizer->pattern_count; i++) {
		if(semantizer->patterns[index].level != semantizer->actual_level) continue;
		if(semantizer_pattern_handle(semantizer, i, index)) return true;
	}
	return false;
}

bool semantizer_pass(semantizer_t *semantizer) {
	uint64_t reductions_made = 0;

	for(size_t i = 0; i < semantizer->stream.used; i++) {
		bool status = semantizer_process(semantizer, i);
		
		if(status == true) reductions_made++;
	}

	semantizer->pass_count++;
	return reductions_made != 0;
}

void semantizer_level_run(semantizer_t *semantizer) {
	while(semantizer_pass(semantizer));
	semantizer->actual_level++;
}

void semantize(semantizer_t *semantizer) {
	while(semantizer->actual_level != semantizer->level_count) semantizer_level_run(semantizer);
}
