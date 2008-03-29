/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003, 2008 Free Software Foundation, Inc.
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

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include "glib-utils.h"
#include "file-utils.h"
#include "gio-utils.h"


#define N_FILES_PER_REQUEST 128


/* -- filter -- */


typedef enum {
	FILTER_DEFAULT = 0,
	FILTER_NODOTFILES = 1 << 1,
	FILTER_IGNORECASE = 1 << 2,
	FILTER_NOBACKUPFILES = 1 << 3
} FilterOptions;


typedef struct {
	char           *pattern;
	char          **patterns;
	FilterOptions   options;
	GRegex        **regexps;
} Filter;


static Filter *
filter_new (const char    *pattern,
	    FilterOptions  options)
{
	Filter             *filter;
	GRegexCompileFlags  flags;
	int                 i;
	
	filter = g_new0 (Filter, 1);

	if (pattern != NULL) {
		filter->pattern = g_strdup (pattern);
		filter->patterns = search_util_get_patterns (pattern);
	}

	filter->options = options;
	if (filter->options & FILTER_IGNORECASE)
		flags = G_REGEX_CASELESS;
	else
		flags = 0;
	
	if (pattern != NULL) {
		filter->regexps = g_new0 (GRegex*, n_fields (filter->patterns) + 1);
		for (i = 0; filter->patterns[i] != NULL; i++) 
			filter->regexps[i] = g_regex_new (filter->patterns[i],
						          flags,
						          G_REGEX_MATCH_NOTEMPTY,
						          NULL);
		filter->regexps[i] = NULL;
	}
	
	return filter;
}


static void
filter_destroy (Filter *filter)
{	
	g_return_if_fail (filter != NULL);

	g_free (filter->pattern);
	if (filter->patterns != NULL)
		g_strfreev (filter->patterns);
	if (filter->regexps != NULL) {
		int i;
		for (i = 0; filter->regexps[i] != NULL; i++)
			 g_regex_unref (filter->regexps[i]);
		g_free (filter->regexps);
	}
	g_free (filter);
}


static gboolean
match_regexps (GRegex     **regexps,
	       const char  *string)
{
	gboolean matched;
	int      i;
	
	if ((regexps == NULL) || (regexps[0] == NULL))
		return TRUE;

	if (string == NULL)
		return FALSE;
	
	matched = FALSE;
	for (i = 0; regexps[i] != NULL; i++)
		if (g_regex_match (regexps[i], string, 0, NULL)) {
			matched = TRUE;
			break;
		}
		
	return matched;
}


static gboolean
filter_matches (Filter     *filter,
	        const char *name)
{
	const char *file_name;
	char       *utf8_name;
	gboolean    matched;

	g_return_val_if_fail (name != NULL, FALSE);

	file_name = file_name_from_path (name);

	if ((filter->options & FILTER_NODOTFILES)
	    && ((file_name[0] == '.') || (strstr (file_name, "/.") != NULL)))
		return FALSE;

	if ((filter->options & FILTER_NOBACKUPFILES)
	    && (file_name[strlen (file_name) - 1] == '~'))
		return FALSE;

	if (filter->pattern == NULL)
		return TRUE;
	
	utf8_name = g_filename_to_utf8 (file_name, -1, NULL, NULL, NULL);
	matched = match_regexps (filter->regexps, utf8_name);
	g_free (utf8_name);

	return matched;
}


static gboolean
filter_empty (Filter *filter)
{
	return ((filter->pattern == NULL) || (strcmp (filter->pattern, "*") == 0))
}


/* -- g_directory_foreach_child -- */


typedef struct {
	char                 *base_directory;
	gboolean              recursive;
	gboolean              follow_links;
	StartDirCallback      start_dir_func;
	ForEachChildCallback  for_each_file_func;
	ForEachDoneCallback   done_func;
	gpointer              user_data;

	/* private */

	GFile                *current;
	GHashTable           *already_visited;
	GList                *to_visit;
	Filter               *filter;
	GCancellable         *cancellable;
	GFileEnumerator      *enumerator;
	GError               *error;	
} ForEachChildData;


static void
for_each_child_data_free (ForEachChildData *fec)
{
	if (fec == NULL)
		return;

	g_free (fec->base_directory);
	if (fec->current != NULL)
		g_object_unref (fec->current);
	if (fec->already_visited)
		g_hash_table_destroy (fec->already_visited);
	if (fec->to_visit != NULL)
		path_list_free (fec->to_visit);
	if (fec->filter)
		filter_destroy (fec->filter);
	if (fec->error != NULL)
		g_error_free (fec->error);
	g_free (fec);
}


static void for_each_child_start (ForEachChildData *fec, const char *directory);


static void
for_each_child_done (ForEachChildData *fec)
{
	if (fec->done_func) 
		fec->done_func (fec->error, fec->user_data);
	for_each_child_data_free (fec);
}


static void  
for_each_child_next_files_ready (GObject      *source_object,
			         GAsyncResult *result,
			         gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	GList            *children, *scan;
	char             *current_directory;
	
	children = g_file_enumerator_next_files_finish (fec->enumerator,
                                                        result,
                                                        &(fec->error));
                                                        
	if (children == NULL) {
		if ((fec->error == NULL) && fec->recursive) {
			char *sub_directory = NULL;
			
			if (fec->to_visit != NULL) {
				GList *tmp;
				
				sub_directory = (char*) fec->to_visit->data;
				tmp = fec->to_visit;
				fec->to_visit = g_list_remove_link (fec->to_visit, tmp);
				g_list_free (tmp);
			}
			
			if (sub_directory != NULL) {
				for_each_child_start (fec, sub_directory);
				return;
			}
		}
		for_each_child_done (fec);
		return;
	}
	
	current_directory = g_file_get_uri (fec->current);
	for (scan = children; scan; scan = scan->next) {
		GFileInfo *child_info = scan->data;
		char      *name, *uri;

		name = g_uri_escape_string (g_file_info_get_name (child_info), G_URI_RESERVED_CHARS_ALLOWED_IN_PATH_ELEMENT, FALSE);
		uri = g_strconcat (current_directory, "/", name, NULL);

		if (g_file_info_get_file_type (child_info) == G_FILE_TYPE_DIRECTORY) {
			char *sub_directory;
			
			/* avoid to visit a directory more than ones */

			sub_directory = g_strdup (uri);
			if (g_hash_table_lookup (fec->already_visited, sub_directory) == NULL) {				
				g_hash_table_insert (fec->already_visited, sub_directory, GINT_TO_POINTER (1));
				fec->to_visit = g_list_append (fec->to_visit, sub_directory);
			}
			else
				g_free (sub_directory);
		}
		
		fec->for_each_func (uri, child_info, fec->user_data);
		
		g_free (uri);		
		g_free (name);
	}
	g_free (current_directory);
	
	g_file_enumerator_next_files_async (fec->enumerator,
                                            N_FILES_PER_REQUEST,
                                            G_PRIORITY_DEFAULT,
                                            fec->cancellable,
                                            for_each_child_next_files_ready,
                                            fec);
}

static void
for_each_child_ready (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	ForEachChildData *fec = user_data;
	
	fec->enumerator = g_file_enumerate_children_finish (fec->current, result, &(fec->error));
	if (fec->enumerator == NULL) {
		for_each_child_done (fec);
		return;
	}
	
	g_file_enumerator_next_files_async (fec->enumerator,
                                            N_FILES_PER_REQUEST,
                                            G_PRIORITY_DEFAULT,
                                            fec->cancellable,
                                            for_each_child_next_files_ready,
                                            fec);
}


static void
for_each_child_start (ForEachChildData *fec,
		      const char       *directory)
{
	if (fec->start_dir_func != NULL) 
		if (! fec->start_dir_func (directory, &(fec->error), fec->user_data)) {
			for_each_child_done (fec);
			return;
		}
	
	if (fec->current != NULL)
		g_object_unref (fec->current);
	fec->current = g_file_new_for_uri (directory);
	
	g_file_enumerate_children_async (fec->current,
					 "standard::name,standard::type",
					 fec->follow_links ? G_FILE_QUERY_INFO_NONE : G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
					 G_PRIORITY_DEFAULT,
                                         fec->cancellable,
					 for_each_child_ready,
					 fec);
}


void
g_directory_foreach_child (const char           *directory,
			   gboolean              recursive,
			   gboolean              follow_links,
			   GCancellable         *cancellable,
			   StartDirCallback      start_dir_func,
			   ForEachChildCallback  for_each_file_func,
			   ForEachDoneCallback   done_func,
			   gpointer              user_data)
{
	ForEachChildData *fec;
	
	g_return_if_fail (for_each_file_func != NULL);

	fec = g_new0 (ForEachChildData, 1);
	
	fec->base_directory = g_strdup (directory);
	fec->recursive = recursive;
	fec->follow_links = follow_links;
	fev->cancellable = cancellable;
	fec->start_dir_func = start_dir_func;
	fec->for_file_each_func = for_each_file_func;
	fec->done_func = done_func;
	fec->user_data = user_data;
	fec->already_visited = g_hash_table_new_full (g_str_hash, 
						      g_str_equal,
						      g_free,
						      NULL);
	
	for_each_child_start (fec, fec->base_directory);	
}


/* -- get_file_list_data -- */


typedef struct {
	GList             *files;
	GList             *dirs;
	VisitDirHandle    *handle;

	char              *directory;
	char              *base_dir;
	VisitDirDoneFunc   done_func;
	gpointer           done_data;

	GList             *to_visit;
	GList             *current_dir;

	Filter            *include_filter;
	Filter            *exclude_filter;
	guint              visit_timeout;
} GetFileListData;


static void
get_file_list_data_free (GetFileListData *gfl)
{
	if (gfl == NULL)
		return;

	filter_destroy (gfl->include_filter);
	filter_destroy (gfl->exclude_filter);
	path_list_free (gfl->files);
	path_list_free (gfl->dirs);
	path_list_free (gfl->to_visit);
	g_free (gfl->directory);
	g_free (gfl->base_dir);
	g_free (gfl);
}


/* -- g_directory_list_async -- */


static GList*
get_relative_file_list (GList      *rel_list,
			GList      *file_list,
			const char *base_dir)
{
	GList *scan;
	int    base_len;

	if (base_dir == NULL)
		return NULL;

	base_len = 0;
	if (strcmp (base_dir, "/") != 0)
		base_len = strlen (base_dir);
		
	for (scan = file_list; scan; scan = scan->next) {
		char *full_path = scan->data;
		if (path_in_path (base_dir, full_path)) {
			char *rel_path = g_strdup (full_path + base_len + 1);
			rel_list = g_list_prepend (rel_list, rel_path);
		}
	}
	
	return rel_list;
}


static GList*
get_dir_list_from_file_list (GHashTable *h_dirs,
			     const char *base_dir,
			     GList      *files,
			     gboolean    is_dir_list)
{
	GList *scan;
	GList *dir_list = NULL;
	int    base_dir_len = strlen (base_dir);
	
	for (scan = files; scan; scan = scan->next) {
		char *filename = scan->data;
		char *dir_name;

		if (is_dir_list)
			dir_name = g_strdup (filename + base_dir_len + 1);
		else
			dir_name = remove_level_from_path (filename + base_dir_len + 1);

		while ((dir_name != NULL) && (dir_name[0] != '\0') && (strcmp (dir_name, "/") != 0)) {
			char *tmp;
			char *dir;

			/* avoid to insert duplicated folders */

			dir = g_strconcat (base_dir, "/", dir_name, NULL);
			if (g_hash_table_lookup (h_dirs, dir) == NULL) {
				g_hash_table_insert (h_dirs, dir, GINT_TO_POINTER (1));
				dir_list = g_list_prepend (dir_list, dir);
			} 
			else
				g_free (dir);

			tmp = dir_name;
			dir_name = remove_level_from_path (tmp);
			g_free (tmp);
		}

		g_free (dir_name);
	}

	return dir_list;
}


static void
get_file_list_done (GError   *error,
		    gpointer  user_data)
{
	GetFileListData *gfl = user_data;
	GHashTable      *h_dirs;
	char            *dir;
	
	gfl->files = g_list_reverse (gfl->files);
	gfl->dirs = g_list_reverse (gfl->dirs);
	
	if (! filter_empty (gfl->include_filter) || (gfl->exclude_filter->pattern != NULL)) {
		path_list_free (gfl->dirs);
		gfl->dirs = NULL;
	}

	/* Always include the base directory, this way empty base 
 	 * directories are added to the archive as well.  */
		 
	dir = g_strdup (gfl->base_dir);
	gfl->dirs = g_list_prepend (gfl->dirs, dir);
	g_hash_table_insert (h_dirs, dir, GINT_TO_POINTER (1));
	
	/* Add all the parent directories in gfl->files/gfl->dirs to the 
	 * gfl->dirs list, the hash table is used to avoid duplicated 
	 * entries. */
	
	h_dirs = g_hash_table_new (g_str_hash, g_str_equal);
	for (scan = gfl->dirs; scan; scan = scan->next)
		g_hash_table_insert (h_dirs, (char*)scan->data, GINT_TO_POINTER (1));
	
	gfl->dirs = g_list_concat (gfl->dirs, get_dir_list_from_file_list (h_dirs, gfl->base_dir, gfl->files, FALSE));
	
	if (filter_empty (gfl->include_filter))
		gfl->dirs = g_list_concat (gfl->dirs, get_dir_list_from_file_list (h_dirs, gfl->base_dir, gfl->dirs, TRUE));
	
	/**/
	
	if (error == NULL) {
		GList *rel_files, *rel_dirs;
		
		if (gfl->base_dir != NULL) {
			rel_files = get_relative_file_list (NULL, gfl->files, gfl->base_dir);
			rel_dirs = get_relative_file_list (NULL, gfl->dirs, gfl->base_dir);
		}
		else {
			rel_files = gfl->files;
			rel_dirs = gfl->dirs;
			gfl->files = NULL;
			gfl->dirs = NULL;
		}
		
		/* rel_files/rel_dirs must be deallocated in done_func */
		gfl->done_func (rel_files, rel_dirs, NULL, gfl->done_data);
	}
	else
		gfl->done_func (NULL, NULL, error, gfl->done_data);
	
	g_hash_table_destroy (h_dirs);
	get_file_list_data_free (gfl_data);
}


static void
get_file_list_for_each_file (const char *uri,
			     GFileInfo  *info, 
			     gpointer    user_data)
{
	GetFileListData *gfl = user_data;

	switch (g_file_info_get_file_type (info)) {
	case G_FILE_TYPE_DIRECTORY:
		gfl->dirs = g_list_prepend (gfl->dirs, g_strdup (uri));
		break;
	case G_FILE_TYPE_DIRECTORY:
		if (filter_matches (gfl->include_filter, uri)) 
			if ((gfl->exclude_filter->pattern == NULL) || ! filter_matches (gfl->exclude_filter, uri))		
				gfl->files = g_list_prepend (gfl->files, g_strdup (uri));
		break;	
	default:
		break;
	}
}


void
g_directory_list_async (const char            *directory, 
		       const char            *base_dir,
		       gboolean               recursive,
		       gboolean               follow_links,
		       gboolean               no_backup_files,
		       gboolean               no_dot_files,
		       const char            *include_files,
		       const char            *exclude_files,
		       gboolean               ignorecase,
		       GCancellable          *cancellable,
		       ListReadyCallback      done_func,
		       gpointer               done_data)
{
	GetFileListData *gfl;
	FilterOptions    filter_options;
	
	gfl = g_new0 (GetFileListData, 1);
	gfl->directory = get_uri_from_path (directory);
	gfl->base_dir = g_strdup (base_dir);
	gfl->done_func = done_func;
	gfl->done_data = done_data;
	
	filter_options = FILTER_DEFAULT;
	if (no_backup_files)
		filter_options |= FILTER_NOBACKUPFILES;
	if (no_dot_files)
		filter_options |= FILTER_NODOTFILES;
	if (ignorecase)
		filter_options |= FILTER_IGNORECASE;
	gfl->include_filter = filter_new (include_files, filter_options);
	gfl->exclude_filter = filter_new (exclude_files, ignorecase ? FILTER_IGNORECASE : FILTER_DEFAULT);
	
	g_directory_foreach_child (directory,
				   recursive,
				   follow_links,
				   cancellable,
				   NULL,
				   get_file_list_for_each_file,
				   get_file_list_done,
				   gfl);
}


/* -- g_list_items_async -- */


static void get_items_for_current_dir (GetFileListData *gfl);


static gboolean
get_items_for_next_dir_idle_cb (gpointer data)
{
	GetFileListData *gfl = data;

	g_source_remove (gfl->visit_timeout);
	gfl->visit_timeout = 0;

	gfl->current_dir = g_list_next (gfl->current_dir);
	get_items_for_current_dir (gfl);

	return FALSE;
}


static void
get_items_for_current_dir_done (GList    *files,
			        GList    *dirs,
			        GError   *error,
			        gpointer  data)
{
	GetFileListData *gfl = data;

	if (error != NULL) {
		if (gfl->done_func)
			gfl->done_func (NULL, NULL, error, gfl->done_data);
		path_list_free (files);
		path_list_free (dirs);
		get_file_list_data_free (gfl);
		return;
	}

	gfl->files = g_list_concat (gfl->files, files);
	gfl->dirs = g_list_concat (gfl->dirs, dirs);

	gfl->visit_timeout = g_idle_add (get_items_for_next_dir_idle_cb, gfl);
}


static void
get_items_for_current_dir (GetFileListData *gfl)
{
	const char *directory_name;
	char       *directory_uri;

	if (gfl->current_dir == NULL) {
		if (gfl->done_func) {
			/* gfl->files/gfl->dirs must be deallocated in gfl->done_func */
			gfl->done_func (gfl->files, gfl->dirs, NULL, gfl->done_data);
			gfl->files = NULL;
			gfl->dirs = NULL;
		}
		get_file_list_data_free (gfl);
		return NULL;
	}

	directory_name = file_name_from_path ((char*) gfl->current_dir->data);
	if (strcmp (gfl->base_dir, "/") == 0)
		directory_uri = g_strconcat (gfl->base_dir, directory_name, NULL);
	else
		directory_uri = g_strconcat (gfl->base_dir, "/", directory_name, NULL);

	g_directory_list_async (directory_uri,
			   	gfl->base_dir,
				TRUE,
				gfl->cancellable,
			   	get_items_for_current_dir_done,
			   	gfl);

	g_free (directory_uri);

	return gfl->handle;
}


void
g_list_items_async (GList            *items,
		    const char       *base_dir,
		    GCancellable     *cancellable,
		    VisitDirDoneFunc  done_func,
		    gpointer          done_data)
{
	GetFileListData *gfl;
	int              base_len;
	GList           *scan;

	g_return_val_if_fail (base_dir != NULL, NULL);

	gfl = g_new0 (GetFileListData, 1);
	gfl->base_dir = g_strdup (base_dir);
	gfl->cancellable = cancellable;
	gfl->done_func = done_func;
	gfl->done_data = done_data;

	base_len = 0;
	if (strcmp (base_dir, "/") != 0)
		base_len = strlen (base_dir);

	for (scan = items; scan; scan = scan->next) {
		char *path = scan->data;

		/* FIXME: this is not async */
		if (path_is_dir (path))
			gfl->to_visit = g_list_prepend (gfl->to_visit, g_strdup (path));
		else {
			char *rel_path = g_strdup (path + base_len + 1);
			gfl->files = g_list_prepend (gfl->files, rel_path);
		}
	}
		
	get_items_for_current_dir (gfl);
}


/* -- g_copy_files_async -- */


typedef struct {
	GList                 *sources;
	GList                 *destinations;
	GFileCopyFlags         flags;
	int                    io_priority;
	GCancellable          *cancellable;
	FilesProgressCallback  progress_callback;
	gpointer               progress_callback_data;
	FilesDoneCallback      callback;
	gpointer               user_data;
	
	GList                 *source;
	GList                 *destination;
	int                    n_file;
	int                    tot_files;
} CopyFilesData;


static CopyFilesData*
copy_files_data_new (GList                 *sources,
		     GList                 *destinations,
		     GFileCopyFlags         flags,
		     int                    io_priority,
		     GCancellable          *cancellable,
		     FilesProgressCallback  progress_callback,
		     gpointer               progress_callback_data,
		     FilesDoneCallback      callback,
		     gpointer               user_data)
{
	CopyFilesData *cfd;
	
	cfd = g_new0 (CopyFilesData, 1);
	cfd->sources = gio_file_list_dup (sources);
	cfd->destinations = gio_file_list_dup (destinations);
	cfd->flags = flags;
	cfd->io_priority = io_priority;
	cfd->cancellable = cancellable;
	cfd->progress_callback = progress_callback;
	cfd->progress_callback_data = progress_callback_data;
	cfd->callback = callback;
	cfd->user_data = user_data;
	
	cfd->source = cfd->sources;
	cfd->destination = cfd->destinations;
	cfd->n_file = 1;
	cfd->tot_files = g_list_length (cfd->sources);
	
	return cfd;
}


static void
copy_files_data_free (CopyFilesData *cfd)
{
	if (cfd == NULL)
		return;
	gio_file_list_free (cfd->sources);
	gio_file_list_free (cfd->destinations);
	g_free (cfd);
}


static void g_copy_current_file (CopyFilesData *cfd);


static void
g_copy_next_file (CopyFilesData *cfd)
{
	cfd->source = g_list_next (cfd->source);
	cfd->destination = g_list_next (cfd->destination);
	cfd->n_file++;
	
	g_copy_current_file (cfd);
}


static void
g_copy_files_ready_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
	CopyFilesData *cfd = user_data;
	GFile         *source = cfd->source->data;
	GError        *error;
	
	if (! g_file_copy_finish (source, result, &error)) {
		if (cfd->callback)
			cfd->callback (error, cfd->user_data);
		g_clear_error (&error);
		copy_files_data_free (cfd);
		return;
	}
	
	g_copy_next_file (cfd);
}


static void
g_copy_files_progess_cb (goffset  current_num_bytes,
                         goffset  total_num_bytes,
                         gpointer user_data)
{
	CopyFilesData *cfd = user_data;
	
	if (cfd->progress_callback)
		cfd->progress_callback (cfd->n_file,
					cfd->tot_files,
					(GFile*) cfd->source->data,
					(GFile*) cfd->destination->data,
					current_num_bytes,
					total_num_bytes,
					cfd->progress_callback_data);
}


static void
g_copy_current_file (CopyFilesData *cfd)
{
	if ((cfd->source == NULL) || (cfd->destination == NULL)) {
		if (cfd->callback)
			cfd->callback (NULL, cfd->user_data);
		copy_files_data_free (cfd);
		return;
	}
	
	g_file_copy_async ((GFile*) cfd->source->data,
			   (GFile*) cfd->destination->data,
			   cfd->flags,
			   cfd->io_priority,
			   cfd->cancellable,
			   g_copy_files_progess_cb,
			   cfd,
			   g_copy_files_ready_cb,
			   cfd);
}


void
g_copy_files_async (GList                 *sources,
		    GList                 *destinations,
		    GFileCopyFlags         flags,
		    int                    io_priority,
		    GCancellable          *cancellable,
		    FilesProgressCallback  progress_callback,
		    gpointer               progress_callback_data,
		    FilesDoneCallback      callback,
		    gpointer               user_data)
{
	CopyFilesData *cfd;
	
	cfd = copy_files_data_new (sources, 
				   destinations, 
				   flags, 
				   io_priority, 
				   cancellable, 
				   progress_callback, 
				   progress_callback_data, 
				   callback, 
				   user_data);
	g_copy_current_file (cfd);
}


void
g_copy_uris_async (GList                 *sources,
		   GList                 *destinations,
		   GFileCopyFlags         flags,
		   int                    io_priority,
		   GCancellable          *cancellable,
		   FilesProgressCallback  progress_callback,
		   gpointer               progress_callback_data,
		   FilesDoneCallback      callback,
		   gpointer               user_data)
{
	GList *source_files, *destination_files;
	
	source_files = gio_file_list_new_from_uri_list (sources);
	destination_files = gio_file_list_new_from_uri_list (destinations);
	
	g_copy_files_async (source_files, 
			    destination_files, 
			    flags, 
			    io_priority, 
			    cancellable, 
			    progress_callback, 
			    progress_callback_data, 
			    callback, 
			    user_data);
	
	gio_file_list_free (source_files);
	gio_file_list_free (destination_files);
}


/* -- g_directory_copy_async -- */


typedef struct {
	char      *uri;
	GFileType  type;
} ChildData;


static ChildData*
child_data_new (const char *uri,
		GFileType   type)
{
	ChildData *data;
	
	data = g_new0 (ChildData, 1);
	data->uri = g_strdup (uri);
	data->type = type;
	
	return data;
}


static void
child_data_free (ChildData *child)
{
	if (child == NULL)
		return;
	g_free (child->uri);
	g_free (child);
}


typedef struct {
	GFile                 *source;
	GFile                 *destination;
	GFileCopyFlags         flags;
	int                    io_priority;
	GCancellable          *cancellable;
	CopyProgressCallback   progress_callback;
	gpointer               progress_callback_data;
	CopyDoneCallback       callback;
	gpointer               user_data;
	GError                *error;
	
	char                  *source_uri;
	char                  *destination_uri;
	GList                 *to_copy;
	GList                 *current;
	GFile                 *current_source;
	GFile                 *current_destination;
	int                    n_file, tot_files;
} DirectoryCopyData;


static void
directory_copy_data_free (DirectoryCopyData *dcd)
{
	if (dcd == NULL)
		return;
	g_object_unref (dcd->source);
	g_object_unref (dcd->destination);
	g_object_unref (dcd->cancellable);
	g_free (dcd->source_uri);
	g_free (dcd->destination_uri);
	if (dcd->current_source != NULL) {
		g_object_unref (dcd->current_source);
		dcd->current_source = NULL;
	}
	if (dcd->current_destination != NULL) {
		g_object_unref (dcd->current_destination);
		dcd->current_destination = NULL;
	}
	g_list_foreach (dcd->to_copy, (GFunc) child_data_free, NULL);
	g_list_free (dcd->to_copy);
	g_free (dcd);
}


static void 
g_directory_copy_done (DirectoryCopyData *dcd,
		       GError            *error)
{
	if (dcd->callback)
		dcd->callback (error, dcd->user_data);
	if (error != NULL)
		g_clear_error (&error);
	directory_copy_data_free (dcd);
}


static GFile *
get_destination_for_uri (DirectoryCopyData *dcd, 
		         const char        *uri)
{	
	char  *uri;
	GFile *file;

	uri = g_strconcat (dcd->destination_uri, "/", uri + strlen (dcd->source_uri) + 1, NULL);
	file = g_file_new_for_uri (uri);
	g_free (uri);

	return file;
}


static void g_directory_copy_current_child (DirectoryCopyData *dcd);


static void
g_directory_copy_child_done_cb (GObject      *source_object,
                        	GAsyncResult *result,
 	                        gpointer      user_data)
{
	DirectoryCopyData *dcd = user_data;

	if (! g_file_copy_finish ((GFile*)source_object, result, &(dcd->error))) {
		g_directory_copy_done (dcd, dcd->error);
		return;
	}

	dcd->current = g_list_next (dcd->current);
	dcd->n_file++;
	g_directory_copy_current_child (dcd);
}


static void
g_directory_copy_child_progess_cb (goffset  current_num_bytes,
                           	   goffset  total_num_bytes,
                           	   gpointer user_data)
{
	DirectoryCopyData *dcd = user_data;
	
	if (dcd->progress_callback)
		dcd->progress_callback (dcd->n_file,
					dcd->tot_files,
					dcd->current_source,
					dcd->current_destination,
					current_num_bytes,
					total_num_bytes,
					dcd->progress_callback_data);
}


static void
g_directory_copy_current_child (DirectoryCopyData *dcd)
{
	ChildData *child;
	GFile     *source, *destination;

	if (dcd->current == NULL) {
		g_directory_copy_done (dcd, NULL);
		return;
	}

	if (dcd->current_source != NULL) {
		g_object_unref (dcd->current_source);
		dcd->current_source = NULL;
	}
	if (dcd->current_destination != NULL) {
		g_object_unref (dcd->current_destination);
		dcd->current_destination = NULL;
	}

	child = dcd->current->data;
	dcd->current_destination = get_destination_for_uri (dcd, child->uri);
	switch (child->type) {
	case G_FILE_TYPE_DIRECTORY:	
		/* FIXME: how to make a directory asynchronously ? */
		if (! g_file_make_directory (destination, dcd->cancellable, &(dcd->error))) {
			g_directory_copy_done (dcd, dcd->error);
			return;
		}
		break;
	case G_FILE_TYPE_REGULAR:
		dcd->current_source = g_file_new_for_uri (child->uri);
		g_file_copy_async (source,
				   destination,
				   dcd->flags,
				   dcd->io_priority,
				   dcd->cancellable,
				   g_directory_copy_child_progess_cb,
				   dcd,
				   g_directory_copy_child_done_cb,
				   dcd);
		g_object_unref (source);
		break;
	default:
		break;
	}
}


static void
g_directory_copy_list_ready (GError   *error,
			     gpointer  user_data)
{
	DirectoryCopyData *dcd = user_data;

	if (error != NULL) {
		g_directory_copy_done (dcd, error);
		return;
	}

	dcd->to_copy = g_list_reverse (dcd->to_copy);
	dcd->current = dcd->to_copy;
	dcd->n_file = 1;
	g_directory_copy_current_child (dcd);
}


static void
g_directory_copy_for_each_file (const char *uri, 
				GFileInfo  *info, 
				gpointer    user_data)
{
	dcd->to_copy = g_list_prepend (dcd->to_copy, child_data_new (uri, g_file_info_get_file_type (info)));
	dcd->tot_files++;
}


static gboolean
g_directory_copy_start_dir (const char  *uri, 
			    GError     **error,
			    gpointer     user_data)
{	
	dcd->to_copy = g_list_prepend (dcd->to_copy, child_data_new (uri, G_FILE_TYPE_DIRECTORY));
	return TRUE;
}


void
g_directory_copy_async (GFile                 *source,
		   	GFile                 *destination,
			GFileCopyFlags         flags,
			int                    io_priority,
			GCancellable          *cancellable,
			FilesProgressCallback  progress_callback,
			gpointer               progress_callback_data,
			FilesDoneCallback      callback,
			gpointer               user_data)
{
	DirectoryCopyData *dcd;
	
	dcd = g_new0 (DirectoryCopyData, 1);
	dcd->source = g_file_dup (source);
	dcd->destination = g_file_dup (destination);
	dcd->flags = flags;
	dcd->io_priority = io_priority;
	dcd->cancellable = cancellable;
	dcd->progress_callback = progress_callback;
	dcd->progress_callback_data = progress_callback_data;
	dcd->callback = callback;
	dcd->user_data = user_data;
		
	dcd->source_uri = g_file_get_uri (dcd->source);
	dcd->destination_uri = g_file_get_uri (dcd->destination);	
	
	dcd->to_copy = g_list_prepend (NULL, child_data_new (dcd->source_uri, G_FILE_TYPE_DIRECTORY));
	dcd->n_nodes = 1;
	
	for_each_children (dcd->source,
			   TRUE,
			   TRUE,
			   cancellable,
			   g_directory_copy_start_dir,
			   g_directory_copy_for_each_file,
			   g_directory_copy_list_ready,
			   dcd);
}

