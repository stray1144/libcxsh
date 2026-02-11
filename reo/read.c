#include <cxtoolchain/libcxsh.h>
#include <stdint.h>
#include <stdio.h>

REOHeader_t *getREOHeader(REOFile_t *file) {
	return (REOHeader_t *)file->data;
}
const char *getREOString(REOFile_t *file, uint32_t offset) {
	REOHeader_t *header = getREOHeader(file);

	if(offset > header->sizes[REO_STRING_SECTION]) return NULL;

	return (char *)file->data + header->offsets[REO_STRING_SECTION] + offset;
}

const uint8_t *getREOCode(REOFile_t *file) {
	REOHeader_t *header = getREOHeader(file);

	return file->data + header->offsets[REO_CODE_SECTION];
}

REOEntry_t *getNextREOEntry(REOFile_t *file) {
	REOHeader_t *header = getREOHeader(file);

	if(file->objectOffset < header->offsets[REO_OBJECT_SECTION]) resetREOEntries(file);
	if(file->objectOffset >= file->size) return 0;	

	REOEntry_t *entry = (void *)file->data + file->objectOffset;

	file->objectOffset += entry->size;

	return entry;
}

void resetREOEntries(REOFile_t *file) {
	REOHeader_t *header = getREOHeader(file);

	file->objectOffset = header->offsets[REO_OBJECT_SECTION];
}
