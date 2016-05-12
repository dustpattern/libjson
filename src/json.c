/*
 * json.c
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

// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
//                               Document                                   //
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

struct json_object const *
json_doc_object(json_document_t const * const doc)
{
	return doc->jdoc_obj;
}

// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //
//                              Discovery                                   //
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: //

struct json_value const *
json_get_value(
	struct json_object const * const obj,
	char const * const path )
{
	char const * const e = strchr(path, '/') ? : path + strlen(path);
	unsigned const len = e - path;

	if (len == 0)
		return NULL;

	for (unsigned i = 0; i < obj->jobj_length; i++) {
		char const * const key =
			obj->jobj_tuples[i].jtup_key;
		struct json_value const * const val =
			&obj->jobj_tuples[i].jtup_val;
		if (strlen(key) != len || strncasecmp(key, path, len))
			continue;
		/* continue along the path */
		if (*e && val->jval_type == JSON_VAL_OBJECT)
			return json_get_value(val->jval_object, e + 1);
		else if (*e == '\0')
			return val;   // final destination
		else
			return NULL;  // can go no farther
	}

	return NULL;
}

char const *
json_get_literal(
	struct json_object const * const obj,
	char const * const path)
{
	struct json_value const * const val = json_get_value(obj, path);
	return val && val->jval_type == JSON_VAL_LITERAL
		? val->jval_lit : NULL;
}

struct json_object const *
json_get_object(
	struct json_object const * const obj,
	char const * const path )
{
	struct json_value const * const val = json_get_value(obj, path);
	return val && val->jval_type == JSON_VAL_OBJECT
		? val->jval_object : NULL;
}

struct json_array const *
json_get_array(
	struct json_object const * const obj,
	char const * const path)
{
	struct json_value const * const val = json_get_value(obj, path);
	return val && val->jval_type == JSON_VAL_ARRAY
		? val->jval_array : NULL;
}

int
json_get_double(
	struct json_object const * const obj,
	char const * const path,
	double * const val
	)
{
	char const * const lit = json_get_literal(obj, path);
	return lit ? str2double(lit, val) : ENOENT;
}

int
json_get_int(
	struct json_object const * const obj,
	char const * const path,
	int * const val
	)
{
	char const * const lit = json_get_literal(obj, path);
	return lit ? str2int(lit, val) : ENOENT;
}

int
json_get_uint(
	struct json_object const * const obj,
	char const * const path,
	unsigned * const val
	)
{
	char const * const lit = json_get_literal(obj, path);
	return lit ? str2uint(lit, val) : ENOENT;
}

int
json_get_array_of_uint32(
	struct json_array const * const jarr,
	uint32_t ** const outvec,
	unsigned * const outlen
	)
{
	int err;

	/* allocate a vector big enough to accomodate all values */
	unsigned const len = jarr->jarr_length;
	size_t const size = sizeof(uint32_t) * len;
	uint32_t * const vec = malloc(size);
	if (vec == NULL) {
		err = errno;
		goto fail;
	}

	/* parse values */
	for (unsigned i = 0; i < len; i++) {
		struct json_value const * const jval = &jarr->jarr_values[i];
		if (   jval->jval_type != JSON_VAL_LITERAL
		    || str2uint32(jval->jval_lit, &vec[i])) {
			err = EINVAL;
			goto fail_1;
		}
	}

	*outvec = vec;
	*outlen = len;
	return 0;

fail_1:	free(vec);
fail:	*outvec = NULL;
	*outlen = 0;
	return err;
}

int
json_get_array_of_uint64(
	struct json_array const  * const jarr,
	uint64_t ** const outvec,
	unsigned * const outlen
	)
{
	int err;

	/* allocate a vector big enough to accomodate the values */
	unsigned const len = jarr->jarr_length;
	size_t const size = sizeof(uint64_t) * len;
	uint64_t * const vec = malloc(size);
	if (vec == NULL) {
		err = errno;
		goto fail;
	}

	/* parse values */
	for (unsigned i = 0; i < len; i++) {
		struct json_value const * const jval = &jarr->jarr_values[i];
		if (   jval->jval_type != JSON_VAL_LITERAL
		    || str2uint64(jval->jval_lit, &vec[i])) {
			err = EINVAL;
			goto fail_1;
		}
	}

	*outvec = vec;
	*outlen = len;
	return 0;

fail_1:	free(vec);
fail:	*outvec = NULL;
	*outlen = 0;
	return err;
}

int
json_get_array_of_text(
	struct json_array const  * const jarr,
	char const *** const outvec,
	unsigned * const outlen
	)
{
	int err;

	/* allocate a vector big enough to accomodate the values */
	unsigned const len = jarr->jarr_length;
	size_t const size = sizeof(char const *) * len;
	char const ** const vec = malloc(size);
	if (vec == NULL) {
		err = errno;
		goto fail;
	}

	/* parse values */
	for (unsigned i = 0; i < len; i++) {
		struct json_value const * const jval = &jarr->jarr_values[i];
		if (jval->jval_type != JSON_VAL_LITERAL) {
			err = EINVAL;
			goto fail_1;
		}
		vec[i] = jval->jval_lit;
	}

	*outvec = vec;
	*outlen = len;
	return 0;

fail_1:	free(vec);
fail:	*outvec = NULL;
	*outlen = 0;
	return err;
}
