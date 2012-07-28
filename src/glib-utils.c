/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include "glib-utils.h"


#define MAX_PATTERNS 128


/* gobject utils*/


gpointer
_g_object_ref (gpointer object)
{
	return (object != NULL) ? g_object_ref (object) : NULL;
}


void
_g_object_unref (gpointer object)
{
	if (object != NULL)
		g_object_unref (object);
}


void
_g_clear_object (gpointer p)
{
	gpointer *object_p = (gpointer *) p;

	if ((object_p != NULL) && (*object_p != NULL)) {
		g_object_unref (*object_p);
		*object_p = NULL;
	}
}


/* error */


void
_g_error_free (GError *error)
{
	if (error != NULL)
		g_error_free (error);
}


/* string */


gboolean
_g_strchrs (const char *str,
	    const char *chars)
{
	const char *c;
	for (c = chars; *c != '\0'; c++)
		if (strchr (str, *c) != NULL)
			return TRUE;
	return FALSE;
}


char *
_g_str_substitute (const char *str,
		   const char *from_str,
		   const char *to_str)
{
	char    **tokens;
	int       i;
	GString  *gstr;

	if (str == NULL)
		return NULL;

	if (from_str == NULL)
		return g_strdup (str);

	if (strcmp (str, from_str) == 0)
		return g_strdup (to_str);

	tokens = g_strsplit (str, from_str, -1);

	gstr = g_string_new (NULL);
	for (i = 0; tokens[i] != NULL; i++) {
		gstr = g_string_append (gstr, tokens[i]);
		if ((to_str != NULL) && (tokens[i+1] != NULL))
			gstr = g_string_append (gstr, to_str);
	}

	return g_string_free (gstr, FALSE);
}


int
_g_strcmp_null_tolerant (const char *s1,
			 const char *s2)
{
	if ((s1 == NULL) && (s2 == NULL))
		return 0;
	else if ((s1 != NULL) && (s2 == NULL))
		return 1;
	else if ((s1 == NULL) && (s2 != NULL))
		return -1;
	else
		return strcmp (s1, s2);
}


/* -- _g_str_escape_full -- */


/* counts how many characters to escape in @str. */
static int
count_chars_to_escape (const char *str,
		       const char *meta_chars)
{
	int         meta_chars_n = strlen (meta_chars);
	const char *s;
	int         n = 0;

	for (s = str; *s != 0; s++) {
		int i;
		for (i = 0; i < meta_chars_n; i++)
			if (*s == meta_chars[i]) {
				n++;
				break;
			}
	}

	return n;
}


char *
_g_str_escape_full (const char *str,
		    const char *meta_chars,
		    const char  prefix,
		    const char  postfix)
{
	int         meta_chars_n = strlen (meta_chars);
	char       *escaped;
	int         i, new_l, extra_chars = 0;
	const char *s;
	char       *t;

	if (str == NULL)
		return NULL;

	if (prefix)
		extra_chars++;
	if (postfix)
		extra_chars++;

	new_l = strlen (str) + (count_chars_to_escape (str, meta_chars) * extra_chars);
	escaped = g_malloc (new_l + 1);

	s = str;
	t = escaped;
	while (*s) {
		gboolean is_bad = FALSE;
		for (i = 0; (i < meta_chars_n) && !is_bad; i++)
			is_bad = (*s == meta_chars[i]);
		if (is_bad && prefix)
			*t++ = prefix;
		*t++ = *s++;
		if (is_bad && postfix)
			*t++ = postfix;
	}
	*t = 0;

	return escaped;
}


/* escape with backslash the string @str. */
char *
_g_str_escape (const char *str,
	       const char *meta_chars)
{
	return _g_str_escape_full (str, meta_chars, '\\', 0);
}


/* escape with backslash the file name. */
char *
_g_str_shell_escape (const char *filename)
{
	return _g_str_escape (filename, "$'`\"\\!?* ()[]&|:;<>#");
}


char *
_g_strdup_with_max_size (const char *s,
			 int         max_size)
{
	char *result;
	int   l = strlen (s);

	if (l > max_size) {
		char *first_half;
		char *second_half;
		int   offset;
		int   half_max_size = max_size / 2 + 1;

		first_half = g_strndup (s, half_max_size);
		offset = half_max_size + l - max_size;
		second_half = g_strndup (s + offset, half_max_size);

		result = g_strconcat (first_half, "...", second_half, NULL);

		g_free (first_half);
		g_free (second_half);
	} else
		result = g_strdup (s);

	return result;
}


const char *
_g_str_eat_spaces (const char *line)
{
	if (line == NULL)
		return NULL;
	while ((*line == ' ') && (*line != 0))
		line++;
	return line;
}


const char *
_g_str_eat_void_chars (const char *line)
{
	if (line == NULL)
		return NULL;
	while (((*line == ' ') || (*line == '\t')) && (*line != 0))
		line++;
	return line;
}


char **
_g_str_split_line (const char *line,
		   int         n_fields)
{
	char       **fields;
	const char  *scan, *field_end;
	int          i;

	fields = g_new0 (char *, n_fields + 1);
	fields[n_fields] = NULL;

	scan = _g_str_eat_spaces (line);
	for (i = 0; i < n_fields; i++) {
		if (scan == NULL) {
			fields[i] = NULL;
			continue;
		}
		field_end = strchr (scan, ' ');
		if (field_end != NULL) {
			fields[i] = g_strndup (scan, field_end - scan);
			scan = _g_str_eat_spaces (field_end);
		}
	}

	return fields;
}


const char *
_g_str_get_last_field (const char *line,
		       int         last_field)
{
	const char *field;
	int         i;

	if (line == NULL)
		return NULL;

	last_field--;
	field = _g_str_eat_spaces (line);
	for (i = 0; i < last_field; i++) {
		if (field == NULL)
			return NULL;
		field = strchr (field, ' ');
		field = _g_str_eat_spaces (field);
	}

	return field;
}


GHashTable *static_strings = NULL;


const char *
_g_str_get_static (const char *s)
{
        const char *result;

        if (s == NULL)
                return NULL;

        if (static_strings == NULL)
                static_strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

        if (! g_hash_table_lookup_extended (static_strings, s, (gpointer*) &result, NULL)) {
                result = g_strdup (s);
                g_hash_table_insert (static_strings,
                                     (gpointer) result,
                                     GINT_TO_POINTER (1));
        }

        return result;
}


/* string vector */


char **
_g_strv_prepend (char       **str_array,
		 const char  *str)
{
	char **result;
	int    i;
	int    j;

	result = g_new (char *, g_strv_length (str_array) + 1);
	i = 0;
	result[i++] = g_strdup (str);
	for (j = 0; str_array[j] != NULL; j++)
		result[i++] = g_strdup (str_array[j]);
	result[i] = NULL;

	return result;
}


gboolean
_g_strv_remove (char       **str_array,
		const char  *str)
{
	int i;
	int j;

	if (str == NULL)
		return FALSE;

	for (i = 0; str_array[i] != NULL; i++)
		if (strcmp (str_array[i], str) == 0)
			break;

	if (str_array[i] == NULL)
		return FALSE;

	for (j = i; str_array[j] != NULL; j++)
		str_array[j] = str_array[j + 1];

	return TRUE;
}


/* string list */


void
_g_string_list_free (GList *path_list)
{
	if (path_list == NULL)
		return;
	g_list_foreach (path_list, (GFunc) g_free, NULL);
	g_list_free (path_list);
}


GList *
_g_string_list_dup (GList *path_list)
{
	GList *new_list = NULL;
	GList *scan;

	for (scan = path_list; scan; scan = scan->next)
		new_list = g_list_prepend (new_list, g_strdup (scan->data));

	return g_list_reverse (new_list);
}


/* GPtrArray */


GPtrArray *
_g_ptr_array_copy (GPtrArray *array)
{
	GPtrArray *new_array;

	if (array == NULL)
		return NULL;

	new_array = g_ptr_array_sized_new (array->len);
	memcpy (new_array->pdata, array->pdata, array->len * sizeof (gpointer));
	new_array->len = array->len;

	return new_array;
}


void
_g_ptr_array_free_full (GPtrArray *array,
                        GFunc      free_func,
                        gpointer   user_data)
{
	g_ptr_array_foreach (array, free_func, user_data);
	g_ptr_array_free (array, TRUE);
}


void
_g_ptr_array_reverse (GPtrArray *array)
{
	int      i, j;
	gpointer tmp;

	for (i = 0; i < array->len / 2; i++) {
		j = array->len - i - 1;
		tmp = g_ptr_array_index (array, i);
		g_ptr_array_index (array, i) = g_ptr_array_index (array, j);
		g_ptr_array_index (array, j) = tmp;
	}
}


int
_g_ptr_array_binary_search (GPtrArray    *array,
			   gpointer      value,
			   GCompareFunc  func)
{
	int l, r, p, cmp = -1;

	l = 0;
	r = array->len;
	while (l < r) {
		p = l + ((r - l) / 2);
		cmp = func(value, &g_ptr_array_index (array, p));
		if (cmp == 0)
			return p;
		else if (cmp < 0)
			r = p;
		else
			l = p + 1;
	}

	return -1;
}


/* GRegex */


gboolean
_g_regexp_matchv (GRegex           **regexps,
	          const char        *string,
	          GRegexMatchFlags   match_options)
{
	gboolean matched;
	int      i;

	if ((regexps == NULL) || (regexps[0] == NULL))
		return TRUE;

	if (string == NULL)
		return FALSE;

	matched = FALSE;
	for (i = 0; regexps[i] != NULL; i++)
		if (g_regex_match (regexps[i], string, match_options, NULL)) {
			matched = TRUE;
			break;
		}

	return matched;
}


void
_g_regexp_freev (GRegex **regexps)
{
	int i;

	if (regexps == NULL)
		return;

	for (i = 0; regexps[i] != NULL; i++)
		g_regex_unref (regexps[i]);
	g_free (regexps);
}


/* -- _g_regexp_get_patternv -- */


static const char *
_g_utf8_strstr (const char *haystack,
		const char *needle)
{
	const char *s;
	gsize       i;
	gsize       haystack_len = g_utf8_strlen (haystack, -1);
	gsize       needle_len = g_utf8_strlen (needle, -1);
	int         needle_size = strlen (needle);

	s = haystack;
	for (i = 0; i <= haystack_len - needle_len; i++) {
		if (strncmp (s, needle, needle_size) == 0)
			return s;
		s = g_utf8_next_char(s);
	}

	return NULL;
}


static char **
_g_utf8_strsplit (const char *string,
		  const char *delimiter,
		  int         max_tokens)
{
	GSList      *string_list = NULL, *slist;
	char       **str_array;
	const char  *s;
	guint        n = 0;
	const char  *remainder;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != NULL, NULL);
	g_return_val_if_fail (delimiter[0] != '\0', NULL);

	if (max_tokens < 1)
		max_tokens = G_MAXINT;

	remainder = string;
	s = _g_utf8_strstr (remainder, delimiter);
	if (s != NULL) {
		gsize delimiter_size = strlen (delimiter);

		while (--max_tokens && (s != NULL)) {
			gsize  size = s - remainder;
			char  *new_string;

			new_string = g_new (char, size + 1);
			strncpy (new_string, remainder, size);
			new_string[size] = 0;

			string_list = g_slist_prepend (string_list, new_string);
			n++;
			remainder = s + delimiter_size;
			s = _g_utf8_strstr (remainder, delimiter);
		}
	}
	if (*string) {
		n++;
		string_list = g_slist_prepend (string_list, g_strdup (remainder));
	}

	str_array = g_new (char*, n + 1);

	str_array[n--] = NULL;
	for (slist = string_list; slist; slist = slist->next)
		str_array[n--] = slist->data;

	g_slist_free (string_list);

	return str_array;
}


static char*
g_utf8_strchug (char *string)
{
	char     *scan;
	gunichar  c;

	g_return_val_if_fail (string != NULL, NULL);

	scan = string;
	c = g_utf8_get_char (scan);
	while (g_unichar_isspace (c)) {
		scan = g_utf8_next_char (scan);
		c = g_utf8_get_char (scan);
	}

	g_memmove (string, scan, strlen (scan) + 1);

	return string;
}


static char*
g_utf8_strchomp (char *string)
{
	char   *scan;
	gsize   len;

	g_return_val_if_fail (string != NULL, NULL);

	len = g_utf8_strlen (string, -1);

	if (len == 0)
		return string;

	scan = g_utf8_offset_to_pointer (string, len - 1);

	while (len--) {
		gunichar c = g_utf8_get_char (scan);
		if (g_unichar_isspace (c))
			*scan = '\0';
		else
			break;
		scan = g_utf8_find_prev_char (string, scan);
	}

	return string;
}


#define g_utf8_strstrip(string) g_utf8_strchomp (g_utf8_strchug (string))


char **
_g_regexp_get_patternv (const char *pattern_string)
{
	char **patterns;
	int    i;

	if (pattern_string == NULL)
		return NULL;

	patterns = _g_utf8_strsplit (pattern_string, ";", MAX_PATTERNS);
	for (i = 0; patterns[i] != NULL; i++) {
		char *p1, *p2;

		p1 = g_utf8_strstrip (patterns[i]);
		p2 = _g_str_substitute (p1, ".", "\\.");
		patterns[i] = _g_str_substitute (p2, "*", ".*");

		g_free (p2);
		g_free (p1);
	}

	return patterns;
}


GRegex **
_g_regexp_split_from_patterns (const char         *pattern_string,
			       GRegexCompileFlags  compile_options)
{
	char   **patterns;
	GRegex **regexps;
	int      i;

	patterns = _g_regexp_get_patternv (pattern_string);
	if (patterns == NULL)
		return NULL;

	regexps = g_new0 (GRegex*, g_strv_length (patterns) + 1);
	for (i = 0; patterns[i] != NULL; i++)
		regexps[i] = g_regex_new (patterns[i],
					  G_REGEX_OPTIMIZE | compile_options,
					  G_REGEX_MATCH_NOTEMPTY,
					  NULL);
	g_strfreev (patterns);

	return regexps;
}


/* time */


char *
_g_time_to_string (time_t time)
{
	struct tm *tm;
	char       s_time[256];
	char      *locale_format = NULL;
	char      *time_utf8;

	tm = localtime (&time);
	/* This is the time format used in the "Date Modified" column and
	 * in the Properties dialog.  See the man page of strftime for an
	 * explanation of the values. */
	locale_format = g_locale_from_utf8 (_("%d %B %Y, %H:%M"), -1, NULL, NULL, NULL);
	strftime (s_time, sizeof (s_time) - 1, locale_format, tm);
	g_free (locale_format);
	time_utf8 = g_locale_to_utf8 (s_time, -1, NULL, NULL, NULL);

	return time_utf8;
}


/* uri/path/filename */


char*
_g_uri_display_basename (const char  *uri)
{
	char *e_name, *name;

	e_name = g_filename_display_basename (uri);
	name = g_uri_unescape_string (e_name, "");
	g_free (e_name);

	return name;
}


const char *
_g_uri_get_home (void)
{
	static char *home_uri = NULL;
	if (home_uri == NULL)
		home_uri = g_filename_to_uri (g_get_home_dir (), NULL, NULL);
	return home_uri;
}


char *
_g_uri_get_home_relative (const char *partial_uri)
{
	return g_strconcat (_g_uri_get_home (),
			    "/",
			    partial_uri,
			    NULL);
}


const char *
_g_uri_remove_host (const char *uri)
{
        const char *idx, *sep;

        if (uri == NULL)
                return NULL;

        idx = strstr (uri, "://");
        if (idx == NULL)
                return uri;
        idx += 3;
        if (*idx == '\0')
                return "/";
        sep = strstr (idx, "/");
        if (sep == NULL)
                return idx;
        return sep;
}


char *
_g_uri_get_host (const char *uri)
{
	const char *idx;

	idx = strstr (uri, "://");
	if (idx == NULL)
		return NULL;
	idx = strstr (idx + 3, "/");
	if (idx == NULL)
		return NULL;
	return g_strndup (uri, (idx - uri));
}


char *
_g_uri_get_root (const char *uri)
{
	char *host;
	char *root;

	host = _g_uri_get_host (uri);
	if (host == NULL)
		return NULL;
	root = g_strconcat (host, "/", NULL);
	g_free (host);

	return root;
}


gboolean
_g_uri_is_local (const char  *uri)
{
	return strncmp (uri, "file://", 7) == 0;
}


int
_g_uri_cmp (const char *uri1,
	    const char *uri2)
{
	return _g_strcmp_null_tolerant (uri1, uri2);
}


char *
_g_uri_build (const char *base, ...)
{
	va_list     args;
	const char *child;
	GString    *uri;

	uri = g_string_new (base);

	va_start (args, base);
        while ((child = va_arg (args, const char *)) != NULL) {
        	if (! g_str_has_suffix (uri->str, "/") && ! g_str_has_prefix (child, "/"))
        		g_string_append (uri, "/");
        	g_string_append (uri, child);
        }
	va_end (args);

	return g_string_free (uri, FALSE);
}


/* like g_path_get_basename but does not warn about NULL and does not
 * alloc a new string. */
const gchar *
_g_path_get_file_name (const gchar *file_name)
{
	register char   *base;
	register gssize  last_char;

	if (file_name == NULL)
		return NULL;

	if (file_name[0] == '\0')
		return "";

	last_char = strlen (file_name) - 1;

	if (file_name [last_char] == G_DIR_SEPARATOR)
		return "";

	base = g_utf8_strrchr (file_name, -1, G_DIR_SEPARATOR);
	if (! base)
		return file_name;

	return base + 1;
}


char *
_g_path_get_dir_name (const gchar *path)
{
	register gssize base;
	register gssize last_char;

	if (path == NULL)
		return NULL;

	if (path[0] == '\0')
		return g_strdup ("");

	last_char = strlen (path) - 1;
	if (path[last_char] == G_DIR_SEPARATOR)
		last_char--;

	base = last_char;
	while ((base >= 0) && (path[base] != G_DIR_SEPARATOR))
		base--;

	return g_strndup (path + base + 1, last_char - base);
}


gchar *
_g_path_remove_level (const gchar *path)
{
	int         p;
	const char *ptr = path;
	char       *new_path;

	if (path == NULL)
		return NULL;

	p = strlen (path) - 1;
	if (p < 0)
		return NULL;

	while ((p > 0) && (ptr[p] != '/'))
		p--;
	if ((p == 0) && (ptr[p] == '/'))
		p++;
	new_path = g_strndup (path, (guint)p);

	return new_path;
}


char *
_g_path_remove_ending_separator (const char *path)
{
	gint len, copy_len;

	if (path == NULL)
		return NULL;

	copy_len = len = strlen (path);
	if ((len > 1) && (path[len - 1] == '/'))
		copy_len--;

	return g_strndup (path, copy_len);
}


gchar *
_g_path_remove_extension (const gchar *path)
{
	int         len;
	int         p;
	const char *ptr = path;
	char       *new_path;

	if (! path)
		return NULL;

	len = strlen (path);
	if (len == 1)
		return g_strdup (path);

	p = len - 1;
	while ((p > 0) && (ptr[p] != '.'))
		p--;
	if (p == 0)
		p = len;
	new_path = g_strndup (path, (guint) p);

	return new_path;
}

/* Check whether the dirname is contained in filename */
gboolean
_g_path_is_parent_of (const char *dirname,
		      const char *filename)
{
	int dirname_l, filename_l, separator_position;

	if ((dirname == NULL) || (filename == NULL))
		return FALSE;

	dirname_l = strlen (dirname);
	filename_l = strlen (filename);

	if ((dirname_l == filename_l + 1)
	     && (dirname[dirname_l - 1] == '/'))
		return FALSE;

	if ((filename_l == dirname_l + 1)
	     && (filename[filename_l - 1] == '/'))
		return FALSE;

	if (dirname[dirname_l - 1] == '/')
		separator_position = dirname_l - 1;
	else
		separator_position = dirname_l;

	return ((filename_l > dirname_l)
		&& (strncmp (dirname, filename, dirname_l) == 0)
		&& (filename[separator_position] == '/'));
}


const char *
_g_path_get_basename (const char *path,
		       const char *base_dir,
		       gboolean    junk_paths)
{
	int         base_dir_len;
	const char *base_path;

	if (junk_paths)
		return _g_path_get_file_name (path);

	base_dir_len = strlen (base_dir);
	if (strlen (path) <= base_dir_len)
		return NULL;

	base_path = path + base_dir_len;
	if (path[0] != '/')
		base_path -= 1;

	return base_path;
}


gboolean
_g_filename_is_hidden (const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0') return FALSE;
	if ((name[1] == '.') && (name[2] == '\0')) return FALSE;

	return TRUE;
}


const char *
_g_filename_get_extension (const char *filename)
{
	const char *ptr = filename;
	int         len;
	int         p;
	const char *ext;

	if (filename == NULL)
		return NULL;

	len = strlen (filename);
	if (len <= 1)
		return NULL;

	p = len - 1;
	while ((p >= 0) && (ptr[p] != '.'))
		p--;
	if (p < 0)
		return NULL;

	ext = filename + p;
	if (ext - 4 > filename) {
		const char *test = ext - 4;
		if (strncmp (test, ".tar", 4) == 0)
			ext = ext - 4;
	}
	return ext;
}


gboolean
_g_filename_has_extension (const char *filename,
		   const char *ext)
{
	int filename_l, ext_l;

	filename_l = strlen (filename);
	ext_l = strlen (ext);

	if (filename_l < ext_l)
		return FALSE;
	return strcasecmp (filename + filename_l - ext_l, ext) == 0;
}


char *
_g_filename_get_random (int length)
{
	const char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	const int   n_letters = strlen (letters);
	char       *result, *c;
	GRand      *rand;
	int         i;

	result = g_new (char, length + 1);

	rand = g_rand_new ();
	for (i = 0, c = result; i < length; i++, c++)
		*c = letters[g_rand_int_range (rand, 0, n_letters)];
	*c = '\0';
	g_rand_free (rand);

	return result;
}


gboolean
_g_mime_type_matches (const char *mime_type,
		      const char *pattern)
{
	return (strcasecmp (mime_type, pattern) == 0);
}


/* GFile */


GFile *
_g_file_new_home_relative (const char *partial_uri)
{
	GFile *file;
	char  *uri;

	uri = g_strconcat (_g_uri_get_home (), "/", partial_uri, NULL);
	file = g_file_new_for_uri (uri);
	g_free (uri);

	return file;
}


GList *
_g_file_list_dup (GList *l)
{
	GList *r = NULL, *scan;
	for (scan = l; scan; scan = scan->next)
		r = g_list_prepend (r, g_file_dup ((GFile*) scan->data));
	return g_list_reverse (r);
}


void
_g_file_list_free (GList *l)
{
	GList *scan;
	for (scan = l; scan; scan = scan->next)
		g_object_unref (scan->data);
	g_list_free (l);
}


GList *
_g_file_list_new_from_uri_list (GList *uris)
{
	GList *r = NULL, *scan;
	for (scan = uris; scan; scan = scan->next)
		r = g_list_prepend (r, g_file_new_for_uri ((char*)scan->data));
	return g_list_reverse (r);
}


/* line parser */


gboolean
_g_line_matches_pattern (const char *line,
			 const char *pattern)
{
	const char *l = line, *p = pattern;

	for (/* void */; (*p != 0) && (*l != 0); p++, l++) {
		if (*p != '%') {
			if (*p != *l)
				return FALSE;
		}
		else {
			p++;
			switch (*p) {
			case 'a':
				break;
			case 'n':
				if (!isdigit (*l))
					return FALSE;
				break;
			case 'c':
				if (!isalpha (*l))
					return FALSE;
				break;
			default:
				return FALSE;
			}
		}
	}

	return (*p == 0);
}


int
_g_line_get_index_from_pattern (const char *line,
				const char *pattern)
{
	int         line_l, pattern_l;
	const char *l;

	line_l = strlen (line);
	pattern_l = strlen (pattern);

	if ((pattern_l == 0) || (line_l == 0))
		return -1;

	for (l = line; *l != 0; l++)
		if (_g_line_matches_pattern (l, pattern))
			return (l - line);

	return -1;
}


char*
_g_line_get_next_field (const char *line,
			int         start_from,
			int         field_n)
{
	const char *f_start, *f_end;

	line = line + start_from;

	f_start = line;
	while ((*f_start == ' ') && (*f_start != *line))
		f_start++;
	f_end = f_start;

	while ((field_n > 0) && (*f_end != 0)) {
		if (*f_end == ' ') {
			field_n--;
			if (field_n != 0) {
				while ((*f_end == ' ') && (*f_end != *line))
					f_end++;
				f_start = f_end;
			}
		}
		else
			f_end++;
	}

	return g_strndup (f_start, f_end - f_start);
}


char*
_g_line_get_prev_field (const char *line,
			int         start_from,
			int         field_n)
{
	const char *f_start, *f_end;

	f_start = line + start_from - 1;
	while ((*f_start == ' ') && (*f_start != *line))
		f_start--;
	f_end = f_start;

	while ((field_n > 0) && (*f_start != *line)) {
		if (*f_start == ' ') {
			field_n--;
			if (field_n != 0) {
				while ((*f_start == ' ') && (*f_start != *line))
					f_start--;
				f_end = f_start;
			}
		}
		else
			f_start--;
	}

	return g_strndup (f_start + 1, f_end - f_start);
}


/* debug */

void
debug (const char *file,
       int         line,
       const char *function,
       const char *format, ...)
{
#ifdef DEBUG
	va_list  args;
	char    *str;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	g_fprintf (stderr, "[FR] %s:%d (%s):\n\t%s\n", file, line, function, str);

	g_free (str);
#else /* ! DEBUG */
#endif
}
