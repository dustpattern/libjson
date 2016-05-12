/*
 * json_schema.h
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

#ifndef __LIBJSON_SCHEMA_H__
#define __LIBJSON_SCHEMA_H__

#include <stdbool.h>

#include "json.h"

/**
 * Scheme operation codes.
 */
enum json_schema_op {
	JSON_SCHEMA_OP_IFEQ,       // JSON_IFEQ(key, val, schema)
	JSON_SCHEMA_OP_RECURSIVE,  // JSON_DESCEND(key, schema)
	JSON_SCHEMA_OP_DEFINE,     // JSON_REQUIRE_type(key, type, ...)
	                           // JSON_OPTIONA_type(key, type, ...)
};

/**
 * Data types.
 */
enum json_schema_type {
	JSON_SCHEMA_TYPE_DOUBLE,
	JSON_SCHEMA_TYPE_INT,
	JSON_SCHEMA_TYPE_OBJ,
	JSON_SCHEMA_TYPE_TEXT,
	JSON_SCHEMA_TYPE_TEXTV,
	JSON_SCHEMA_TYPE_UINT,
	JSON_SCHEMA_TYPE_UINT32V,
	JSON_SCHEMA_TYPE_UINT64V,
};

/*
 * Used to recursively validate an object value.
 */
struct json_schema_recursive {
	char const                     * jrec_key;
	struct json_schema const       * jrec_schema;   // recursive schema
	unsigned                         jrec_n;        // recursive schema size
};

/**
 * Used to check the existense of a key and read its value.
 */
struct json_schema_define {
	char const                     * jdef_key;
	enum json_schema_type            jdef_type;
	bool                             jdef_required;
	void                           * jdef_valptr;   // pointer to value
	unsigned                       * jdef_lenptr;   // array length
};

/**
 * Used to check for equality between a key value and an given value.
 */
struct json_schema_ifeq {
	char const                     * jequ_key;      // key
	char const                     * jequ_exp;      // expected value
	struct json_schema const       * jequ_schema;   // recursive schema
	unsigned                         jequ_n;        // recursive schema size
};

/**
 * The definition of a JSON schema.
 */
struct json_schema {
	enum json_schema_op              jscm_op;
	union {
	struct json_schema_recursive     jscm_recursive;
	struct json_schema_define        jscm_define;
	struct json_schema_ifeq          jscm_ifeq;
	};
};

/**
 * Require a key having an object value.
 */
#define JSON_REQUIRE_OBJ(key, p_obj) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_OBJ, \
		true, \
		GCC_TYPECHECK(struct json_object const **, p_obj), \
		NULL\
	} \
}

/**
 * Require a key having a text value.
 */
#define JSON_REQUIRE_TEXT(key, pp_char) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_TEXT, \
		true, \
		GCC_TYPECHECK(char const **, pp_char), \
		NULL\
	} \
}

/**
 * Require a key having an `int' value.
 */
#define JSON_REQUIRE_INT(key, p_int) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_INT, \
		true, \
		GCC_TYPECHECK(int *, p_int), \
		NULL \
	} \
}

/**
 * Define optional key with `int' value.
 */
#define JSON_OPTIONAL_INT(key, p_int) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_INT, \
		false, \
		GCC_TYPECHECK(int *, p_int), \
		NULL \
	} \
}

/**
 * Require a key having an `unsigned int' value.
 */
#define JSON_REQUIRE_UINT(key, p_uint) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_UINT, \
		true, \
		GCC_TYPECHECK(unsigned *, p_uint), \
		NULL \
	} \
}

/**
 * Define an optional key with `unsigned int' value.
 */
#define JSON_OPTIONAL_UINT(key, p_uint) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_UINT, \
		false, \
		GCC_TYPECHECK(unsigned *, p_uint), \
		NULL \
	} \
}

/**
 * Require a key having a `double' value.
 */
#define JSON_REQUIRE_DBL(key, p_double) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_DOUBLE, \
		true, \
		GCC_TYPECHECK(double *, p_double), \
		NULL \
	} \
}

/**
 * Require a key having a `char const * []' value.
 */
#define JSON_REQUIRE_TEXTV(key, ppp_char, p_uint) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_TEXTV, \
		true, \
		GCC_TYPECHECK(char const ***, ppp_char), \
		GCC_TYPECHECK(unsigned *,  p_uint), \
	} \
}

/**
 * Require a key having a `uint32_t[]' value.
 */
#define JSON_REQUIRE_U32V(key, pp_uint32, p_uint) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_UINT32V, \
		true, \
		GCC_TYPECHECK(uint32_t **, pp_uint32), \
		GCC_TYPECHECK(unsigned *,  p_uint), \
	} \
}

/**
 * Require a key having a `uint64_t[]' value.
 */
#define JSON_REQUIRE_U64V(key, pp_uint64, p_uint) \
{	.jscm_op = JSON_SCHEMA_OP_DEFINE, \
	.jscm_define = { \
		key, \
		JSON_SCHEMA_TYPE_UINT64V, \
		true, \
		GCC_TYPECHECK(uint64_t **, pp_uint64), \
		GCC_TYPECHECK(unsigned *,  p_uint), \
	} \
}

/**
 * Check whether key matches the given value. If so, read the given schema at
 * the current position.
 */
#define JSON_IFEQ(key, val, schema) \
{	.jscm_op = JSON_SCHEMA_OP_IFEQ, \
	.jscm_ifeq = { key, val, schema, GCC_DIM(schema) } \
}

/**
 * Recursively validate the object matching the given key.
 */
#define JSON_DESCEND(key, schema) \
{	.jscm_op = JSON_SCHEMA_OP_RECURSIVE, \
	.jscm_recursive = {  key, schema, GCC_DIM(schema) } \
}

/**
 * Validate JSON object.
 */
extern int
json_validate(
	struct json_object const * obj,
	struct json_schema const * schema,
	unsigned                   n,
	char                     * buf,
	size_t const               size
	);

#endif
