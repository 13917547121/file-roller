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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H


#define RC_BOOKMARKS_FILE ".file-roller/bookmarks"
#define RC_RECENT_FILE    ".file-roller/recents"


typedef enum {
	WINDOW_SORT_BY_NAME = 0,
	WINDOW_SORT_BY_SIZE = 1,
	WINDOW_SORT_BY_TYPE = 2,
	WINDOW_SORT_BY_TIME = 3,
	WINDOW_SORT_BY_PATH = 4
} WindowSortMethod;


typedef enum {
	WINDOW_LIST_MODE_FLAT,
	WINDOW_LIST_MODE_AS_DIR
} WindowListMode;


typedef enum {
	FR_COMPRESS_PROGRAM_NONE,
	FR_COMPRESS_PROGRAM_GZIP,
	FR_COMPRESS_PROGRAM_BZIP,
	FR_COMPRESS_PROGRAM_BZIP2,
	FR_COMPRESS_PROGRAM_COMPRESS,
	FR_COMPRESS_PROGRAM_LZOP,
} FRCompressProgram;


typedef enum {
	FR_PROC_ERROR_NONE,
	FR_PROC_ERROR_GENERIC,
	FR_PROC_ERROR_COMMAND_NOT_FOUND,
	FR_PROC_ERROR_EXITED_ABNORMALLY,
	FR_PROC_ERROR_SPAWN,
	FR_PROC_ERROR_STOPPED
} FRProcErrorType;


typedef struct {
	FRProcErrorType  type;
	int              status;
	GError          *gerror;
} FRProcError;


#endif /* TYPEDEFS_H */

