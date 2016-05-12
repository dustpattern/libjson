/*
 * json_parse.c
 *
 * Simple JSON parser.
 *
 * https://github.com/e03213ac/libjson
 *
 * This work belongs to the Public Domain. Everyone is free to use, modify,
 * republish, sell or give away this work without prior consent from anybody.
 *
 * This software is provided on an "AS IS" basis, without warranty of any kind.
 * Use at your own risk! Under no circumstances shall the author(s) or
 * contributor(s) be liable for damages resulting directly or indirectly from
 * the use or non-use of this documentation.
 */

/* Private API */
#include "json_private.h"

/* Forward declarations */
static int _Array  (struct json_doc *, struct json_array **);
static int _Object (struct json_doc *, struct json_object **);

/** Return a syntax error */
#define RETURN_PARSE_ERROR() \
do { \
	return EINVAL; \
} while (0)

/**
 * Match the next token with an expected token.
 */
static inline int
_match(struct json_doc * const doc,
       enum json_token_id const id,
       struct json_token * const ret_tok)
{
	struct json_token tok;
	int err;

	if ((err = json_consume_token(doc, &tok)))
		return err;
	if (tok.tok_id != id)
		RETURN_PARSE_ERROR();
	if (ret_tok)
		*ret_tok = tok;

	return 0;
}

/**
 * Parse a literal, object, or array.
 */
static int
_Value(struct json_doc * const doc, struct json_value * const val)
{
	struct json_token tok;
	int err;

	if ((err = json_consume_token(doc, &tok)))
		return err;

	switch (tok.tok_id) {
	case JSON_TOK_LIT:
		val->jval_type = JSON_VAL_LITERAL;
		val->jval_lit  = tok.tok_s;
		break;
	case JSON_TOK_OBJECT_BEGIN:
		val->jval_type = JSON_VAL_OBJECT;
		if ((err = _Object(doc, &val->jval_object)))
			return err;
		break;
	case JSON_TOK_ARRAY_BEGIN:
		val->jval_type = JSON_VAL_ARRAY;
		if ((err = _Array(doc, &val->jval_array)))
			return err;
		break;
	default:
		RETURN_PARSE_ERROR();
	}

	return 0;
}

/**
 * Parse array.
 */
static int
_Array_r(
	struct json_doc      * const doc,
	int unsigned           const ix,
	struct json_array   ** const newarray )
{
	struct json_token      tok;
	struct json_array    * array;
	struct json_value      val;
	size_t                 size;
	int                    err;

	/* value */
	if ((err = _Value(doc, &val)))
		return err;

	/* , or ] */
	if ((err = json_consume_token(doc, &tok)))
		return err;
	switch (tok.tok_id) {
	case JSON_TOK_COMMA:
		/* continue parsing values in the array... */
		if ((err = _Array_r(doc, ix + 1, &array)))
			return err;
		break;
	case JSON_TOK_ARRAY_END:
		/* finalize array */
		size = sizeof(struct json_array) +
		       sizeof(struct json_value) * (ix + 1);
		if ((err = _gcmalloc(doc, size, &array)))
			return err;
		*array = (struct json_array) {
			.jarr_length = ix + 1,
		};
		break;
	default:
		RETURN_PARSE_ERROR();
	}

	/* store value in array */
	array->jarr_values[ix] = val;
	*newarray = array;
	return 0;
}

static int
_Array(
	struct json_doc    * const doc,
	struct json_array ** const newarray )
{
	struct json_token    tok;
	int                  err;

	/* empty array? */
	if ((err = json_peek_token(doc, &tok)))
		return err;
	if (tok.tok_id == JSON_TOK_ARRAY_END) {
		(void) json_consume_token(doc, &tok);
		size_t const size = sizeof(struct json_array);
		if ((err = _gcmalloc(doc, size, newarray)))
			return err;
		memset(*newarray, 0, size);
		return 0;
	}

	return _Array_r(doc, 0, newarray);
}

/**
 * Parse object.
 */
static int
_Object_r(
	struct json_doc      * const doc,
	int unsigned           const ix,
	struct json_object  ** const newobj )
{
	struct json_token      tok;
	struct json_object   * obj;
	char const           * key;
	struct json_value      val;
	size_t                 size;
	int                    err;

	/* key */
	if ((err = _match(doc, JSON_TOK_LIT, &tok)))
		return err;

	key = tok.tok_s;

	/* : */
	if ((err = _match(doc, JSON_TOK_COLON, NULL)))
		return err;

	/* value */
	if ((err = _Value(doc, &val)))
		return err;

	/* , or } */
	if ((err = json_consume_token(doc, &tok)))
		return err;
	switch (tok.tok_id) {
	case JSON_TOK_COMMA:
		/* continue parsing tuples in this object... */
		if ((err = _Object_r(doc, ix + 1, &obj)))
			return err;
		break;
	case JSON_TOK_OBJECT_END:
		/* allocate object */
		size = sizeof(struct json_object) +
		       sizeof(struct json_tuple ) * (ix + 1);
		if ((err = _gcmalloc(doc, size, &obj)))
			return err;
		*obj = (struct json_object) {
			.jobj_length = ix + 1,
		};
		break;
	default:
		RETURN_PARSE_ERROR();
	}

	/* store tuple in object */
	obj->jobj_tuples[ix] = (struct json_tuple) { key, val };
	*newobj = obj;
	return 0;
}

static int
_Object(
	struct json_doc     * const doc,
	struct json_object ** const newobject )
{
	struct json_token     tok;
	int                   err;

	/* empty object? */
	if ((err = json_peek_token(doc, &tok)))
		return err;
	if (tok.tok_id == JSON_TOK_OBJECT_END) {
		(void) json_consume_token(doc, &tok);
		size_t const size = sizeof(struct json_object);
		if ((err = _gcmalloc(doc, size, newobject)))
			return err;
		memset(*newobject, 0, size);
		return 0;
	}

	return _Object_r(doc, 0, newobject);
}

/**
 * Entry point for parsing.
 */
static int
_Start(struct json_doc     * const doc,
       struct json_object ** const newobj )
{
	struct json_object * obj;
	int err;

	if ((err = _match(doc, JSON_TOK_OBJECT_BEGIN, NULL)))
		return err;

	if ((err = _Object(doc, &obj)))
		return err;

	*newobj = obj;
	return 0;
}

void
json_free(struct json_doc * const doc)
{
	struct json_gc * next;
	for (struct json_gc * p = doc->jdoc_head; p; p = next) {
		next = p->jgc_next;
		free(p);
	}
	free(doc);
}

int
json_parse(FILE * const f, struct json_doc ** newdoc)
{
	struct json_doc * doc;
	int err;

	if ((doc = malloc(sizeof(*doc))) == NULL) {
		err = errno;
		goto fail_malloc;
	}
	*doc = (struct json_doc) {
		.jdoc_f      = f,
		.jdoc_lineno = 1,
	};

	if ((err = _Start(doc, &doc->jdoc_obj)))
		goto fail_parse;

	*newdoc = doc;
	return 0;

fail_parse:
	json_free(doc);
fail_malloc:
	*newdoc = NULL;
	return err;
}

#ifdef __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int
json_parse_string(char const * str, struct json_doc ** newdoc)
{
	*newdoc = null;
	return enosys;
}
int
json_parse_data(void * buf, size_t size, struct json_doc ** newdoc)
{
	*newdoc = null;
	return enosys;
}
#pragma GCC diagnostic pop
#else
int
json_parse_string(char const * str, struct json_doc ** newdoc)
{
	int err;

	/* duplicate input stream and open as a FILE stream */
	char * const dup = strdup(str);
	if (dup == NULL) {
		err = errno;
		goto fail_strdup;
	}
	FILE * const f = fmemopen(dup, strlen(str), "r");
	if (f == NULL) {
		err = errno;
		goto fail_fmemopen;
	}

	/* parse document */
	struct json_doc * doc;
	if ((err = json_parse(f, &doc)))
		goto fail_parse;

	doc->jdoc_f = NULL;
	fclose(f);
	free(dup);

	*newdoc = doc;
	return 0;

fail_parse:
	fclose(f);
fail_fmemopen:
	free(dup);
fail_strdup:
	*newdoc = NULL;
	return err;
}

int
json_parse_data(void * buf, size_t size, struct json_doc ** newdoc)
{
	int err;

	/* open as a FILE stream */
	FILE * const f = fmemopen(buf, size, "rb");
	if (f == NULL) {
		err = errno;
		goto fail_fmemopen;
	}

	/* parse document */
	struct json_doc * doc;
	if ((err = json_parse(f, &doc)))
		goto fail_parse;

	doc->jdoc_f = NULL;
	fclose(f);

	*newdoc = doc;
	return 0;

fail_parse:
	fclose(f);
fail_fmemopen:
	*newdoc = NULL;
	return err;
}
#endif
