#include <cxtoolchain/libcxsh.h>

#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t unpatched_code[] = {
	0x05, 0x00, 
	0x00, 0x01, 
	0b11000000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // add $1, $1 
};

uint8_t patched_code[] = {
	0x05, 0x00, 
	0x00, 0x01, 
	0b11000000, 0x44, 0x11, 0x44, 0x11, 0x00, 0x00, 0x00, 0x00 // add $1, $1 
};

uint8_t data[] = "hello world";


void create_test(void) {
	reo_file_t file = {0};
	reo_file_init(&file);

	reo_offset_t string = reo_string_add(&file, "some_data");
	reo_string_add(&file, "stray1144");
	test_assert(string == 0, "First added string index isn't 0. weird");

	reo_code_write(&file, unpatched_code, sizeof(unpatched_code));
	test_assert(memcmp(buffer_get(&file.code, 0), unpatched_code, sizeof(unpatched_code)) == 0, "Data is wrong");

	uint64_t weird_address = 0x11441144;
	uint64_t picky_address = 0x44114411;
	reo_code_patch(&file, 5, &weird_address, 8);
	test_assert(memcmp(buffer_get(&file.code, 0), patched_code, sizeof(patched_code)) == 0, "Patched data is wrong");

	reo_data_add(&file, &weird_address, sizeof(weird_address));
	reo_data_add(&file, &picky_address, sizeof(picky_address));

	reo_embed_add(&file, string, data, sizeof(data));
	reo_symbol_add(&file, string, weird_address, 8, REO_SYMBOL_OBJECT);
	reo_relocation_add(&file, string, picky_address, REO_RELOCATION_ABSOLUTE);
	reo_import_add(&file, string, reo_string_add(&file, "1.0.0"), REO_IMPORT_OBJECT);
	reo_export_add(&file, string, weird_address, 8, REO_EXPORT_OBJECT);

	reo_file_save(&file, "test.cxo");
	reo_file_clear(&file);

}

void load_test(void) {
	// reo_file_t file = {0};
	// reo_file_load(&file, "test.cxo");
	//
	//
	//
}

int main(void) {
	create_test();
	load_test();

	return 0;
}
