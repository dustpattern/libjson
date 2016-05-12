/*
 * json_conv.h
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

#ifndef __LIBJSON_CONV_H__
#define __LIBJSON_CONV_H__

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

static inline int
str2int64(char const * const lit, int64_t * const val)
{
	char * endptr;
	long long ll;

	if (!*lit)
		goto fail;
	errno = 0;
	ll = strtoll(lit, &endptr, 10);
	if (*endptr || errno)
		goto fail;

	*val = ll;
	return 0;

fail:	*val = 0;
	return EINVAL;
}

static inline int
str2uint64(char const * const lit, uint64_t * const val)
{
	char * endptr;
	unsigned long long ull;

	if (!*lit)
		return EINVAL;
	errno = 0;
	ull = strtoull(lit, &endptr, 10);
	if (*endptr || errno)
		return EINVAL;

	*val = ull;
	return 0;
}

static inline int
str2uint32(char const * const s, uint32_t * const val)
{
	uint64_t u;
	int err;

	if ((err = str2uint64(s, &u)))
		return err;
	if (u > UINT32_MAX)
		return EINVAL;

	*val = u;
	return 0;
}

static inline int
str2int(char const * const lit, int * const val)
{
	int64_t i;
	int err;

	if ((err = str2int64(lit, &i)))
		return err;
	if (i < INT_MIN || i > INT_MAX)
		return ERANGE;

	*val = i;
	return 0;
}

static inline int
str2uint(char const * const lit, unsigned * const val)
{
	int64_t i;
	int err;

	if ((err = str2int64(lit, &i)))
		return err;
	if (i < 0 || i > UINT_MAX)
		return ERANGE;

	*val = i;
	return 0;
}

static inline int
str2double(char const * const lit, double * const val)
{
	char * endptr;
	double d;

	if (!*lit)
		return EINVAL;
	errno = 0;
	d = strtod(lit, &endptr);
	if (*endptr || errno)
		return EINVAL;

	*val = d;
	return 0;
}

#endif
