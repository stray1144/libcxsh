#include <stddef.h>
#include <stdio.h>
#if !defined (__libcxsh)
#define __libcxsh

#include <stdint.h>
#include <stdbool.h>

#define _BSD_SOURCE

#define __packed __attribute__((__packed__))

// lexer

#define LEXER_TOKEN_KIND_COUNT 9
typedef enum lexer_token_kind_e {
	LEXER_TOKEN_UNDEFINED,
	LEXER_TOKEN_COMMENT,
	LEXER_TOKEN_IDENTIFIER,
	LEXER_TOKEN_FLOAT_LITERAL,
	LEXER_TOKEN_HEXADECIMAL_LITERAL,
	LEXER_TOKEN_INTEGER_LITERAL,
	LEXER_TOKEN_STRING_LITERAL,
	LEXER_TOKEN_PUNCTUATION,
	LEXER_TOKEN_NEWLINE,
} lexer_token_kind_t;

typedef uint32_t lexer_position_t;

typedef struct lexer_span_s {
	char *base;
	lexer_position_t length; 
} lexer_span_t;

typedef struct lexer_token_s {
	lexer_token_kind_t kind;
	lexer_position_t position;
	union {
		int64_t number;
		float real;
		lexer_span_t string;
		char character;
	};
} lexer_token_t;

typedef struct lexer_s lexer_t;

typedef bool (lexer_callback_t)(lexer_t *lexer, lexer_token_t *token);
#define LEXER_CALLBACK_NONE (lexer_callback_t *)(0)

struct lexer_s {
	char *data;
	lexer_callback_t *callback[LEXER_TOKEN_KIND_COUNT];

	lexer_position_t consumed;
};

bool lexer_init(lexer_t *lexer, char *data);
void lexer_clear(lexer_t *lexer);

bool lexer_callback_run(lexer_t *lexer, lexer_token_t *token, lexer_token_kind_t kind);
void lexer_callback_set(lexer_t *lexer, lexer_token_kind_t kind, lexer_callback_t *callback);

char *lexer_string_get(lexer_t *lexer);
void lexer_advance(lexer_t *lexer, lexer_position_t steps);

lexer_position_t lexer_position_get(lexer_t *lexer);

bool lex(lexer_t *lexer, lexer_token_t *token);

// buffer

#define BUFFER_RESIZE_FACTOR 3 / 2
#define BUFFER_INITIAL_SIZE 16

typedef struct buffer_s {
	void *data;
	size_t used;
	size_t object_size;
	size_t capacity;
} buffer_t;

bool buffer_init(buffer_t *buffer, size_t object_size);
void buffer_clear(buffer_t *buffer);

bool buffer_insert(buffer_t *buffer, size_t index, void *data, size_t count);
bool buffer_append(buffer_t *buffer, void *data, size_t count);
bool buffer_remove(buffer_t *buffer, size_t index, size_t count);
bool buffer_empty(buffer_t *buffer);
void *buffer_get(buffer_t *buffer, size_t index);

// semantizer

// enum semantic_kind_e is meant to be defined by the developer
enum semantizer_unit_kind_e : uint32_t;

typedef enum semantizer_unit_kind_e semantizer_unit_kind_t;

typedef struct semantizer_s semantizer_t;

typedef void (semantizer_data_free_t)(void *data);
#define SEMANTIZER_DATA_FREE_NONE (semantizer_data_free_t *)(0)

typedef struct semantizer_unit_s {
	semantizer_unit_kind_t kind;
	lexer_position_t position; // HERE. uses these 4 lost padding bytes, so structure is still 24 bytes
	void *data;
	semantizer_data_free_t *data_free;
} semantizer_unit_t;

typedef bool (semantizer_matcher_callback_t)(semantizer_t *semantizer, size_t start);
#define SEMANTIZER_MATCHER_CALLBACK_NONE (semantizer_matcher_callback_t *)(0)

typedef size_t (semantizer_reductor_callback_t)(semantizer_t *semantizer, semantizer_unit_t *unit, size_t start);
#define SEMANTIZER_REDUCTOR_CALLBACK_NONE (semantizer_reductor_callback_t *)(0)

typedef struct semantizer_pattern_s {
	semantizer_matcher_callback_t *match;
	semantizer_reductor_callback_t *reduct;
	uint64_t level;
} semantizer_pattern_t;

typedef bool (semantizer_forge_callback_t)(semantizer_unit_t *unit, lexer_token_t *token);
#define SEMANTIZER_FORGE_CALLBACK_NONE (semantizer_forge_callback_t *)(0)

typedef void (semantizer_log_callback_t)(void *context, uint32_t level, const char *message);
#define SEMANTIZER_LOG_CALLBACK_NONE (semantizer_log_callback_t *)(0)

typedef struct semantizer_s {
	buffer_t stream;

	semantizer_forge_callback_t **forge_callbacks;
	size_t forge_callback_count;

	semantizer_pattern_t *patterns;
	size_t pattern_count;

	uint32_t level_count;
	uint32_t actual_level;

	bool debug_enable;

	semantizer_log_callback_t *logger_function;
	void *logger_context;
	uint32_t logger_level;

	uint32_t name_offset;
	char **semantic_names;

	uint32_t pass_count;
} semantizer_t;

typedef enum semantizer_forge_status_e {
	FORGE_SUCCESS,
	FORGE_UNHANDLED
} semantizer_forge_status_t;

typedef struct semantizer_forge_result_s {
	semantizer_forge_status_t status;	
	size_t at;
} semantizer_forge_result_t;

bool semantizer_unit_init(semantizer_unit_t *unit, semantizer_unit_kind_t kind, void *data, semantizer_data_free_t *data_free);
void semantizer_unit_clear(semantizer_unit_t *unit);

void semantizer_token_trace(semantizer_unit_t *tracer, lexer_token_t *unit);

void semantizer_stream_trace(semantizer_t *semantizer, semantizer_unit_t *tracer, size_t index);
semantizer_unit_kind_t semantizer_stream_get(semantizer_t *semantizer, size_t index);
bool semantizer_stream_match(semantizer_t *semantizer, size_t index, semantizer_unit_kind_t kind);
void semantizer_stream_peek(semantizer_t *semantizer, size_t index, void **target, semantizer_data_free_t **target_free);
void semantizer_stream_steal(semantizer_t *semantizer, size_t index, void **target, semantizer_data_free_t **target_free);
size_t semantizer_stream_size(semantizer_t *semantizer);

bool semantizer_init(semantizer_t *semantizer);
void semantizer_clear(semantizer_t *semantizer);

void semantizer_pattern_setup(semantizer_t *semantizer, semantizer_pattern_t *patterns, size_t pattern_count, size_t level_count);

void semantizer_forge_setup(semantizer_t *semantizer, semantizer_forge_callback_t **callbacks, size_t callback_count);
semantizer_forge_result_t semantizer_forge_atomize(semantizer_t *semantizer, lexer_token_t *array, size_t array_size);

void semantizer_debug_setup(semantizer_t *semantizer, 
			    bool debug_enable, 
			    char **semantic_names, size_t name_offset, 
			    semantizer_log_callback_t *log, void *logger_context);

void semantize(semantizer_t *semantizer);

// cxREO

#define REO_MAGIC 0x004F4552
#define REO_VERSION 1

#define REO_STRING_SECTION 0
#define REO_CODE_SECTION 1
#define REO_DATA_SECTION 2
#define REO_OBJECT_SECTION 3
#define REO_BLOCK_SECTION 4

#define REO_SECTION_COUNT 5

typedef uint32_t reo_size_t;
typedef uint32_t reo_offset_t;

typedef enum reo_file_type_e : uint8_t {
	REO_TYPE_NONE,
	REO_TYPE_RELOCATABLE,
	REO_TYPE_SHARED,
	REO_TYPE_EXECUTABLE
} reo_file_type_t;

typedef struct reo_header_s {
	uint32_t magic;
	uint8_t version;
	reo_file_type_t type;
	uint16_t reserved;
	uint32_t objects;
	reo_offset_t entry;
	reo_size_t sizes[REO_SECTION_COUNT]; // 256
} reo_header_t;

typedef enum reo_entry_kind_e : uint8_t {
	REO_ENTRY_NONE,
	REO_ENTRY_EMBED,
	REO_ENTRY_SYMBOL,
	REO_ENTRY_RELOCATION,
	REO_ENTRY_IMPORT,
	REO_ENTRY_EXPORT,
} reo_entry_kind_t;

typedef struct __packed reo_entry_s {
	reo_size_t size;
	reo_offset_t name_string;
	reo_entry_kind_t type;
} reo_entry_t;

typedef struct __packed reo_embed_s {
	reo_entry_t entry;
	uint8_t data[];
} reo_embed_t;

#define REO_LOCATION_CODE 1
#define REO_LOCATION_DATA 2
#define REO_LOCATION_BLOCK 3

typedef struct reo_symbol_type_s {
	bool is_function; // false if object
	bool is_global; // false if local
	bool is_weak; // false if not
	uint8_t location : 2; // INVALID, CODE, DATA OR BLOCK
	uint8_t reserved : 3;
} reo_symbol_type_t;

typedef struct __packed reo_symbol_s {
	reo_entry_t entry;
	reo_offset_t location;
	reo_size_t size;
	reo_symbol_type_t type;
} reo_symbol_t;

typedef enum reo_relocation_type_e : uint8_t {
	REO_RELOCATION_ABSOLUTE,
	REO_RELOCATION_PC_RELATIVE
} reo_relocation_type_t;

typedef struct __packed reo_relocation_s {
	reo_entry_t entry;
	reo_offset_t patch_location;
	reo_relocation_type_t type;
} reo_relocation_t;

typedef enum reo_import_type_e : uint8_t {
	REO_IMPORT_OBJECT,
	REO_IMPORT_FUNCTION,
	REO_IMPORT_SERVICE
} reo_import_type_t;

typedef struct __packed reo_import_s {
	reo_entry_t entry;
	reo_offset_t version_string;
	reo_import_type_t type;
} reo_import_t;

typedef enum reo_export_type_e : uint8_t {
	REO_EXPORT_OBJECT,
	REO_EXPORT_FUNCTION,
	REO_EXPORT_ROUTINE
} reo_export_type_t;

typedef struct __packed reo_export_s {
	reo_entry_t entry;
	reo_offset_t location;
	reo_size_t size;
	reo_export_type_t type;
} reo_export_t;

typedef struct reo_file_s {
	reo_header_t header;
	buffer_t strings;   // buffer<char>
	buffer_t code;      // buffer<uint8_t>
	buffer_t data;      // buffer<uint8_t>
	buffer_t entries;   // buffer<reo_entry_t *>
} reo_file_t;

bool reo_file_init(reo_file_t *file);
void reo_file_clear(reo_file_t *file);

bool reo_file_load(reo_file_t *file, const char *path);
bool reo_file_save(reo_file_t *file, const char *path);

reo_offset_t reo_string_add(reo_file_t *file, const char *string);
// TODO: make safe
// void reo_string_remove(reo_file_t *file, reo_offset_t offset);
const char *reo_string_get(reo_file_t *file, reo_offset_t offset);

void reo_code_write(reo_file_t *file, void *source, reo_size_t size);
void reo_code_patch(reo_file_t *file, reo_offset_t offset, void *source, reo_size_t size);
const uint8_t *reo_code_get(reo_file_t *file);

reo_offset_t reo_data_add(reo_file_t *file, void *source, reo_size_t size);
void reo_data_remove(reo_file_t *file, reo_offset_t offset, reo_size_t size);
void *reo_data_get(reo_file_t *file, reo_offset_t offset);

void reo_block_reserve(reo_file_t *file, reo_size_t size);

bool reo_entry_init(reo_entry_t *entry, reo_size_t size, reo_offset_t name_string, reo_entry_kind_t type);
void reo_entry_clear(reo_entry_t *entry);

// returns the index of the entry
size_t reo_embed_add(reo_file_t *file, reo_offset_t name_string, uint8_t *data, reo_size_t size);
size_t reo_symbol_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t location, reo_size_t size, reo_symbol_type_t type);
size_t reo_relocation_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t patch_location, reo_relocation_type_t type);
size_t reo_import_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t version, reo_import_type_t type);
size_t reo_export_add(reo_file_t *file, reo_offset_t name_string, reo_offset_t location, reo_size_t size, reo_export_type_t type);

size_t reo_entry_count(reo_file_t *file);
reo_symbol_t *reo_symbol_search(reo_file_t *file, const char *name);

void reo_entry_remove(reo_file_t *file, size_t index);
reo_entry_t *reo_entry_get(reo_file_t *file, size_t index);

#endif
