#include <cxtoolchain/libcxsh.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


bool reo_file_init(reo_file_t *file) {
	if(file == nullptr) return false;	

	reo_file_clear(file);

	file->header.magic = REO_MAGIC;	
	buffer_init(&file->strings, sizeof(char));
	buffer_init(&file->code, sizeof(uint8_t));
	buffer_init(&file->data, sizeof(uint8_t));
	buffer_init(&file->entries, sizeof(reo_entry_t *));

	reo_string_add(file, ""); // null string @ 0x00

	return true;
}

void reo_file_clear(reo_file_t *file) {
	if(file == nullptr) return;

	buffer_clear(&file->strings);
	buffer_clear(&file->code);
	buffer_clear(&file->data);

	for(size_t i = 0; i < file->entries.used; i++) {
		reo_entry_t **entry = buffer_get(&file->entries, i);
		free(*entry);
	}
	buffer_clear(&file->entries);

	memset(file, 0, sizeof(reo_file_t));
}

bool reo_file_load(reo_file_t *file, const char *path);

bool reo_header_serialize(reo_file_t *file, FILE *handle) {
	bool status = true;

	status &= fwrite(&file->header.magic, sizeof(uint32_t), 1, handle) == 1; 
	status &= fwrite(&file->header.version, sizeof(uint8_t), 1, handle) == 1; 
	status &= fwrite(&file->header.type, sizeof(reo_file_type_t), 1, handle) == 1; 
	status &= fwrite(&file->header.reserved, sizeof(uint16_t), 1, handle) == 1; 
	status &= fwrite(&file->header.objects, sizeof(uint32_t), 1, handle) == 1; 
	status &= fwrite(&file->header.entry_name, sizeof(reo_offset_t), 1, handle) == 1; 
	status &= fwrite(file->header.sizes, sizeof(reo_offset_t), REO_SECTION_COUNT, handle) == REO_SECTION_COUNT; 

	return status;
}

bool reo_string_serialize(reo_file_t *file, FILE *handle) {
	bool status = true;

	status &= fwrite(buffer_get(&file->strings, 0), file->strings.used, 1, handle) == 1;

	return status;
}

bool reo_code_serialize(reo_file_t *file, FILE *handle) {
	bool status = true;

	status &= fwrite(buffer_get(&file->code, 0), file->header.sizes[REO_CODE_SECTION], 1, handle) == 1;

	return status;
}

bool reo_data_serialize(reo_file_t *file, FILE *handle) {
	bool status = true;

	status &= fwrite(buffer_get(&file->data, 0), file->data.used, 1, handle) == 1;

	return status;
}

bool reo_entry_serialize(FILE *handle, reo_entry_t *entry) {
	bool status = true;

	status &= fwrite(entry, entry->size, 1, handle) == 1;

	return status;
}

bool reo_object_serialize(reo_file_t *file, FILE *handle) {
	bool status = true;

	uint64_t index = 0;
	while(true) {
		reo_entry_t *entry = reo_entry_get(file, index);
		if(!entry) break;

		status &= reo_entry_serialize(handle, entry);

		index++;
	}

	return status;
}

bool reo_file_save(reo_file_t *file, const char *path) {
	bool status = true;

	FILE *handle = fopen(path, "wb+");

	status &= reo_header_serialize(file, handle);
	status &= reo_string_serialize(file, handle);
	status &= reo_code_serialize(file, handle);
	status &= reo_data_serialize(file, handle);
	status &= reo_object_serialize(file, handle);

	fclose(handle);

	return status;
}

reo_file_type_t reo_type_get(reo_file_t *file) {
	return file->header.type;
}

void reo_type_set(reo_file_t *file, reo_file_type_t type) {
	file->header.type = type;
}

reo_offset_t reo_string_add(reo_file_t *file, const char *string) {
	size_t string_size = strlen(string) + 1;
	reo_offset_t offset = file->strings.used;
	buffer_append(&file->strings, (void *)string, string_size);
	file->header.sizes[REO_STRING_SECTION] += string_size;
	return offset;
};

// void reo_string_remove(reo_file_t *file, reo_offset_t offset) {
// 	size_t string_size = strlen(buffer_get(&file->strings, offset)) + 1;
//
// 	buffer_remove(&file->strings, offset, string_size);
// 	file->header.sizes[REO_STRING_SECTION] -= string_size;
// }

const char *reo_string_get(reo_file_t *file, reo_offset_t offset) {
	return buffer_get(&file->strings, offset);
}

void reo_code_write(reo_file_t *file, void *source, reo_size_t size) {
	buffer_empty(&file->code);
	buffer_append(&file->code, source, size);
	file->header.sizes[REO_CODE_SECTION] = size;
}

void reo_code_patch(reo_file_t *file, reo_offset_t offset, void *source, reo_size_t size) {
	buffer_remove(&file->code, offset, size);
	buffer_insert(&file->code, offset, source, size);
}

const uint8_t *reo_code_get(reo_file_t *file) {
	return buffer_get(&file->code, 0);
}

reo_offset_t reo_data_add(reo_file_t *file, void *source, reo_size_t size) {
	reo_offset_t offset = file->data.used;
	buffer_append(&file->data, (void *)source, size);
	file->header.sizes[REO_DATA_SECTION] += size;
	return offset;
}

void reo_data_remove(reo_file_t *file, reo_offset_t offset, reo_size_t size) {
	buffer_remove(&file->data, offset, size);
	file->header.sizes[REO_DATA_SECTION] -= size;
}

void *reo_data_get(reo_file_t *file, reo_offset_t offset) {
	return buffer_get(&file->data, offset);
}

void reo_block_reserve(reo_file_t *file, reo_size_t size) {
	file->header.sizes[REO_BLOCK_SECTION] = size;
}

bool reo_entry_init(reo_entry_t *entry, reo_size_t size, reo_offset_t name_string, reo_entry_kind_t type) {
	if(entry == nullptr) return false;

	reo_entry_clear(entry);

	entry->size = size;
	entry->name_string = name_string;
	entry->type = type;

	return true;
}

void reo_entry_clear(reo_entry_t *entry) {
	if(entry == nullptr) return;

	memset(entry, 0, sizeof(reo_entry_t));
}

void *reo_entry_create(reo_size_t size, reo_offset_t name_string, reo_entry_kind_t kind) {
	reo_entry_t *entry = calloc(size, 1);
	reo_entry_init(entry, size, name_string, kind);
	return entry;
}

void reo_entry_destroy(reo_entry_t *entry) {
	if(entry == nullptr) return;
	reo_entry_clear(entry);
	free(entry);
}

size_t reo_entry_add(reo_file_t *file, reo_entry_t *entry) {
	size_t index = file->entries.used;
	buffer_append(&file->entries, &entry, 1);
	file->header.objects++;
	file->header.sizes[REO_OBJECT_SECTION] += entry->size; 
	return index;
}

size_t reo_embed_add(reo_file_t *file, reo_offset_t name_string, uint8_t *data, reo_size_t size) {
	reo_embed_t *entry = reo_entry_create(sizeof(reo_entry_t) + size, name_string, REO_ENTRY_EMBED);

	memcpy(entry->data, data, size);

	return reo_entry_add(file, (void *)entry);
}

size_t reo_symbol_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t location, reo_symbol_type_t type) {
	reo_symbol_t *entry = reo_entry_create(sizeof(reo_symbol_t), name_string, REO_ENTRY_SYMBOL);

	entry->location = location;
	entry->type = type;

	return reo_entry_add(file, (void *)entry);
}

size_t reo_relocation_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t patch_location, reo_relocation_type_t type) {
	reo_relocation_t *entry = reo_entry_create(sizeof(reo_relocation_t), name_string, REO_ENTRY_RELOCATION);

	entry->patch_location = patch_location;
	entry->type = type;

	return reo_entry_add(file, (void *)entry);

}

size_t reo_import_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t version_string, reo_import_type_t type) {
	reo_import_t *entry = reo_entry_create(sizeof(reo_import_t), name_string, REO_ENTRY_IMPORT);

	entry->version_string = version_string;
	entry->type = type;

	return reo_entry_add(file, (void *)entry);
}

size_t reo_export_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t location, reo_export_type_t type) {
	reo_export_t *entry = reo_entry_create(sizeof(reo_export_t), name_string, REO_ENTRY_EXPORT);

	entry->location = location;
	entry->type = type;

	return reo_entry_add(file, (void *)entry);
}

void reo_entry_remove(reo_file_t *file, size_t index) {
	reo_entry_t *entry = reo_entry_get(file, index);
	reo_entry_destroy(entry);
	buffer_remove(&file->entries, index, 1);
}

reo_entry_t *reo_entry_get(reo_file_t *file, size_t index) {
	reo_entry_t **entry = buffer_get(&file->entries, index);
	return (entry == nullptr) ? nullptr : *entry;
}
