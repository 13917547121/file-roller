/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <sys/types.h>
#include <time.h>
#include <libgnomevfs/gnome-vfs-file-size.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>


#define FILENAME_MAX_LENGTH 30 /* FIXME: find out the best value */

#define get_home_relative_dir(x)        \
        g_strconcat (g_get_home_dir (), \
                     "/",               \
                     (x),               \
                     NULL)

gboolean            path_is_file                 (const gchar *s);
gboolean            path_is_dir                  (const gchar *s);
gboolean            dir_is_empty                 (const gchar *s);
gboolean            path_in_path                 (const char  *path_src,
						  const char  *path_dest);
GnomeVFSFileSize    get_file_size                (const gchar *s);
time_t              get_file_mtime               (const gchar *s);
time_t              get_file_ctime               (const gchar *s);
gboolean            file_copy                    (const gchar *from, 
						  const gchar *to);
gboolean            file_move                    (const gchar *from, 
						  const gchar *to);
gint                file_in_path                 (const gchar *name);
gboolean            ensure_dir_exists            (const gchar *a_path,
						  mode_t       mode);
gboolean            file_is_hidden               (const gchar *name);
G_CONST_RETURN char*file_name_from_path          (const gchar *path);
gchar *             remove_level_from_path       (const gchar *path);
gchar *             remove_extension_from_path   (const gchar *path);
gchar *             remove_ending_separator      (const gchar *path);
gboolean            file_extension_is            (const char *filename, 
						  const char *ext);
void                path_list_free               (GList *path_list);
GList *             path_list_dup                (GList *path_list);
gboolean            is_mime_type                 (const char* type, 
						  const char* pattern);
GnomeVFSFileSize    get_dest_free_space          (const char  *path);
gboolean            rmdir_recursive              (const gchar *directory);
char *              get_temp_work_dir            (void);

/* misc functions used to parse a command output lines. */

gboolean            file_list__match_pattern     (const char *line, 
						  const char *pattern);
int                 file_list__get_index_from_pattern (const char *line, 
						       const char *pattern);
char*               file_list__get_next_field    (const char *line,
						  int         start_from,
						  int         field_n);
char*               file_list__get_prev_field    (const char *line,
						  int         start_from,
						  int         field_n);
gboolean            check_permissions            (const char *path,
						  int         mode);
gboolean 	    is_program_in_path		 (const char *filename);

#endif /* FILE_UTILS_H */
