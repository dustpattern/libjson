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

#include <string.h>

#include "json.h"

static void test(
	char const * const test_name,
	char const * const test_doc
	)
{
	json_document_t * doc;
	int err;

	if ((err = json_parse_string(test_doc, &doc)))
		printf("%s: error: %s\n", test_name, strerror(err));
	else {
		printf("%s: ok\n", test_name);
		json_dump(doc, stdout);
		json_free(doc);
	}
}


int
main()
{
	/* invalid combinations */
	test("7a641d91", ""); // bad
	test("f55a9156", "{"); // bad
	test("3025fa52", "}"); // bad
	test("624d511e", "{x}"); // bad
	test("79a88562", "{x:}"); // bad
	test("b43ae774", "{x:1"); // bad
	test("b740a298", "{x:123"); // bad
	test("5580b330", "{x:1.23"); // bad
	test("3a5bb9bc", "{x:1."); // bad
	test("b9c3d98f", "{x:0."); // bad
	test("b92ff4c2", "{x:foo"); // bad

	/* misc spacing */
	test("ddcb27bc", "{x:1}");
	test("6864e671", "{x: 1}");
	test("b3702dd8", "{ x: 1 }");
	test("bf1ac9b7", "{x:1,y:2}");
	test("66afbfd8", "{x: 1, y:2}");
	test("fa458ac9", "{ x: 1, y :2 }");
	test("b10c6f54", "{foo:a,bar:b}");
	test("0d8babfa", "{foo:a,bar:[1]}");
	test("b648fb24", "{foo:a,bar:[1,2,3,4]}");
	test("a50d938b", "\n{\n\tfoo: a,\n\tbar: [\n\t\t1,\n\t\t2\n\t]\n}\n");
	test("8535e6e0", "{foo:a,bar:[1,{x:1,y:2},3,4]}");

	/* literal names */
	test("00942f06", "{ test: x }");
	test("41593cc5", "{ _test: x }");
	test("31457e98", "{ __test: x }");
	test("eb08bee4", "{ __test_1: x }");
	test("0dc05728", "{ abc0: x }");
	test("0d42f744", "{ 0abc: x }");  // bad
	test("f13f7b1d", "{ 0: x }");
	test("1ff27b41", "{ 123: x }");

	/* quotes */
	test("224e0215", "{ \"foo\": \"bar\" }");
	test("6c2bc37d", "{ \"hello world\": \"bar\" }");
	test("0c348644", "{ \"hello world\": \"foo bar\" }");
	test("a8013767", "{ \"hello world\": \"foo \\\"bar\" }");
	test("2f40cb81", "{ \"hello world\": \"foo\nbar\" }"); // bad
	test("64b2f1cb", "{ \"hello world\": \"foo"); // bad
	test("379df015", "{ \"hello world\": \""); // bad

	/* large tokens */
	test("83cb7be2", "{ foo: \"012345678901234567890123456789012345678901234567890123456789012\" }");
	test("aa922bf2", "{ foo: a12345678901234567890123456789012345678901234567890123456789012 }");
	test("a54f32db", "{ foo: \"0123456789012345678901234567890123456789012345678901234567890123\" }"); // bad
	test("35c8fc21", "{ foo: a123456789012345678901234567890123456789012345678901234567890123 }"); // bad

	/* numbers */
	test("3aaf8a94", "{ x: 0 }");
	test("af00c05d", "{ x: 01 }"); // bad
	test("d3e1d6c3", "{ x: 1 }");
	test("7a7a348b", "{ x: 12 }");
	test("da422f25", "{ x: 0.1 }");
	test("ea7aed17", "{ x: 0.123 }");
	test("335b0d45", "{ x: 0. }"); // bad
	test("f5d3f5ee", "{ x: 0.123b }"); // bad
	test("abf03b26", "{ x: 0.1b }"); // bad
	test("8756c16f", "{ x: 0.b }"); // bad
	test("f7ff22cb", "{ x: 0b }"); // bad
	test("8febf389", "{ x: 01.1 }"); // bad
	test("45ec065f", "{ x: 00.1 }"); // bad
	test("da422f25", "{ x: 1.1 }");
	test("ea7aed17", "{ x: 1.123 }");
	test("335b0d45", "{ x: 1. }"); // bad
	test("f5d3f5ee", "{ x: 1.123b }"); // bad
	test("abf03b26", "{ x: 1.1b }"); // bad
	test("8756c16f", "{ x: 1.b }"); // bad
	test("f7ff22cb", "{ x: 1b }"); // bad
	test("d123475a", "{ x: 12.1 }");
	test("f36010cc", "{ x: 12. }"); // bad

	test("0c350fb9", "{ }");
	test("d76b246d", "{ v: [] }");
	test("f208cb30", "{ v: [], {} }"); // bad
	test("6eeef86f", "{ x: [], y: {} }");
	test("321fe338", "{ a: [], b: { c: [] }, d: [ [], [], [] ] }");
	test("76c526a0", "{ x: [ {}, {}, {}] }");
	test("76c526a0", "{ x: [ {}, [], {}, [], {} ] }");

	return 0;
}
