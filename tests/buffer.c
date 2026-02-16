#include <cxtoolchain/libcxsh.h>

#include "test.h"

#include <stdio.h>
#include <string.h>

char *message = "Hello world\n";

int main(void) {
	bool status = true;
	buffer_t buffer = {0};
	status = buffer_init(&buffer, sizeof(char));
	test_assert(status == true, "The buffer couldn't initialize");

	status = buffer_append(&buffer, message, strlen(message));
	test_assert(status == true, "Couldn't append the message");

	test_assert(buffer.used == 12, "Used count isn't valid");
	test_assert(strncmp(buffer_get(&buffer, 0), message, strlen(message)) == 0, "Data isn't equal");

	buffer_clear(&buffer);
}
