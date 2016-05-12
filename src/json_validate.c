/*
 * json_validate.c
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

static int
_recursive(
	struct json_object const * const obj,
	struct json_schema const * const it,
	char                     * const buf,
	size_t                     const size
	)
{
	struct json_schema_recursive const * const rec = &it->jscm_recursive;
	if (rec->jrec_schema == NULL)
		goto fail_schema;

	/* find object of recursion */
	char const * const key = rec->jrec_key;
	struct json_value const * const jval = json_get_value(obj, key);
	if (jval == NULL)
		goto fail_missing;
	if (jval->jval_type != JSON_VAL_OBJECT)
		goto fail_not_object;

	/* take note of our new location in the error buffer */
	size_t const len = snprintf(buf, size, "in `%s': ", key);
	size_t const subsize = len < size ? size - len : 0;
	char * const subbuf = buf + len;

	/* recursively validate object */
	unsigned const subn = rec->jrec_n;
	struct json_schema const * const subschema = rec->jrec_schema;
	struct json_object const * const subobj = jval->jval_object;
	return json_validate(subobj, subschema, subn, subbuf, subsize);

fail_not_object:
	snprintf(buf, size, "expected OBJECT value for key `%s'", key);
	return EINVAL;
fail_missing:
	snprintf(buf, size, "missing required object `%s'", key);
	return EINVAL;
fail_schema:
	snprintf(buf, size, "invalid schema definition");
	return ENOTSUP;
}

static int
_ifeq(
	struct json_object const * const obj,
	struct json_schema const * const it,
	char                     * const buf,
	size_t                     const size
	)
{
	struct json_schema_ifeq const * const equ = &it->jscm_ifeq;
	char const * const key = equ->jequ_key;
	char const * const exp = equ->jequ_exp;

	if (equ->jequ_schema == NULL) {
		snprintf(buf, size, "invalid schema definition");
		return ENOTSUP;
	}

	char const * const jval = json_get_literal(obj, key);
	return jval && !strcasecmp(jval, exp)
	       ? json_validate(obj, equ->jequ_schema, equ->jequ_n, buf, size)
	       : 0;
}

static int
_define(
	struct json_object const * const obj,
	struct json_schema const * const it,
	char                     * const buf,
	size_t                     const size
	)
{
	struct json_schema_define const * const def = &it->jscm_define;
	char const * const key    = def->jdef_key;
	void       * const valptr = def->jdef_valptr;
	unsigned   * const lenptr = def->jdef_lenptr;

	/* determine whether key exists */
	struct json_value const * const jval = json_get_value(obj, key);
	if (jval == NULL) {
		if (def->jdef_required)
			goto fail_missing;
		else
			return 0;
	}

	/* check key value */
	switch (def->jdef_type) {

	case JSON_SCHEMA_TYPE_OBJ:
		if (jval->jval_type != JSON_VAL_OBJECT)
			goto fail_not_object;
		if (!valptr || lenptr)
			goto fail_schema;
		*((struct json_object const **) valptr) = jval->jval_object;
		return 0;

	case JSON_SCHEMA_TYPE_TEXT:
		if (jval->jval_type != JSON_VAL_LITERAL)
			goto fail_not_string;
		if (!valptr || lenptr)
			goto fail_schema;
		*((char const **) valptr) = jval->jval_lit;
		return 0;

	case JSON_SCHEMA_TYPE_TEXTV: {
		if (jval->jval_type != JSON_VAL_ARRAY)
			goto fail_not_text_array;
		if (!valptr || !lenptr)
			goto fail_schema;
		struct json_array const * const jarr = jval->jval_array;
		switch (json_get_array_of_text(jarr, valptr, lenptr)) {
		case 0:
			return 0;
		case ENOMEM:
			goto fail_memory;
		default:
			goto fail_not_text_array;
		}
	}

	case JSON_SCHEMA_TYPE_INT:
		if (jval->jval_type != JSON_VAL_LITERAL)
			goto fail_not_int;
		if (!valptr || lenptr)
			goto fail_schema;
		if (str2int(jval->jval_lit, def->jdef_valptr))
			goto fail_not_int;
		return 0;

	case JSON_SCHEMA_TYPE_UINT:
		if (jval->jval_type != JSON_VAL_LITERAL)
			goto fail_not_uint;
		if (!valptr || lenptr)
			goto fail_schema;
		if (str2uint(jval->jval_lit, def->jdef_valptr))
			goto fail_not_uint;
		return 0;

	case JSON_SCHEMA_TYPE_DOUBLE:
		if (jval->jval_type != JSON_VAL_LITERAL)
			goto fail_not_double;
		if (!valptr || lenptr)
			goto fail_schema;
		if (str2double(jval->jval_lit, def->jdef_valptr))
			goto fail_not_double;
		return 0;

	case JSON_SCHEMA_TYPE_UINT32V: {
		if (jval->jval_type != JSON_VAL_ARRAY)
			goto fail_not_uint32_array;
		if (!valptr || !lenptr)
			goto fail_schema;
		struct json_array const * const jarr = jval->jval_array;
		switch (json_get_array_of_uint32(jarr, valptr, lenptr)) {
		case 0:
			return 0;
		case ENOMEM:
			goto fail_memory;
		default:
			goto fail_not_uint32_array;
		}
	}

	case JSON_SCHEMA_TYPE_UINT64V: {
		if (jval->jval_type != JSON_VAL_ARRAY)
			goto fail_not_uint64_array;
		if (!valptr || !lenptr)
			goto fail_schema;
		struct json_array const * const jarr = jval->jval_array;
		switch (json_get_array_of_uint64(jarr, valptr, lenptr)) {
		case 0:
			return 0;
		case ENOMEM:
			goto fail_memory;
		default:
			goto fail_not_uint64_array;
		}
	}

	default:
		goto fail_schema;
	}

fail_not_text_array:
	snprintf(buf, size, "expected TEXT array value for key `%s'", key);
	return EINVAL;
fail_not_uint64_array:
	snprintf(buf, size, "expected UINT64 array value for key `%s'", key);
	return EINVAL;
fail_not_uint32_array:
	snprintf(buf, size, "expected UINT32 array value for key `%s'", key);
	return EINVAL;
fail_not_double:
	snprintf(buf, size, "expected DOUBLE value for key `%s'", key);
	return EINVAL;
fail_not_int:
	snprintf(buf, size, "expected INT integer value for key `%s'", key);
	return EINVAL;
fail_not_uint:
	snprintf(buf, size, "expected UINT integer value for key `%s'", key);
	return EINVAL;
fail_not_string:
	snprintf(buf, size, "expected TEXT value for key `%s'", key);
	return EINVAL;
fail_not_object:
	snprintf(buf, size, "expected OBJECT value for key `%s'", key);
	return EINVAL;
fail_memory:
	snprintf(buf, size, "cannot allocate memory for key `%s'", key);
	return ENOMEM;
fail_missing:
	snprintf(buf, size, "missing required key `%s'", key);
	return EINVAL;
fail_schema:
	snprintf(buf, size, "invalid schema definition");
	return ENOTSUP;
}

int
json_validate(
	struct json_object const * const obj,
	struct json_schema const * const schema,
	unsigned                   const n,
	char                     * const buf,
	size_t                     const size
	)
{
	int err = 0;
#ifdef SABOTAGE
	snprintf(buf, size, "sabotaged");
#endif
	struct json_schema const * it = schema;
	for (unsigned i = 0; i < n && !err; i++, it++) {
		switch (it->jscm_op) {
		case JSON_SCHEMA_OP_RECURSIVE:
			err = _recursive(obj, it, buf, size);
			break;
		case JSON_SCHEMA_OP_DEFINE:
			err = _define(obj, it, buf, size);
			break;
		case JSON_SCHEMA_OP_IFEQ:
			err = _ifeq(obj, it, buf, size);
			break;
		default:
			snprintf(buf, size, "invalid schema definition");
			err = ENOTSUP;
		}
	}

	return err;
}
