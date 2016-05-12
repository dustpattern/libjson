/*
 * json_debug.c
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
static void _dump_object (unsigned, struct json_object const *, FILE *);
static void _dump_value  (unsigned, struct json_value  const *, FILE *);
static void _dump_array  (unsigned, struct json_array  const *, FILE *);

static inline void
_indent(unsigned const n, FILE * const f)
{
	for (unsigned i = 0; i < n; i++)
		fputs("    ", f);
}

static void
_dump_value(unsigned const lev,
	    struct json_value const * const val,
	    FILE * const f)
{
	switch (val->jval_type) {
	case JSON_VAL_LITERAL:
		fprintf(f, "\"%s\"", val->jval_lit);
		break;
	case JSON_VAL_OBJECT:
		_dump_object(lev, val->jval_object, f);
		break;
	case JSON_VAL_ARRAY:
		_dump_array(lev, val->jval_array, f);
		break;
	default:
		fputs("########", f);
	}
}

static void
_dump_array(unsigned const lev,
	    struct json_array const * const array,
	    FILE * const f)
{
	fputs("[\n", f);
	unsigned const len = array->jarr_length;
	for (unsigned i = 0; i < len; i++) {
		_indent(lev + 1, f);
		_dump_value(lev + 1, &array->jarr_values[i], f);
		if (i + 1 < len)
			fputs(",\n", f);
		else
			fputs("\n", f);
	}
	_indent(lev, f);
	fputs("]", f);
}

static void
_dump_object(unsigned const lev,
	     struct json_object const * const obj,
	     FILE * const f)
{
	fputs("{\n", f);

	unsigned const len = obj->jobj_length;
	for (unsigned i = 0; i < len; i++) {
		struct json_tuple const * const tup = &obj->jobj_tuples[i];
		_indent(lev + 1, f);
		fprintf(f, "\"%s\": ", tup->jtup_key);
		_dump_value(lev + 1, &tup->jtup_val, f);
		if (i + 1 < len)
			fputs(",\n", f);
		else
			fputs("\n", f);
	}

	_indent(lev, f);
	fputs("}", f);
}

void
json_dump(struct json_doc const * const doc, FILE * const f)
{
	_dump_object(0, doc->jdoc_obj, f);
	putc('\n', f);
}
