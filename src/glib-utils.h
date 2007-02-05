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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef _GLIB_UTILS_H
#define _GLIB_UTILS_H

#include <time.h>

gboolean            strchrs                      (const char *str,
						  const char *chars);
char *              str_substitute               (const char *str,
						  const char *from_str,
						  const char *to_str);
int                 strcmp_null_tollerant        (const char *s1, const char *s2);
char*               escape_str_common            (const char *str,
						  const char *meta_chars,
						  const char  prefix,
						  const char  postfix);
char*               escape_str                   (const char  *str,
						  const char  *meta_chars);
char*               unescape_str                 (const char  *str);
gchar *             shell_escape                 (const gchar *filename);
gboolean            match_patterns               (char       **patterns,
						  const char  *string,
						  int          flags);
char **             search_util_get_patterns     (const char  *pattern_string);
char *              _g_strdup_with_max_size      (const char *s,
						  int         max_size);
const char *        eat_spaces                   (const char *line);
const char *        eat_void_chars               (const char *line);
char **             split_line                   (const char *line,
						  int   n_fields);
const char *        get_last_field               (const char *line,
						  int         last_field);
int                 n_fields                     (char      **str_array);
char *              get_time_string              (time_t      time);

/**/

#ifndef __GNUC__
#define __FUNCTION__ ""
#endif

#define DEBUG_INFO __FILE__, __LINE__, __FUNCTION__

void     debug                     (const char *file,
				    int         line,
				    const char *function,
				    const char *format, ...);

#endif /* _GLIB_UTILS_H */
