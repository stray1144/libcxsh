#include <cxtoolchain/libcxsh.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool buffer_bounds_check(buffer_t *buffer, size_t index) {
	if(index >= buffer->used) return false;

	return true;
}

size_t buffer_size_calculate(buffer_t *buffer, size_t index) {
	return buffer->object_size * index;
}

bool buffer_size_ensure(buffer_t *buffer, size_t minimum) {
	if(minimum <= buffer->capacity) return true;

	size_t new_size = buffer->capacity * BUFFER_RESIZE_FACTOR;
	if(new_size < minimum) new_size = minimum;

	void *new_data = calloc(new_size, buffer->object_size);
	if(new_data == nullptr) return false;

	memcpy(new_data, buffer->data, buffer_size_calculate(buffer, buffer->used));

	free(buffer->data);
	buffer->data = new_data;

	buffer->capacity = new_size;
	buffer->generation++;

	return true;
}

void *buffer_pointer_calculate(buffer_t *buffer, size_t index) {
	return buffer->data + buffer_size_calculate(buffer, index);
}

bool buffer_shift_right(buffer_t *buffer, size_t index, size_t value) {
	if(buffer_size_ensure(buffer, buffer->used + value) == false) return false;

	buffer->used += value;

	memmove(
		buffer_pointer_calculate(buffer, index + value), 
		buffer_pointer_calculate(buffer, index),
		buffer_size_calculate(buffer, buffer->used - (index + value))
	);

	return true;
}

bool buffer_shift_left(buffer_t *buffer, size_t index, size_t value) {
	memmove(
		buffer_pointer_calculate(buffer, index), 
		buffer_pointer_calculate(buffer, index + value),
		buffer_size_calculate(buffer, buffer->used - (index + value))
	);

	buffer->used -= value;

	return true;
}

bool buffer_insert(buffer_t *buffer, size_t index, void *data, size_t count) {
	// buffer_insert uses another kind of bound checking,
	if((index > buffer->used) || buffer_shift_right(buffer, index, count) == false) return false;

	memcpy(
		buffer_pointer_calculate(buffer, index), 
		data,
		buffer_size_calculate(buffer, count)
	);

	return true;
}

bool buffer_append(buffer_t *buffer, void *data, size_t count) {
	return buffer_insert(buffer, buffer->used, data, count);
}

bool buffer_remove(buffer_t *buffer, size_t index, size_t count) {
	if(buffer_bounds_check(buffer, index) == false) return false;

	buffer_shift_left(buffer, index, count);

	return true;
}

bool buffer_empty(buffer_t *buffer) {
	return buffer_remove(buffer, 0, buffer->used);
}

void *buffer_get(buffer_t *buffer, size_t index) {
	if(buffer_bounds_check(buffer, index) == false) return nullptr;

	return buffer_pointer_calculate(buffer, index);
}

bool buffer_init(buffer_t *buffer, size_t object_size) {
	if(buffer == nullptr) return false;

	buffer_clear(buffer);

	buffer->object_size = object_size;

	buffer_size_ensure(buffer, BUFFER_INITIAL_SIZE);

	return true;
}

void buffer_clear(buffer_t *buffer) {
	if(buffer == nullptr) return;

	free(buffer->data); // assuming free(nullptr) is valid...

	memset(buffer, 0, sizeof(buffer_t));
}

