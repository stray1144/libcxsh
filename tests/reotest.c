#include <cxtoolchain/libcxsh.h>

#include "test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t code[] = {
	0x05, 0x00, 
	0x00, 0x01, 
	0b11000000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // add $1, $1 
};

uint8_t data[] = "hello world";

uint8_t deleted[] = {
	#embed "deleted.cxo" 
};

uint8_t undeleted[] = {
	#embed "undeleted.cxo" 
};

int main() {
	REOFile_t *file = createREOFile();
	addREOString(file, "stray1144");
	writeREOCode(file, code, sizeof(code));
	uint64_t address = 0x1234567890;
	patchREOCode(file, 5, &address, 8);

	offset_t prefix = addREOString(file, "text");
	(void)(prefix);

	addREOEmbed(file, prefix, data, sizeof(data));
	addREOSymbol(file, addREOString(file, "__entry"), 0x100, 8, 0);
	addREOSymbol(file, addREOString(file, "loop_r1"), 0x100, 8, 0);

	test_assert(memcmp(undeleted, file->data, sizeof(undeleted)) == 0, "The data with no deleted entries doesn't match");

	while(true) {
		REOEntry_t *entry = getNextREOEntry(file);
		if(!entry) break;
		printf("removing entry %s (%d) sized %d\n", getREOString(file, entry->name), entry->type, entry->size);
		removeREOEntry(file, entry);	
	}
	
	test_assert(memcmp(deleted, file->data, sizeof(deleted)) == 0, "The data with deleted entries doesn't match");

	destroyREOFile(file);

	return 0;
}
