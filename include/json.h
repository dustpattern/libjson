/*
 * json.h
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

#ifndef __LIBJSON_H__
#define __LIBJSON_H__

#include <stdio.h>

/* String conversions */
#include "json_conv.h"

/**
 * Value types.
 */
enum json_value_type {
	JSON_VAL_LITERAL,
	JSON_VAL_OBJECT,
	JSON_VAL_ARRAY,
};

/**
 * Value.
 */
struct json_value {
	enum json_value_type   jval_type;
	union {
	struct json_object   * jval_object;
	struct json_array    * jval_array;
	char const           * jval_lit;
	};
};

/**
 * Array.
 */
struct json_array {
	int unsigned           jarr_length;
	struct json_value      jarr_values[];
};

/**
 * Key/value pair.
 */
struct json_tuple {
	char const           * jtup_key;
	struct json_value      jtup_val;
};

/**
 * Object.
 */
struct json_object {
	int unsigned           jobj_length;
	struct json_tuple      jobj_tuples[];
};

/* JSON document.
 */
typedef struct json_doc json_document_t;

// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
//                               Document                                   //
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

/**
 * Parse JSON document.
 */
extern int json_parse(FILE * f, json_document_t ** newdoc);

/**
 * Parse JSON document from string.
 */
extern int json_parse_string(char const * s, json_document_t ** newdoc);

/**
 * Parse JSON document from buffer.
 */
extern int json_parse_data(void * buf, size_t size, json_document_t ** newdoc);

/**
 * Free JSON document.
 */
extern void json_free(json_document_t * doc);

/**
 * Return root document object.
 */
extern struct json_object const * json_doc_object(json_document_t const *);

/**
 * Dump JSON document to file stream.
 */
extern void json_dump(json_document_t const * doc, FILE * f);

// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
//                              Object values                               //
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

/**
 * Fetch value at given path.
 *
 * Return NULL if the path cannot be reached.
 */
extern struct json_value const *
json_get_value(
	struct json_object const * const obj,
	char const * const path
	);

/**
 * Fetch literal string value at given path.
 *
 * Returns NULL if the path cannot be reached or if the corresponding value is
 * not a literal.
 */
extern char const *
json_get_literal(
	struct json_object const * obj,
	char const * path
	);

/**
 * Fetch object value at given path.
 *
 * Returns NULL if the path cannot be reached or if the corresponding value is
 * not an object.
 */
extern struct json_object const *
json_get_object(
	struct json_object const * obj,
	char const * path
	);

/**
 * Fetch array value at given path.
 *
 * Returns NULL if the path does not exist or if the corresponding value is
 * not an array.
 */
extern struct json_array const *
json_get_array(
	struct json_object const * obj,
	char const * path
	);

// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
//                             Literal values                               //
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

/**
 * Lookup given path in the object and return its value as a double.
 */
extern int
json_get_double(
	struct json_object const * const obj,
	char const * const path,
	double * const val
	);

/**
 * Lookup given path in the object and return its value as a signed integer.
 */
extern int
json_get_int(
	struct json_object const * const obj,
	char const * const path,
	int * const val
	);

/**
 * Lookup given path in the object and return its value as an unsigned
 * integer.
 */
extern int
json_get_uint(
	struct json_object const * const obj,
	char const * const path,
	unsigned * const val
	);

/**
 * Lookup given path in the object and return its value as an array of
 * unsigned 32-bit integers.
 */
extern int
json_get_array_of_uint32(
	struct json_array const * const jarr,
	uint32_t ** const outvec,
	unsigned * const outlen
	);

/**
 * Lookup given path in the object and return its value as an array of
 * unsigned 32-bit integers.
 */
extern int
json_get_array_of_uint64(
	struct json_array const  * const jarr,
	uint64_t ** const outvec,
	unsigned * const outlen
	);

/**
 * Lookup given path in the object and return its value as an array of
 * text values.
 */
extern int
json_get_array_of_text(
	struct json_array const  * const jarr,
	char const *** const outvec,
	unsigned * const outlen
	);

#endif
