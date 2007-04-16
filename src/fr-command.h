/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#ifndef FR_COMMAND_H
#define FR_COMMAND_H

#include <glib.h>
#include "file-data.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND            (fr_command_get_type ())
#define FR_COMMAND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND, FrCommand))
#define FR_COMMAND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND, FrCommandClass))
#define FR_IS_COMMAND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND))
#define FR_IS_COMMAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND))
#define FR_COMMAND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND, FrCommandClass))

typedef struct _FrCommand       FrCommand;
typedef struct _FrCommandClass  FrCommandClass;

typedef enum {
	FR_ACTION_NONE,
	FR_ACTION_CREATING_NEW_ARCHIVE,
	FR_ACTION_LOADING_ARCHIVE,            /* loading the archive from a remote location */
	FR_ACTION_LISTING_CONTENT,            /* listing the content of the archive */
	FR_ACTION_DELETING_FILES,             /* deleting files from the archive */
	FR_ACTION_TESTING_ARCHIVE,            /* testing the archive integrity */
	FR_ACTION_GETTING_FILE_LIST,          /* getting the file list (when fr_archive_add_with_wildcard or
						 fr_archive_add_directory are used, we need to scan a directory
						 and collect the files to add to the archive, this
						 may require some time to complete, so the operation
						 is asynchronous) */
	FR_ACTION_COPYING_FILES_FROM_REMOTE,  /* copying files from a remote location */
	FR_ACTION_ADDING_FILES,               /* adding files to an archive */
	FR_ACTION_EXTRACTING_FILES,           /* extracting files */
	FR_ACTION_COPYING_FILES_TO_REMOTE,    /* copying files to a remote location */
	FR_ACTION_CREATING_ARCHIVE,           /* creating the archive */
	FR_ACTION_SAVING_REMOTE_ARCHIVE       /* copying the archive to a remote location */
} FRAction;

#ifdef DEBUG
extern char *action_names[];
#endif

struct _FrCommand
{
	GObject  __parent;

	FRFileType  file_type;
	GList      *file_list;       /* FileData elements */

	/*<protected>*/

	/* properties the command supports. */

	guint propCanModify : 1;
	guint propAddCanUpdate : 1;
	guint propAddCanReplace : 1;
	guint propAddCanStoreFolders : 1;
	guint propExtractCanAvoidOverwrite : 1;
	guint propExtractCanSkipOlder : 1;
	guint propExtractCanJunkPaths : 1;
	guint propPassword : 1;
	guint propTest : 1;
	guint propCanExtractAll : 1;

	/* used by the progress signal */

	int     n_file;
	int     n_files;         /* used by the progress signal */

	/*<private>*/

	FrProcess *process;      /* the process object used to execute
				  * commands. */
	FRAction   action;       /* current action. */
	char      *filename;     /* archive filename. */
	char      *e_filename;   /* escaped archive filename. */

	gboolean   fake_load;    /* if TRUE does nothing when the list
				  * operation is invoked. */
};

struct _FrCommandClass
{
	GObjectClass __parent_class;

	/*<virtual functions>*/

	void        (*list)           (FrCommand     *comm,
				       const char    *password);

	void        (*add)            (FrCommand     *comm,
				       GList         *file_list,
				       const char    *base_dir,
				       gboolean       update,
				       const char    *password,
				       FRCompression  compression); 

	void        (*delete)         (FrCommand     *comm,
				       GList         *file_list); 

	void        (*extract)        (FrCommand     *comm,
				       GList         *file_list,
				       const char    *dest_dir,
				       gboolean       overwrite,
				       gboolean       skip_older,
				       gboolean       junk_paths,
				       const char    *password);

	void        (*test)           (FrCommand     *comm,
				       const char    *password);

	void        (*uncompress)     (FrCommand     *comm);

	void        (*recompress)     (FrCommand     *comm,
				       FRCompression  compression);

	void        (*handle_error)   (FrCommand     *comm,
				       FRProcError   *error);

	char *      (*escape)         (FrCommand     *comm,
				       const char    *str);

	/*<signals>*/

	void        (*start)          (FrCommand   *comm,
				       FRAction     action); 

	void        (*done)           (FrCommand   *comm,
				       FRAction     action,
				       FRProcError *error);

	void        (*progress)       (FrCommand   *comm,
				       double       fraction);

	void        (*message)        (FrCommand   *comm,
				       const char  *msg);
};

GType          fr_command_get_type           (void);

void           fr_command_construct          (FrCommand     *comm,
					      FrProcess     *process,
					      const char    *filename);

void           fr_command_set_filename       (FrCommand     *comm,
					      const char    *filename);

void           fr_command_list               (FrCommand     *comm,
					      const char    *password);

void           fr_command_add                (FrCommand     *comm,
					      GList         *file_list,
					      const char    *base_dir,
					      gboolean       update,
					      const char    *password,
					      FRCompression  compression); 

void           fr_command_delete             (FrCommand     *comm,
					      GList         *file_list); 

void           fr_command_extract            (FrCommand     *comm,
					      GList         *file_list,
					      const char    *dest_dir,
					      gboolean       overwrite,
					      gboolean       skip_older,
					      gboolean       junk_paths,
					      const char    *password);

void           fr_command_test               (FrCommand     *comm,
					      const char    *password);

void           fr_command_uncompress         (FrCommand     *comm);

void           fr_command_recompress         (FrCommand     *comm,
					      FRCompression  compression);

char *         fr_command_escape             (FrCommand     *comm,
					      const char    *str);

/* protected functions */

void           fr_command_progress           (FrCommand     *comm,
					      double         fraction);

void           fr_command_message            (FrCommand     *comm,
					      const char    *msg);

void           fr_command_set_n_files        (FrCommand     *comm,
					      int            n_files);

/* private functions */

void           fr_command_handle_error       (FrCommand     *comm,
					      FRProcError   *error);

#endif /* FR_COMMAND_H */
