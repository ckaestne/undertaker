#!/usr/bin/env python
#
# busyfix test - tests whether busyfix works well
#
# Copyright (C) 2012 Manuel Zerpies <manuel.f.zerpies@ww.stud.uni-erlangen.de>
# Copyright (C) 2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import logging
import unittest2

from vamos.busyfix.normalizer import normalize_line as nl
from vamos.tools import setup_logging

class BasicTests(unittest2.TestCase):

    def test_ignore_empty_lines(self):
        i = ""
        o = ""
        self.equal_test(i, o)


    def test_ignore_empty_lines_with_newline(self):
        i = """
"""
        o = """
"""
        self.equal_test(i, o)


    def test_noops(self):
        for l in ['#if 0', '#else', '#endif',
                  '#define CONFIG_FOO 1',
                  '#define CONFIG_BAR 0'
                  '       a;', # do not eat leading spaces
                  'code(); //usage #if IF_FEATURE_ACPID_COMPACT('
                  ]:
            self.equal_test(nl(l), l)


    def test_defined_normalizations(self):
        self.assertEqual(nl("#if ENABLE_FEATURE_USB"),
                         "#if defined CONFIG_FEATURE_USB")
        self.assertEqual(nl("#if ENABLE_HUSH_IF || ENABLE_HUSH_LOOPS"),
                         "#if defined CONFIG_HUSH_IF || defined CONFIG_HUSH_LOOPS")

    def test_ENABLE_inline(self):
        i=nl("if (ENABLE_FEATURE_USB) {")
        o="""if (
#if defined CONFIG_FEATURE_USB
1
#else
0
#endif
) {"""
        self.equal_test(i, o)


    def test_ENABLE_inline2(self):
        i=nl("if (ENABLE_FEATURE_USB)")
        o="""if (
#if defined CONFIG_FEATURE_USB
1
#else
0
#endif
)"""
        self.equal_test(i, o)


    def test_ENABLE_inline3(self):
        i=nl("	char day_headings[ENABLE_UNICODE_SUPPORT ? 28 * 6 : 28];")
        o="""\tchar day_headings[
#if defined CONFIG_UNICODE_SUPPORT
1
#else
0
#endif
 ? 28 * 6 : 28];"""
        self.equal_test(i, o)


    def test_ENABLE_in_multiplication(self):
        i = """enum {
	OPT_TIMESPEC  = (1 << 5) * ENABLE_FEATURE_DATE_ISOFMT, /* I */
	OPT_HINT      = (1 << 6) * ENABLE_FEATURE_DATE_ISOFMT, /* D */
};"""
        # trailing whitespace intended here
        o = """enum {
	OPT_TIMESPEC  = (1 << 5) * 
#if defined CONFIG_FEATURE_DATE_ISOFMT
1
#else
0
#endif
, /* I */
	OPT_HINT      = (1 << 6) * 
#if defined CONFIG_FEATURE_DATE_ISOFMT
1
#else
0
#endif
, /* D */
};"""
        self.equal_test(i, o)


    @unittest2.expectedFailure
    def test_line_continuation_advanced(self):
        i = """
#if ENABLE_FEATURE_VI_YANKMARK \\
 || (ENABLE_FEATURE_VI_COLON && ENABLE_FEATURE_VI_SEARCH) \\
 || ENABLE_FEATURE_VI_CRASHME
// might reallocate text[]! use p += string_insert(p, ...),
// and be careful to not use pointers into potentially freed text[]!
"""
        o = """
#if ENABLE_FEATURE_VI_YANKMARK || (ENABLE_FEATURE_VI_COLON && ENABLE_FEATURE_VI_SEARCH) || ENABLE_FEATURE_VI_CRASHME
// might reallocate text[]! use p += string_insert(p, ...),
// and be careful to not use pointers into potentially freed text[]!
"""
        self.equal_test(i, o)


    def test_if_inline(self):
        i=nl("IF_FEATURE_IPV6(sa_family_t af = AF_INET)")
        o="""
#if defined CONFIG_FEATURE_IPV6
sa_family_t af = AF_INET
#endif"""
        self.equal_test(i, o)


    def test_if_inline2(self):
        i=nl("IF_FEATURE_FIND_PATH(ACTS(path,  const char *pattern; bool ipath;))")
        o="""
#if defined CONFIG_FEATURE_FIND_PATH
ACTS(path,  const char *pattern; bool ipath;)
#endif"""
        self.equal_test(i, o)

    def test_notif_inline(self):
        i=nl("IF_NOT_FEATURE_IPV6(sa_family_t af = AF_INET)")
        o="""
#if !defined CONFIG_FEATURE_IPV6
sa_family_t af = AF_INET
#endif"""
        self.equal_test(i, o)


    def test_IF_FEATURE_inline(self):
        i = nl("""IF_DESKTOP(long long) int bz_write(bz_stream *strm, void* rbuf, ssize_t rlen, void *wbuf) {""")
        o = """
#if defined CONFIG_DESKTOP
long long
#endif
 int bz_write(bz_stream *strm, void* rbuf, ssize_t rlen, void *wbuf) {"""

        self.equal_test(i, o)


    def test_not_ENABLE_line_continuation(self):
        i = nl("""#if !ENABLE_DESKTOP \\""")
        o = nl("""#if !ENABLE_DESKTOP """)

        self.equal_test(i, o)


    def test_IF_multiline(self):
        i = """IF_FEATURE_MOUNT_LOOP(
    /* "loop" */ 0,
)"""
        o = """
#if defined CONFIG_FEATURE_MOUNT_LOOP
    /* "loop" */ 0,
#endif"""
        self.equal_test(i, o)


    def test_multiIF_multiline(self):
        i = """IF_FEATURE_MOUNT_LOOP(
    IF_FEATURE_USB_ON("USB is on",)
    IF_FEATURE_USB_OFF("USB is off",)
    /* "loop" */ 0,
)"""

        o = """
#if defined CONFIG_FEATURE_MOUNT_LOOP

#if defined CONFIG_FEATURE_USB_ON
"USB is on",
#endif

#if defined CONFIG_FEATURE_USB_OFF
"USB is off",
#endif
    /* "loop" */ 0,
#endif"""
        self.equal_test(i, o)


    def test_ignore_applet_macros(self):
        i = """//applet:IF_DATE(APPLET(date, BB_DIR_BIN, BB_SUID_DROP))"""
        o = """//applet:IF_DATE(APPLET(date, BB_DIR_BIN, BB_SUID_DROP))"""
        self.equal_test(i, o)


    def test_comples_macros(self):
        i = """
#if (ENABLE_FEATURE_SEAMLESS_GZ || ENABLE_FEATURE_SEAMLESS_BZ2)
# if !(ENABLE_FEATURE_SEAMLESS_GZ && ENABLE_FEATURE_SEAMLESS_BZ2)
#  define vfork_compressor(tar_fd, gzip) vfork_compressor(tar_fd)
# endif
/* Don't inline: vfork scares gcc and pessimizes code */
static void NOINLINE vfork_compressor(int tar_fd, int gzip)
#endif
"""
        o = """
#if (defined CONFIG_FEATURE_SEAMLESS_GZ || defined CONFIG_FEATURE_SEAMLESS_BZ2)
# if !(defined CONFIG_FEATURE_SEAMLESS_GZ && defined CONFIG_FEATURE_SEAMLESS_BZ2)
#  define vfork_compressor(tar_fd, gzip) vfork_compressor(tar_fd)
# endif
/* Don't inline: vfork scares gcc and pessimizes code */
static void NOINLINE vfork_compressor(int tar_fd, int gzip)
#endif
"""
        self.equal_test(i, o)


    def test_defined_ENABLE_macro(self):
        i = """#if defined ENABLE_PARSE && ENABLE_PARSE"""
        o = """#if defined CONFIG_PARSE && ENABLE_PARSE"""

        self.equal_test(i, o)


    def test_line_continuation_define(self):
        i = """
#define ZIPPED (ENABLE_FEATURE_SEAMLESS_LZMA \\
       || ENABLE_FEATURE_SEAMLESS_BZ2 \\
       || ENABLE_FEATURE_SEAMLESS_GZ \\
       /* || ENABLE_FEATURE_SEAMLESS_Z */ \\
"""
        o = """
#define ZIPPED (ENABLE_FEATURE_SEAMLESS_LZMA        || ENABLE_FEATURE_SEAMLESS_BZ2        || ENABLE_FEATURE_SEAMLESS_GZ        /* || ENABLE_FEATURE_SEAMLESS_Z */ """

        self.equal_test(i, o)


    def test_elif(self):
        i = """# if ENABLE_FEATURE_SEAMLESS_GZ && ENABLE_FEATURE_SEAMLESS_BZ2
	const char *zip_exec = (gzip == 1) ? "gzip" : "bzip2";
# elif ENABLE_FEATURE_SEAMLESS_GZ
	const char *zip_exec = "gzip";
# else /* only ENABLE_FEATURE_SEAMLESS_BZ2 */
	const char *zip_exec = "bzip2";
# endif"""

        o = """# if defined CONFIG_FEATURE_SEAMLESS_GZ && defined CONFIG_FEATURE_SEAMLESS_BZ2
	const char *zip_exec = (gzip == 1) ? "gzip" : "bzip2";
# elif defined CONFIG_FEATURE_SEAMLESS_GZ
	const char *zip_exec = "gzip";
# else /* only ENABLE_FEATURE_SEAMLESS_BZ2 */
	const char *zip_exec = "bzip2";
# endif"""

        self.equal_test(i, o)


    def test_line_continuation_with_IF(self):
        i = """
#define FIX_ENDIANNESS_CDF(cdf_header) do { \\
	(cdf_header).formatted.crc32        = SWAP_LE32((cdf_header).formatted.crc32       ); \\
	(cdf_header).formatted.cmpsize      = SWAP_LE32((cdf_header).formatted.cmpsize     ); \\
	(cdf_header).formatted.ucmpsize     = SWAP_LE32((cdf_header).formatted.ucmpsize    ); \\
	(cdf_header).formatted.file_name_length = SWAP_LE16((cdf_header).formatted.file_name_length); \\
	(cdf_header).formatted.extra_field_length = SWAP_LE16((cdf_header).formatted.extra_field_length); \\
	(cdf_header).formatted.file_comment_length = SWAP_LE16((cdf_header).formatted.file_comment_length); \\
	IF_DESKTOP( \\
	(cdf_header).formatted.version_made_by = SWAP_LE16((cdf_header).formatted.version_made_by); \\
	(cdf_header).formatted.external_file_attributes = SWAP_LE32((cdf_header).formatted.external_file_attributes); \\
	) \\
} while (0)
"""
        o = """
#define FIX_ENDIANNESS_CDF(cdf_header) do { 	(cdf_header).formatted.crc32        = SWAP_LE32((cdf_header).formatted.crc32       ); 	(cdf_header).formatted.cmpsize      = SWAP_LE32((cdf_header).formatted.cmpsize     ); 	(cdf_header).formatted.ucmpsize     = SWAP_LE32((cdf_header).formatted.ucmpsize    ); 	(cdf_header).formatted.file_name_length = SWAP_LE16((cdf_header).formatted.file_name_length); 	(cdf_header).formatted.extra_field_length = SWAP_LE16((cdf_header).formatted.extra_field_length); 	(cdf_header).formatted.file_comment_length = SWAP_LE16((cdf_header).formatted.file_comment_length); 	IF_DESKTOP( 	(cdf_header).formatted.version_made_by = SWAP_LE16((cdf_header).formatted.version_made_by); 	(cdf_header).formatted.external_file_attributes = SWAP_LE32((cdf_header).formatted.external_file_attributes); 	) } while (0)
"""

        self.equal_test(i, o)


    def test_anylines_with_empty_lines(self):
        i = """a
b

c"""
        o = """a
b

c"""
        self.equal_test(i, o)


    def test_has_closingbracket_IF_inline(self):
        i = """IF_DESKTOP(long long) int FAST_FUNC unpack_uncompress(unpack_info_t *info UNUSED_PARAM)"""
        o = """
#if defined CONFIG_DESKTOP
long long
#endif
 int FAST_FUNC unpack_uncompress(unpack_info_t *info UNUSED_PARAM)"""
        self.equal_test(i, o)


    def test_ENABLE_inline_multiline(self):
        i = """		if (ENABLE_FEATURE_SEAMLESS_BZ2
		 && magic.b16[0] == BZIP2_MAGIC
		) {
			unpack = unpack_bz2_stream;
		} else
		if (ENABLE_FEATURE_SEAMLESS_XZ
		 && magic.b16[0] == XZ_MAGIC1
		) {
			xread(rpm_fd, magic.b32, sizeof(magic.b32[0]));
			if (magic.b32[0] != XZ_MAGIC2)
				goto no_magic;
			/* unpack_xz_stream wants fd at position 6, no need to seek */
			//xlseek(rpm_fd, -6, SEEK_CUR);
			unpack = unpack_xz_stream;
                }"""
        o = """		if (
#if defined CONFIG_FEATURE_SEAMLESS_BZ2
1
#else
0
#endif
		 && magic.b16[0] == BZIP2_MAGIC
		) {
			unpack = unpack_bz2_stream;
		} else
		if (
#if defined CONFIG_FEATURE_SEAMLESS_XZ
1
#else
0
#endif
		 && magic.b16[0] == XZ_MAGIC1
		) {
			xread(rpm_fd, magic.b32, sizeof(magic.b32[0]));
			if (magic.b32[0] != XZ_MAGIC2)
				goto no_magic;
			/* unpack_xz_stream wants fd at position 6, no need to seek */
			//xlseek(rpm_fd, -6, SEEK_CUR);
			unpack = unpack_xz_stream;
                }"""
        self.equal_test(i, o)

    def test_busybox_wget_timeout(self):
        i = """
IF_FEATURE_WGET_TIMEOUT(&G.timeout_seconds) IF_NOT_FEATURE_WGET_TIMEOUT(NULL)
"""
        o = """

#if defined CONFIG_FEATURE_WGET_TIMEOUT
&G.timeout_seconds
#endif
 
#if !defined CONFIG_FEATURE_WGET_TIMEOUT
NULL
#endif
"""
        self.equal_test(i, o)

    def test_double_ENABLE(self):
        i = """#if ENABLE_FEATURE_ENABLE_USB
    new comment();
#endif"""
        o = """#if defined CONFIG_FEATURE_ENABLE_USB
    new comment();
#endif"""
        self.equal_test(i, o)

    def equal_test(self, i, o):
        """ runs normalizer on each line

        ensures that the input string i is equal to the reference string o
        """

        r = list()

        i = i.replace("\\\n", "\\")
        i = i.replace("\\", "")
        for line in i.splitlines():
            out = nl(line)
            if len(out) > 0:
                lines = out.splitlines()
                for res_line in lines:
                    r.append(res_line)
            else:
                r.append(out)
        self.assertEqual(r, o.splitlines())


if __name__ == '__main__':
    unittest2.main()
