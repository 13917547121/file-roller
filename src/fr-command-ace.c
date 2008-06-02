/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-ace.h"

static void fr_command_ace_class_init  (FrCommandAceClass *class);
static void fr_command_ace_init        (FrCommand        *afile);
static void fr_command_ace_finalize    (GObject          *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */


static time_t
mktime_from_string (char *date,
		    char *time)
{
	struct tm    tm = {0, };
	char       **fields;

	tm.tm_isdst = -1;

	/* date */

	fields = g_strsplit (date, ".", 3);
	if (fields[0] != NULL) {
		tm.tm_mday = atoi (fields[0]);
		if (fields[1] != NULL) {
			tm.tm_mon = atoi (fields[1]) - 1;
			if (fields[2] != NULL) {
				int y = atoi (fields[2]);
				if (y > 75)
					tm.tm_year = y;
				else
					tm.tm_year = 100 + y;
			}
		}
	}
	g_strfreev (fields);

	/* time */

	fields = g_strsplit (time, ":", 2);
	if (fields[0] != NULL) {
		tm.tm_hour = atoi (fields[0]);
		if (fields[1] != NULL) 
			tm.tm_min = atoi (fields[1]);
	}
	tm.tm_sec = 0;
	g_strfreev (fields);

	return mktime (&tm);
}


static void
process_line (char     *line, 
	      gpointer  data)
{
	FileData      *fdata;
	FrCommandAce  *ace_comm = FR_COMMAND_ACE (data);
	FrCommand     *comm = FR_COMMAND (data);
	char         **fields;
	char          *field_name;

	g_return_if_fail (line != NULL);

	fdata = file_data_new ();

	fields = g_strsplit (line, "|", 6);

	if ((fields == NULL) || (fields[0] == NULL))
		return;

	if (! ace_comm->list_started && (strncmp(fields[0], "Date", 4) == 0)) {
		ace_comm->list_started = TRUE;
		return;
	}

	if (n_fields (fields) != 6) {
		ace_comm->list_started = FALSE;
		return;
	}

	fdata->size = g_ascii_strtoull (fields[3], NULL, 10);
	fdata->modified = mktime_from_string (fields[0], fields[1]);

	field_name = fields[5] + 1;
	if (field_name[0] != '/') {
		fdata->full_path = g_strconcat ("/", field_name, NULL);
		fdata->original_path = fdata->full_path + 1;
	} else {
		fdata->full_path = g_strdup (field_name);
		fdata->original_path = fdata->full_path;
	}

	g_strfreev (fields);

	fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else 
		fr_command_add_file (comm, fdata);
}


static void
fr_command_ace_list (FrCommand  *comm,
		    const char *password)
{
	FR_COMMAND_ACE (comm)->list_started = FALSE;

	fr_process_set_out_line_func (FR_COMMAND (comm)->process, 
				      process_line,
				      comm);

	fr_process_begin_command (comm->process, "unace");
	fr_process_add_arg (comm->process, "v");
	fr_process_add_arg (comm->process, comm->e_filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_ace_extract (FrCommand   *comm,
			GList       *file_list,
			const char *dest_dir,
			gboolean     overwrite,
			gboolean     skip_older,
			gboolean     junk_paths,
			const char *password)
{
	GList *scan;

	fr_process_begin_command (comm->process, "unace");

	if (dest_dir != NULL) 
		fr_process_set_working_dir (comm->process, dest_dir);

	if (junk_paths)
		fr_process_add_arg (comm->process, "e");
	else
		fr_process_add_arg (comm->process, "x");
	fr_process_add_arg (comm->process, comm->e_filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}


static void
fr_command_ace_test (FrCommand   *comm,
                     const char  *password)
{
        fr_process_begin_command (comm->process, "unace");
        fr_process_add_arg (comm->process, "t");
	fr_process_add_arg (comm->process, comm->e_filename);
        fr_process_end_command (comm->process);
}


static void
fr_command_ace_handle_error (FrCommand   *comm, 
			     FrProcError *error)
{
	/* FIXME */
}


static void
fr_command_ace_set_mime_type (FrCommand  *comm,
			      const char *mime_type)
{
	FR_COMMAND_CLASS (parent_class)->set_mime_type (comm, mime_type);
	
	comm->capabilities |= FR_COMMAND_CAP_ARCHIVE_MANY_FILES;
	if (is_program_in_path ("unace")) 
		comm->capabilities |= FR_COMMAND_CAP_READ;
}


static void 
fr_command_ace_class_init (FrCommandAceClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_ace_finalize;

        afc->list           = fr_command_ace_list;
	afc->extract        = fr_command_ace_extract;
	afc->test           = fr_command_ace_test;
	afc->handle_error   = fr_command_ace_handle_error;
	afc->set_mime_type  = fr_command_ace_set_mime_type;
}

 
static void 
fr_command_ace_init (FrCommand *comm)
{
	comm->propCanModify                = FALSE;
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = TRUE;
}


static void 
fr_command_ace_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_ACE (object));
	
	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_ace_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandAceClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_ace_class_init,
			NULL,
			NULL,
			sizeof (FrCommandAce),
			0,
			(GInstanceInitFunc) fr_command_ace_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandAce",
					       &type_info,
					       0);
        }

        return type;
}
