#include <cxtoolchain/libcxsh.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void gapREOFileContent(REOFile_t *file, offset_t offset, int32_t size) {
	ensureREOFileSize(file, file->capacity + size);

	memmove(file->data + offset + size, file->data + offset, file->size - offset);

	file->size += size;
}

void resizeREOSection(REOFile_t *file, uint8_t id, uint32_t size) {
	REOHeader_t *header = getREOHeader(file);

	uint32_t gap = size - header->sizes[id];

	header->sizes[id] = size;
	for(uint8_t i = id + 1; i < REO_SECTION_COUNT; i++) header->offsets[i] += gap;
}

offset_t addREOString(REOFile_t *file, const char *string) {
	uint64_t size = strlen(string) + 1;

	REOHeader_t *header = getREOHeader(file);

	offset_t offset = header->offsets[REO_STRING_SECTION] + header->sizes[REO_STRING_SECTION];

	resizeREOSection(file, REO_STRING_SECTION, header->sizes[REO_STRING_SECTION] + size);
	gapREOFileContent(file, offset, size); // HEADER POINTER IS INVALIDATED HERE
	memcpy(file->data + offset, string, size);

	return offset - sizeof(REOHeader_t);
}

void writeREOCode(REOFile_t *file, uint8_t *source, uint32_t size) {
	REOHeader_t *header = getREOHeader(file);

	uint32_t gap = size - header->sizes[REO_CODE_SECTION]; 
	offset_t section = header->offsets[REO_CODE_SECTION];

	gapREOFileContent(file, section, gap); // HEADER POINTER IS INVALIDATED HERE
	resizeREOSection(file, REO_CODE_SECTION, size);
	memcpy(file->data + section, source, size);
}

void patchREOCode(REOFile_t *file, uint32_t offset, void *source, uint32_t size) {
	REOHeader_t *header = getREOHeader(file);

	memcpy(file->data + header->offsets[REO_CODE_SECTION] + offset, source, size);
}

void appendREOEntry(REOFile_t *file, REOEntry_t *entry) {
	REOHeader_t *header = getREOHeader(file);

	offset_t offset = header->offsets[REO_OBJECT_SECTION] + header->sizes[REO_OBJECT_SECTION];
	resizeREOSection(file, REO_OBJECT_SECTION, header->sizes[REO_OBJECT_SECTION] + entry->size);
	gapREOFileContent(file, offset, entry->size); // HEADER POINTER IS INVALIDATED HERE

	memcpy(file->data + offset, entry, entry->size);
}

void addREOEmbed(REOFile_t *file, offset_t name, uint8_t *data, uint64_t size) {
	REOEmbedEntry_t *entry = calloc(1, sizeof(REOEmbedEntry_t) + size);
	entry->entry = (REOEntry_t) {sizeof(REOEmbedEntry_t) + size, name, reo_entry_embed};

	memcpy(entry->data, data, size);

	appendREOEntry(file, (void *)entry);

	free(entry);
}

void addREOSymbol(REOFile_t *file, offset_t name, uint64_t address, uint64_t size, uint8_t symbolType) {
	REOSymbolEntry_t entry = {0};
	entry.entry = (REOEntry_t) {sizeof(REOSymbolEntry_t), name, reo_entry_symbol};
	entry.address = address;
	entry.size = size;
	entry.symbolType = symbolType;

	appendREOEntry(file, (void *)&entry);
}

void addREORelocation(REOFile_t *file, offset_t name, offset_t patchLocation, uint8_t relocationType) {
	REORelocationEntry_t entry = {0};
	entry.entry = (REOEntry_t) {sizeof(REOSymbolEntry_t), name, reo_entry_relocation};
	entry.patchLocation = patchLocation;
	entry.relocationType = relocationType;

	appendREOEntry(file, (void *)&entry);
}

void addREOImport(REOFile_t *file, offset_t name, offset_t versionString, uint8_t importType) {
	REOImportEntry_t entry = {0};
	entry.entry = (REOEntry_t) {sizeof(REOImportEntry_t), name, reo_entry_import};
	entry.versionString = versionString;
	entry.importType = importType;

	appendREOEntry(file, (void *)&entry);
}

void addREOExport(REOFile_t *file, offset_t name, uint64_t address, uint64_t size, uint8_t exportType) {
	REOExportEntry_t entry = {0};
	entry.entry = (REOEntry_t) {sizeof(REOExportEntry_t), name, reo_entry_export};
	entry.address = address;
	entry.size = size;
	entry.exportType = exportType;

	appendREOEntry(file, (void *)&entry);
}

void removeREOEntry(REOFile_t *file, REOEntry_t *entry) {
	REOHeader_t *header = getREOHeader(file);

	offset_t offset = (uint64_t) entry - (uint64_t) file->data; 
	resizeREOSection(file, REO_OBJECT_SECTION, header->sizes[REO_OBJECT_SECTION] - entry->size);
	gapREOFileContent(file, offset+entry->size, -entry->size);

	resetREOEntries(file);
}
