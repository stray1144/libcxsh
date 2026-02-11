#include <stdio.h>
#if !defined (__libcxsh)
#define __libcxsh

#include <stdint.h>
#include <stdbool.h>

#define TOKEN_KIND_COUNT 9
typedef enum tokenKind_e {
	token_undefined = 0,
	token_comment = 1,
	token_identifier = 2,
	token_hex_literal = 3,
	token_integer_literal = 4,
	token_float_literal = 5,
	token_string_literal = 6,
	token_punctuation = 7,
	token_newline = 8,
} tokenKind_t;

typedef struct stringSlice_s {
	char *base;
	uint64_t length;
} slice_t;

typedef struct token_s {
	tokenKind_t kind;
	union {
		int64_t number;
		double _float;
		slice_t string;
		char character;
	};
} token_t;

typedef struct lexer_s lexer_t;

typedef bool (lexerBehaviour_t)(lexer_t *lexer, token_t *token);

struct lexer_s {
	char *data;
	uint64_t consumed;
	lexerBehaviour_t *behaviours[TOKEN_KIND_COUNT];
};

bool initLexer(lexer_t *lexer, char *data);

bool triggerLexerBehaviour(lexer_t *lexer, token_t *token, tokenKind_t kind);
void configurateLexerBehaviour(lexer_t *lexer, tokenKind_t set, lexerBehaviour_t *behaviour);

static inline char *getLexerString(lexer_t *lexer) {
	return lexer->data + lexer->consumed;
}
static inline void advanceLexer(lexer_t *lexer, uint64_t steps) {
	lexer->consumed += steps;
}

bool lex(lexer_t *lexer, token_t *token);

#define REO_MAGIC 0x004F4552
#define REO_VERSION 1
#define REO_STRING_SECTION 0
#define REO_CODE_SECTION 1
#define REO_OBJECT_SECTION 2
#define REO_SECTION_COUNT 3

typedef uint32_t offset_t;

typedef enum REOFileType_e : uint8_t {
	reo_type_none,
	reo_type_relocatable,
	reo_type_shared,
	reo_type_executable
} REOFileType_t;

typedef struct __attribute__((__packed__)) REOHeader_s {
	uint32_t magic;
	uint8_t version;
	REOFileType_t type;
	uint16_t reserved;
	offset_t entry;
	uint32_t objects;
	offset_t offsets[3];
	uint32_t sizes[3];
} REOHeader_t;

typedef enum REOEntryKind_e : uint8_t {
	reo_entry_none,
	reo_entry_embed,
	reo_entry_symbol,
	reo_entry_relocation,
	reo_entry_import,
	reo_entry_export,
} REOEntryKind_t;

typedef struct __attribute__((__packed__)) REOEntry_s {
	uint32_t size;
	offset_t name;
	REOEntryKind_t type;
} REOEntry_t;

typedef struct __attribute__((__packed__)) REOEmbedEntry_s {
	REOEntry_t entry;
	uint8_t data[];
} REOEmbedEntry_t;

typedef struct __attribute__((__packed__)) REOSymbolEntry_s {
	REOEntry_t entry;
	offset_t address;
	uint32_t size;
	uint8_t symbolType;
} REOSymbolEntry_t;

typedef struct __attribute__((__packed__)) REORelocationEntry_s {
	REOEntry_t entry;
	offset_t patchLocation;
	uint8_t relocationType;
} REORelocationEntry_t;

typedef struct __attribute__((__packed__)) REOImportEntry_s {
	REOEntry_t entry;
	offset_t versionString;
	uint8_t importType;
} REOImportEntry_t;

typedef struct __attribute__((__packed__)) REOExportEntry_s {
	REOEntry_t entry;
	offset_t address;
	uint32_t size;
	uint8_t exportType;
} REOExportEntry_t;

typedef struct REOFile_s {
	uint8_t *data;
	uint64_t capacity;
	uint64_t size;
	uint64_t objectOffset;
} REOFile_t;

REOFile_t *createREOFile(void);
REOFile_t *openREOFile(const char *path);
void ensureREOFileSize(REOFile_t *file, uint64_t minimum);
bool saveREOFile(REOFile_t *file, const char *path);
void destroyREOFile(REOFile_t *file);


REOHeader_t *getREOHeader(REOFile_t *file);
const char *getREOString(REOFile_t *file, uint32_t offset);
const uint8_t *getREOCode(REOFile_t *file);
REOEntry_t *getNextREOEntry(REOFile_t *file);
void resetREOEntries(REOFile_t *file);


offset_t addREOString(REOFile_t *file, const char *string);
void writeREOCode(REOFile_t *file, uint8_t *source, uint32_t size);
void patchREOCode(REOFile_t *file, uint32_t offset, void *source, uint32_t size);

void addREOEmbed     (REOFile_t *file, offset_t name, uint8_t *data, uint64_t size);
void addREOSymbol    (REOFile_t *file, offset_t name, uint64_t address, uint64_t size, uint8_t symbolType);
void addREORelocation(REOFile_t *file, offset_t name, offset_t patch, uint8_t type);
void addREOImport    (REOFile_t *file, offset_t name, offset_t version, uint8_t type);
void addREOExport    (REOFile_t *file, offset_t name, uint64_t address, uint64_t size, uint8_t exportType);

void removeREOEntry(REOFile_t *file, REOEntry_t *entry);
void rebuildREOFile(REOFile_t *file);


#endif
