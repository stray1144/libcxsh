#include <cxtoolchain/libcxsh.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// this function ASSUMES a valid REOFile_t
bool applyREOFileSize(REOFile_t *file, uint64_t newSize) {
	uint8_t *buffer = calloc(newSize, 1);
	if(!buffer) return false;

	memcpy(buffer, file->data, (file->size > newSize) ? newSize : file->size);

	free(file->data);

	file->data = buffer;
	file->capacity = newSize;

	return true;
}

void ensureREOFileSize(REOFile_t *file, uint64_t minimum) {
	if(minimum < file->capacity) return;

	uint64_t size = ((file->capacity * 2) < minimum) ? minimum : file->capacity * 2;

	applyREOFileSize(file, size);
}

REOFile_t *createREOFile(void) {
	REOFile_t *file = calloc(1, sizeof(REOFile_t));
	if(!file) return NULL;

	ensureREOFileSize(file, sizeof(REOHeader_t));	
	
	REOHeader_t *header = getREOHeader(file);

	header->magic = REO_MAGIC;
	header->version = REO_VERSION;

	header->offsets[REO_STRING_SECTION] = sizeof(REOHeader_t);
	header->offsets[REO_CODE_SECTION] = sizeof(REOHeader_t);
	header->offsets[REO_OBJECT_SECTION] = sizeof(REOHeader_t);

	file->size += sizeof(REOHeader_t);

	return file;
}

REOFile_t *openREOFile(const char *path);
bool saveREOFile(REOFile_t *file, const char *path) {
	FILE *handle = fopen(path, "w+");

	fwrite(file->data, file->size, 1, handle);

	fclose(handle);

	return true;
}

void destroyREOFile(REOFile_t *file) {
	if(!file) return;
	free(file->data);
	free(file);
}
