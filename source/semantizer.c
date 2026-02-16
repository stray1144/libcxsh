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

// bool semantizer_stream_insert(semantizer_t *semantizer, size_t index, semantizer_unit_t *data, size_t count);
// bool semantizer_stream_append(semantizer_t *semantizer, semantizer_unit_t *data, size_t count);
// bool semantizer_stream_remove(semantizer_t *semantizer, size_t index, size_t count);
bool semantizer_stream_match(semantizer_t *semantizer, size_t index, semantizer_unit_kind_t kind) {
	semantizer_unit_t *unit = buffer_get(&semantizer->stream, index);
	if(unit == nullptr) return false;

	return unit->kind == kind;
}
void semantizer_stream_steal(semantizer_t *semantizer, size_t index, void **target, semantizer_data_free_t **target_free) {
	semantizer_unit_t *unit = buffer_get(&semantizer->stream, index);
	if(target != nullptr) {
		*target = unit->data;
		unit->data = nullptr;
	}
	if(target_free != nullptr) {
		*target_free = unit->data_free;
		unit->data_free = SEMANTIZER_DATA_FREE_NONE;
	}
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

void semantizer_pattern_setup(semantizer_t *semantizer, semantizer_pattern_t *patterns, size_t pattern_count) {
	semantizer->patterns = patterns;
	semantizer->pattern_count = pattern_count;
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

bool semantizer_forge_atomize(semantizer_t *semantizer, lexer_token_t *array, size_t array_size) {
	for (size_t i = 0; i < array_size; i++) {
		bool status = semantizer_forge_convert(semantizer, &array[i]);
		if(status == false) return false;
	}

	return true;
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

	buffer_remove(&semantizer->stream, semantic_index, size);
	buffer_insert(&semantizer->stream, semantic_index, &unit, 1);
	return true;
}

bool semantizer_process(semantizer_t *semantizer, size_t index) {
	for(size_t i = 0; i < semantizer->pattern_count; i++) {
		if(semantizer_pattern_handle(semantizer, i, index)) return true;
	}
	return false;
}

bool semantizer_cycle(semantizer_t *semantizer) {
	uint64_t reductions_made = 0;

	for(size_t i = 0; i < semantizer->stream.used; i++) {
		bool status = semantizer_process(semantizer, i);
		
		if(status == true) reductions_made++;
	}

	semantizer->cycles++;
	return reductions_made != 0;
}

void semantize(semantizer_t *semantizer) {
	while(semantizer_cycle(semantizer));
}
