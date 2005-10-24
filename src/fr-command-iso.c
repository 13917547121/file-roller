/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2004 The Free Software Foundation, Inc.
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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-mime.h>

#include "file-data.h"
#include "file-utils.h"
#include "fr-command.h"
#include "fr-command-iso.h"

static void fr_command_iso_class_init  (FRCommandIsoClass *class);
static void fr_command_iso_init           (FRCommand         *afile);
static void fr_command_iso_finalize     (GObject           *object);

/* Parent Class */

static FRCommandClass *parent_class = NULL;


static time_t
mktime_from_string (char *month, 
		    char *mday,
		    char *year)
{
	static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	struct tm     tm = {0, };

	if (month != NULL) {
		int i;
		for (i = 0; i < 12; i++)
			if (strcmp (months[i], month) == 0) {
				tm.tm_mon = i;
				break;
			}
	}
	tm.tm_mday = atoi (mday);
	tm.tm_year = atoi (year) - 1900;

	return mktime (&tm);
}


static void
list__process_line (char     *line, 
		    gpointer  data)
{
	FileData      *fdata;
	FRCommand     *comm = FR_COMMAND (data);
	FRCommandIso  *comm_iso = FR_COMMAND_ISO (comm);
	char         **fields;
	const char    *name_field;
	
	g_return_if_fail (line != NULL);

	if (line[0] == 'd') /* Ignore directories. */
		return;

	if (line[0] == 'D') {
		if (comm_iso->cur_path != NULL)
			g_free (comm_iso->cur_path);
		comm_iso->cur_path = g_strdup (get_last_field (line, 4));
		
	} else if (line[0] == '-') { /* Is file */
		const char *last_field, *first_bracket;

		fdata = file_data_new ();
		
		fields = split_line (line, 8);
		fdata->size = g_ascii_strtoull (fields[4], NULL, 10);
		fdata->modified = mktime_from_string (fields[5], fields[6], fields[7]);
		g_strfreev (fields);
		
		/* Full path */

		last_field = get_last_field (line, 9);
		first_bracket = strchr (last_field, ']');
		if (first_bracket == NULL) {
			file_data_free (fdata);
			return;
		}
			
		name_field = eat_spaces (first_bracket + 1);
		if ((name_field == NULL) 
		    || (strcmp (name_field, ".") == 0)
		    || (strcmp (name_field, "..") == 0)) {
			file_data_free (fdata);
			return;
		}

		fdata->original_path = fdata->full_path = g_strstrip (g_strconcat (comm_iso->cur_path, name_field, NULL));
		fdata->name = g_strdup (file_name_from_path (fdata->full_path));
		fdata->path = remove_level_from_path (fdata->full_path);
		fdata->type = gnome_vfs_mime_type_from_name_or_default (fdata->name, GNOME_VFS_MIME_TYPE_UNKNOWN);
		comm->file_list = g_list_prepend (comm->file_list, fdata);
	}
}


static void
fr_command_iso_list (FRCommand *comm)
{
	fr_process_set_out_line_func (FR_COMMAND (comm)->process, 
				      list__process_line,
				      comm);

	fr_process_begin_command (comm->process, "isoinfo");
	fr_process_add_arg (comm->process, "-J -R");
	fr_process_add_arg (comm->process, "-l -i");
	fr_process_add_arg (comm->process, comm->e_filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_iso_extract (FRCommand  *comm,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths,
			const char *password)
{
	char  *e_dest_dir = fr_command_escape (comm, dest_dir);
	GList *scan;

	for (scan = file_list; scan; scan = scan->next) {
		char       *path = scan->data;
		const char *filename;
                char       *file_dir, *e_temp_dest_dir = NULL, *temp_dest_dir = NULL;
	
		filename = file_name_from_path (path);
		file_dir = remove_level_from_path (path);
		if ((file_dir != NULL) && (strcmp (file_dir, "/") != 0)) 
			e_temp_dest_dir = g_build_filename (e_dest_dir, file_dir, NULL);
		 else 
			e_temp_dest_dir = g_strdup (e_dest_dir);
		g_free (file_dir);
		if (e_temp_dest_dir == NULL) 
			continue;

		temp_dest_dir = unescape_str (e_temp_dest_dir);
		ensure_dir_exists (temp_dest_dir, 0700);

		fr_process_begin_command (comm->process, "isoinfo");
		fr_process_set_working_dir (comm->process, temp_dest_dir);
		fr_process_add_arg (comm->process, "-J -R");
		fr_process_add_arg (comm->process, "-i");
		fr_process_add_arg (comm->process, comm->e_filename);
		fr_process_add_arg (comm->process, "-x");
		fr_process_add_arg (comm->process, path);
		fr_process_add_arg (comm->process, ">");
		fr_process_add_arg (comm->process, filename);
		fr_process_end_command (comm->process);
	
		g_free (e_temp_dest_dir);
		g_free (temp_dest_dir);
	}

	g_free (e_dest_dir);

	fr_process_start (comm->process);
}


static void
fr_command_iso_handle_error (FRCommand   *comm, 
			     FRProcError *error)
{
	if (error->type == 2) { /* ERROR: Unable to find Joliet SVD */

		/* Remove the -J -R options and start again */
		fr_process_set_arg_at (comm->process, 
				       comm->process->error_command, 
				       1, 
				       "");
		comm->process->restart = TRUE;
	}
}


static void 
fr_command_iso_class_init (FRCommandIsoClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FRCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FRCommandClass*) class;

	gobject_class->finalize = fr_command_iso_finalize;

        afc->list           = fr_command_iso_list;
	afc->extract        = fr_command_iso_extract;
	afc->handle_error   = fr_command_iso_handle_error;
}


static void 
fr_command_iso_init (FRCommand *comm)
{
	FRCommandIso *comm_iso = FR_COMMAND_ISO (comm);

	comm_iso->cur_path = NULL;
	
	comm->propCanModify                = FALSE;
	comm->propAddCanUpdate             = FALSE;
	comm->propAddCanReplace            = FALSE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = FALSE;
	comm->propCanExtractAll            = FALSE;
}


static void 
fr_command_iso_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_ISO (object));

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_iso_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FRCommandIsoClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_iso_class_init,
			NULL,
			NULL,
			sizeof (FRCommandIso),
			0,
			(GInstanceInitFunc) fr_command_iso_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandIso",
					       &type_info,
					       0);
        }

        return type;
}


FRCommand *
fr_command_iso_new (FRProcess  *process,
		    const char *filename)
{
	FRCommand *comm;

	if (!is_program_in_path("isoinfo")) {
		return NULL;
	}

	comm = FR_COMMAND (g_object_new (FR_TYPE_COMMAND_ISO, NULL));
	fr_command_construct (comm, process, filename);

	return comm;
}
