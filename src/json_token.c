/*
 * json_token.c
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

/* Return an error token */
#define RETURN_TOKEN_ERROR(tok, err) \
do { \
	*(tok) = (struct json_token) { JSON_TOK_ERR, NULL }; \
	return (err); \
} while(0)

/* Return a punctuation token */
#define RETURN_TOKEN(tok, id) \
do { \
	*(tok) = (struct json_token) { (id), NULL }; \
	return 0; \
} while(0)

/* Return a literal token */
#define RETURN_TOKEN_LIT(tok, lit) \
do { \
	*(tok) = (struct json_token) { JSON_TOK_LIT, (lit) }; \
	return 0; \
} while(0)

/* Store character in buffer */
#define WRITECHAR(s, e, c) \
({ \
	bool const _ok = s < e; \
	if (_ok) *s++ = c; \
	_ok; \
})

/* Check if character belongs to unquoted literal */
static inline bool
_is_literal_char(char const c)
{
	return isascii(c) && (isalpha(c) || isdigit(c) || c == '_');
}

/* Check if character belongs to quoted literal */
static inline bool
_is_literal_string_char(char const c)
{
	return isascii(c) && isprint(c);
}

/**
 * Consume unquoted literal token.
 */
static inline int
_consume_literal(struct json_doc * const doc, char c,
		 struct json_token * const tok )
{
	FILE * const f = doc->jdoc_f;
	char   buf[64];
	char * s = buf;
	char * e = buf + sizeof(buf);
	char * lit;
	int    err;

	/* consume first character */
	(void) WRITECHAR(s, e, c);

	/* consume remaining characters */
	while ((c = getc(f)) != EOF
	       && _is_literal_char(c)
	       && WRITECHAR(s, e, c))
		continue;

	/* the terminating character */
	if (!WRITECHAR(s, e, '\0'))
		RETURN_TOKEN_ERROR(tok, EINVAL);

	/* allocate string */
	if ((err = _gcmalloc(doc, s - buf, &lit)))
		RETURN_TOKEN_ERROR(tok, err);
	memcpy(lit, buf, s - buf);

	doc->jdoc_nextc = c;
	doc->jdoc_nextc_avail = true;
	RETURN_TOKEN_LIT(tok, lit);
}

/**
 * Consume quoted literal token.
 */
static inline int
_consume_literal_string(struct json_doc * const doc, char c,
			struct json_token * const tok )
{
	FILE * const f = doc->jdoc_f;
	char   buf[64];
	char * s = buf;
	char * e = buf + sizeof(buf);
	char * lit;
	int    err;

	/* consume characters in string */
	while ((c = getc(f)) != EOF && c != '"') {
		// FIXME We are not parsing \x codes correctly.
		if (c == '\\' && (c = getc(f)) == EOF)
			break;
		if (!_is_literal_string_char(c))
			RETURN_TOKEN_ERROR(tok, EINVAL);
		if (!WRITECHAR(s, e, c))
			RETURN_TOKEN_ERROR(tok, EINVAL);
	}
	/* unterminated string */
	if (c == EOF)
		RETURN_TOKEN_ERROR(tok, feof(f) ? EINVAL : EIO);

	/* the terminating character */
	if (!WRITECHAR(s, e, '\0'))
		RETURN_TOKEN_ERROR(tok, EINVAL);

	/* allocate string */
	if ((err = _gcmalloc(doc, s - buf, &lit)))
		RETURN_TOKEN_ERROR(tok, err);
	memcpy(lit, buf, s - buf);

	RETURN_TOKEN_LIT(tok, lit);
}

/**
 * Consume numeric literal token.
 */
static inline int
_consume_literal_number(
	struct json_doc * const doc, char c,
	struct json_token * const tok )
{
	FILE * const f = doc->jdoc_f;
	char   buf[64];
	char * s = buf;
	char * e = buf + sizeof(buf);
	char * lit;
	int    err;

	/* States */
	enum { ST, IN, ZR, DO, FR, XX };

	/* State transitions */
	static int const st[5][11]  = {
	/* State    Input ->  '0' '1' '2' '3' '4' '5' '6' '7' '8' '9' '.' */
	/* -----              ------------------------------------------  */
	/* START */ [ST] = {  ZR, IN, IN, IN, IN, IN, IN, IN, IN, IN, XX  },
	/*   INT */ [IN] = {  IN, IN, IN, IN, IN, IN, IN, IN, IN, IN, DO  },
	/*  ZERO */ [ZR] = {  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, DO  },
	/*   DOT */ [DO] = {  FR, FR, FR, FR, FR, FR, FR, FR, FR, FR, XX  },
	/*  FRAC */ [FR] = {  FR, FR, FR, FR, FR, FR, FR, FR, FR, FR, XX  },
	};

	/* consume characters using the state machine */
	int y = ST;
	do {
		/* next state */
		if (isascii(c) && isdigit(c))
			y = st[y][c - '0'];
		else if (c == '.')
			y = st[y][10];
		else
			break;
		/* error state */
		if (y == XX)
			RETURN_TOKEN_ERROR(tok, EINVAL);
		/* write character to buffer */
		if (!WRITECHAR(s, e, c))
			RETURN_TOKEN_ERROR(tok, EINVAL);

	} while ((c = getc(f)) != EOF);

	/* state machine must be in a valid end state */
	if (y != IN && y != ZR && y != FR)
		RETURN_TOKEN_ERROR(tok, EINVAL);
	/* literal must be terminated by a space or punctuation character */
	if (! (c == EOF || (isascii(c) && (isspace(c) || ispunct(c)))) )
		RETURN_TOKEN_ERROR(tok, EINVAL);

	/* finish literal value */
	if (!WRITECHAR(s, e, '\0'))
		RETURN_TOKEN_ERROR(tok, EINVAL);

	/* duplicate literal value */
	if ((err = _gcmalloc(doc, s - buf, &lit)))
		RETURN_TOKEN_ERROR(tok, err);
	memcpy(lit, buf, s - buf);

	/* put back last character into stream */
	doc->jdoc_nextc = c;
	doc->jdoc_nextc_avail = true;
	RETURN_TOKEN_LIT(tok, lit);
}

/**
 * Consume a token.
 */
int
json_consume_token(struct json_doc * const doc, struct json_token * const tok)
{
	FILE * const f = doc->jdoc_f;
	int c;

	/* use lookahead token if available */
	if (doc->jdoc_lookahead_avail) {
		doc->jdoc_lookahead_avail = false;
		*tok = doc->jdoc_lookahead;
		return 0;
	}

	/* ignore whitespace */
	c = doc->jdoc_nextc_avail ? doc->jdoc_nextc : getc(f);
	doc->jdoc_nextc_avail = false;
	for (; isascii(c) && isspace(c); c = getc(f))
		if (c == '\n')
			doc->jdoc_lineno++;
	if (c == EOF && feof(f))
		RETURN_TOKEN(tok, JSON_TOK_EOF);
	if (c == EOF && ferror(f))
		RETURN_TOKEN_ERROR(tok, EIO);
	if (!isascii(c))
		RETURN_TOKEN_ERROR(tok, EINVAL);

	/* token */
	switch (c) {
	case '{': RETURN_TOKEN(tok, JSON_TOK_OBJECT_BEGIN);
	case '}': RETURN_TOKEN(tok, JSON_TOK_OBJECT_END);
	case ':': RETURN_TOKEN(tok, JSON_TOK_COLON);
	case '[': RETURN_TOKEN(tok, JSON_TOK_ARRAY_BEGIN);
	case ']': RETURN_TOKEN(tok, JSON_TOK_ARRAY_END);
	case ',': RETURN_TOKEN(tok, JSON_TOK_COMMA);
	case 'a' ... 'z':
	case 'A' ... 'Z':
	case '_':
		return _consume_literal(doc, c, tok);
	case '"':
		return _consume_literal_string(doc, c, tok);
	case '0' ... '9':
		return _consume_literal_number(doc, c, tok);
	default:
		RETURN_TOKEN_ERROR(tok, EINVAL);
	}
}

/**
 * Look at the next token without consuming it.
 */
int
json_peek_token(struct json_doc * const doc, struct json_token * const tok)
{
	int err;

	if ((err = json_consume_token(doc, tok)))
		return err;

	doc->jdoc_lookahead_avail = true;
	doc->jdoc_lookahead = *tok;
	return 0;
}
