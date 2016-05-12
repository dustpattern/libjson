#ifndef JSON_PRIVATE_H
#define JSON_PRIVATE_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Public API */
#include "json.h"
#include "json_schema.h"

/*
 * Tokens.
 */
enum json_token_id {
	JSON_TOK_ERR          = '#',
	JSON_TOK_EOF          = '.',
	JSON_TOK_LIT          = 'L',
	JSON_TOK_COLON        = ':',
	JSON_TOK_COMMA        = ',',
	JSON_TOK_OBJECT_BEGIN = '{',
	JSON_TOK_OBJECT_END   = '}',
	JSON_TOK_ARRAY_BEGIN  = '[',
	JSON_TOK_ARRAY_END    = ']',
};

/**
 * A token.
 */
struct json_token {
	enum json_token_id     tok_id;
	char const           * tok_s;
};

/**
 * Garbage collector.
 */
struct json_gc {
	struct json_gc       * jgc_next;
	char                   jgc_data[];
};

/**
 * JSON parser handle.
 */
struct json_doc {
	FILE                 * jdoc_f;
	unsigned               jdoc_lineno;
	bool                   jdoc_nextc_avail;
	char                   jdoc_nextc;
	bool                   jdoc_lookahead_avail;
	struct json_token      jdoc_lookahead;
	struct json_gc       * jdoc_head;
	struct json_object   * jdoc_obj;
};

/**
 * Allocate using the garbage collector.
 */
#define _gcmalloc(doc, size, pp) \
({ \
	struct json_doc * const _doc = (doc); \
	struct json_gc * const _p = malloc(sizeof(struct json_gc) + (size)); \
 	*(pp) = _p ? (void *) _p->jgc_data : NULL; \
	if (_p) { \
		_p->jgc_next = _doc->jdoc_head; \
		_doc->jdoc_head = _p; \
	} \
	_p ? 0 : errno; \
})

/* Tokenizer methods.
 */
extern int json_consume_token(struct json_doc *, struct json_token *);
extern int json_peek_token(struct json_doc *, struct json_token *);

#endif
