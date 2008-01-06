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

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>


static void
count_selected (GtkTreeModel *model,
		GtkTreePath  *path,
		GtkTreeIter  *iter,
		gpointer      data)
{
	int *n = data;
	*n = *n + 1;
}


int
_gtk_count_selected (GtkTreeSelection *selection)
{
	int n = 0;

	if (selection == NULL)
		return 0;
	gtk_tree_selection_selected_foreach (selection, count_selected, &n);
	return n;
}


GtkWidget*
_gtk_message_dialog_new (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *stock_id,
			 const char       *message,
			 const char       *secondary_message,
			 const gchar      *first_button_text,
			 ...)
{
	GtkWidget    *dialog;
	GtkWidget    *label;
	GtkWidget    *image;
	GtkWidget    *hbox;
	va_list       args;
	const gchar  *text;
	int           response_id;
	char         *escaped_message, *markup_text;

	if (stock_id == NULL)
		stock_id = GTK_STOCK_DIALOG_INFO;

	dialog = gtk_dialog_new_with_buttons ("", parent, flags, NULL);

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new ("");

	escaped_message = g_markup_escape_text (message, -1);
	if (secondary_message != NULL) {
		char *escaped_secondary_message = g_markup_escape_text (secondary_message, -1);
		markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
					       escaped_message,
					       escaped_secondary_message);
		g_free (escaped_secondary_message);
	} else
		markup_text = g_strdup (escaped_message);
	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	g_free (markup_text);
	g_free (escaped_message);

	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 24);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add buttons */

	if (first_button_text == NULL)
		return dialog;

	va_start (args, first_button_text);

	text = first_button_text;
	response_id = va_arg (args, gint);

	while (text != NULL) {
		gtk_dialog_add_button (GTK_DIALOG (dialog), text, response_id);

		text = va_arg (args, char*);
		if (text == NULL)
			break;
		response_id = va_arg (args, int);
	}

	va_end (args);

	return dialog;
}


static GtkWidget *
create_button (const char *stock_id,
	       const char *text)
{
	GtkWidget    *button;
	GtkWidget    *hbox;
	GtkWidget    *image;
	GtkWidget    *label;
	GtkWidget    *align;
	const char   *label_text;
	gboolean      text_is_stock;
	GtkStockItem  stock_item;

	button = gtk_button_new ();

	if (gtk_stock_lookup (text, &stock_item)) {
		label_text = stock_item.label;
		text_is_stock = TRUE;
	} else {
		label_text = text;
		text_is_stock = FALSE;
	}

	if (text_is_stock)
		image = gtk_image_new_from_stock (text, GTK_ICON_SIZE_BUTTON);
	else
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
	label = gtk_label_new_with_mnemonic (label_text);
	hbox = gtk_hbox_new (FALSE, 2);
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), align);
	gtk_container_add (GTK_CONTAINER (align), hbox);

	gtk_widget_show_all (button);

	return button;
}


char *
_gtk_request_dialog_run (GtkWindow        *parent,
			 GtkDialogFlags    flags,
			 const char       *title,
			 const char       *message,
			 const char       *default_value,
			 int               max_length,
			 const gchar      *no_button_text,
			 const gchar      *yes_button_text)
{
	GtkWidget    *dialog;
	GtkWidget    *label;
	GtkWidget    *image;
	GtkWidget    *hbox;
	GtkWidget    *vbox;
	GtkWidget    *entry;
	GtkWidget    *button;
	char         *stock_id;
	char         *result;

	stock_id = GTK_STOCK_DIALOG_QUESTION;

	dialog = gtk_dialog_new_with_buttons (title, parent, flags, NULL);

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), FALSE);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

	entry = gtk_entry_new ();
	gtk_widget_set_size_request (entry, 250, -1);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_entry_set_max_length (GTK_ENTRY (entry), max_length);
	gtk_entry_set_text (GTK_ENTRY (entry), default_value);

	hbox = gtk_hbox_new (FALSE, 24);
	vbox = gtk_vbox_new (FALSE, 6);

	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), vbox,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), label,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), entry,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add buttons */

	button = create_button (GTK_STOCK_CANCEL, no_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button, 
				      GTK_RESPONSE_CANCEL);

	button = create_button (GTK_STOCK_OK, yes_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button,
				      GTK_RESPONSE_YES);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

	/* Run dialog */

	gtk_widget_grab_focus (entry);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
		result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	else
		result = NULL;

	gtk_widget_destroy (dialog);

	return result;
}


GtkWidget*
_gtk_yesno_dialog_new (GtkWindow        *parent,
		       GtkDialogFlags    flags,
		       const char       *message,
		       const char       *no_button_text,
		       const char       *yes_button_text)
{
	GtkWidget    *d;
	GtkWidget    *label;
	GtkWidget    *image;
	GtkWidget    *hbox;
	GtkWidget    *button;
	char         *stock_id = GTK_STOCK_DIALOG_WARNING;

	d = gtk_dialog_new_with_buttons ("", parent, flags, NULL);
	gtk_window_set_resizable (GTK_WINDOW (d), FALSE);

	gtk_dialog_set_has_separator (GTK_DIALOG (d), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (d), 6);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (d)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (d)->vbox), 8);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new (message);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	hbox = gtk_hbox_new (FALSE, 24);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), label,
			    TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (d)->vbox),
			    hbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	/* Add buttons */

	button = create_button (GTK_STOCK_CANCEL, no_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button, 
				      GTK_RESPONSE_CANCEL);

	/**/

	button = create_button (GTK_STOCK_OK, yes_button_text);
	gtk_dialog_add_action_widget (GTK_DIALOG (d),
				      button, 
				      GTK_RESPONSE_YES);

	/**/

	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);

	return d;
}


static void
toggle_visibility (GtkWidget *widget)
{
	if (GTK_WIDGET_VISIBLE (widget))
		gtk_widget_hide (widget);
	else
		gtk_widget_show (widget);
}


GtkWidget*
_gtk_error_dialog_new (GtkWindow        *parent,
		       GtkDialogFlags    flags,
		       GList            *row_output,
		       const char       *primary_text,
		       const char       *secondary_text,
		       ...)
{
	GtkWidget     *dialog;
	GtkWidget     *label;
	GtkWidget     *image;
	GtkWidget     *hbox;
	GtkWidget     *vbox;
	GtkWidget     *text_view;
	GtkWidget     *scrolled = NULL;
	GtkWidget     *button;
	GtkTextBuffer *text_buf;
	GtkTextIter    iter;
	char          *stock_id;
	GList         *scan;
	char          *escaped_message, *markup_text;
	va_list        args;
	gboolean       view_output = (row_output != NULL);

	stock_id = GTK_STOCK_DIALOG_ERROR;

	dialog = gtk_dialog_new_with_buttons ("",
					      parent, 
					      flags,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 8);

	gtk_widget_set_size_request (dialog, 500, -1);

	/* Add label and image */

	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);

	label = gtk_label_new ("");
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);

	escaped_message = g_markup_escape_text (primary_text, -1);
	if (secondary_text != NULL) {
		char *secondary_message;
		char *escaped_secondary_message;

		va_start (args, secondary_text);
		secondary_message = g_strdup_vprintf (secondary_text, args);
		va_end (args);
		escaped_secondary_message = g_markup_escape_text (secondary_message, -1);

		markup_text = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",
					       escaped_message,
					       escaped_secondary_message);

		g_free (escaped_secondary_message);
		g_free (secondary_message);
	}
	else
		markup_text = g_strdup (escaped_message);
	gtk_label_set_markup (GTK_LABEL (label), markup_text);
	g_free (markup_text);
	g_free (escaped_message);

	if (view_output) {
		/* Button */

		button = gtk_toggle_button_new_with_mnemonic (_("Command _Line Output"));

		/* Add text */

		scrolled = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
						     GTK_SHADOW_ETCHED_IN);
		gtk_widget_set_size_request (scrolled, -1, 200);

		text_buf = gtk_text_buffer_new (NULL);
		gtk_text_buffer_create_tag (text_buf, "monospace",
					    "family", "monospace", NULL);
		gtk_text_buffer_get_iter_at_offset (text_buf, &iter, 0);
		for (scan = row_output; scan; scan = scan->next) {
			char *line = scan->data;
			char *utf8_line;
			gsize bytes_written;

			utf8_line = g_locale_to_utf8 (line, -1, NULL, &bytes_written, NULL);
			gtk_text_buffer_insert_with_tags_by_name (text_buf,
								  &iter,
								  utf8_line,
								  bytes_written,
								  "monospace", NULL);
			g_free (utf8_line);

			gtk_text_buffer_insert (text_buf, &iter, "\n", 1);
		}
		text_view = gtk_text_view_new_with_buffer (text_buf);
		g_object_unref (text_buf);
		gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
		gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (text_view), FALSE);
	}

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), image,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label,
			    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox,
			    TRUE, TRUE, 0);

	if (view_output) {
		hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (hbox), button,
			    FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox,
				    TRUE, TRUE, 0);

		gtk_container_add (GTK_CONTAINER (scrolled), text_view);
		gtk_box_pack_start (GTK_BOX (vbox), scrolled,
				    FALSE, FALSE, 0);

		g_signal_connect_swapped (G_OBJECT (button),
					  "clicked", 
					  G_CALLBACK (toggle_visibility), 
					  scrolled);				    
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			    vbox,
			    FALSE, FALSE, 0);

	gtk_widget_show_all (vbox);

	if (scrolled != NULL)
		gtk_widget_hide (scrolled);

	return dialog;
}


void
_gtk_error_dialog_run (GtkWindow        *parent,
		       const gchar      *main_message,
		       const gchar      *format,
		       ...)
{
	GtkWidget *d;
	char      *message;
	va_list    args;

	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);

	d =  _gtk_message_dialog_new (parent,
				      GTK_DIALOG_MODAL,
				      GTK_STOCK_DIALOG_ERROR,
				      main_message,
				      message,
				      GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
				      NULL);
	g_free (message);

	g_signal_connect (G_OBJECT (d), "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (d);
}


void
_gtk_entry_set_locale_text (GtkEntry   *entry,
			    const char *text)
{
	char *utf8_text;

	if (text == NULL)
		return;

	utf8_text = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
	if (utf8_text != NULL)
		gtk_entry_set_text (entry, utf8_text);
	else
		gtk_entry_set_text (entry, "");
	g_free (utf8_text);
}


char *
_gtk_entry_get_locale_text (GtkEntry *entry)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_entry_get_text (entry);
	if (utf8_text == NULL)
		return NULL;

	text = g_locale_from_utf8 (utf8_text, -1, NULL, NULL, NULL);

	return text;
}


void
_gtk_label_set_locale_text (GtkLabel   *label,
			    const char *text)
{
	char *utf8_text;

	utf8_text = g_locale_to_utf8 (text, -1, NULL, NULL, NULL);
	if (utf8_text != NULL) {
		gtk_label_set_text (label, utf8_text);
		g_free (utf8_text);
	} else
		gtk_label_set_text (label, "");
}


char *
_gtk_label_get_locale_text (GtkLabel *label)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_label_get_text (label);
	if (utf8_text == NULL)
		return NULL;

	text = g_locale_from_utf8 (utf8_text, -1, NULL, NULL, NULL);

	return text;
}


void
_gtk_entry_set_filename_text (GtkEntry   *entry,
			      const char *text)
{
	char *utf8_text;

	utf8_text = g_filename_to_utf8 (text, -1, NULL, NULL, NULL);
	if (utf8_text != NULL) {
		gtk_entry_set_text (entry, utf8_text);
		g_free (utf8_text);
	} else
		gtk_entry_set_text (entry, "");
}


char *
_gtk_entry_get_filename_text (GtkEntry   *entry)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_entry_get_text (entry);
	if (utf8_text == NULL)
		return NULL;

	text = g_filename_from_utf8 (utf8_text, -1, NULL, NULL, NULL);

	return text;
}


void
_gtk_label_set_filename_text (GtkLabel   *label,
			      const char *text)
{
	char *utf8_text;

	utf8_text = g_filename_display_name (text);
	gtk_label_set_text (label, utf8_text);
	g_free (utf8_text);
}


char *
_gtk_label_get_filename_text (GtkLabel   *label)
{
	const char *utf8_text;
	char       *text;

	utf8_text = gtk_label_get_text (label);
	if (utf8_text == NULL)
		return NULL;

	text = g_filename_from_utf8 (utf8_text, -1, NULL, NULL, NULL);

	return text;
}

/* This function from gnome-panel/panel-util.c
 * (C) 1997, 1998, 1999, 2000 The Free Software Foundation
 * Copyright 2000 Helix Code, Inc.
 * Copyright 2000,2001 Eazel, Inc.
 * Copyright 2001 George Lebl
 * Copyright 2002 Sun Microsystems Inc.
 *
 * Authors: George Lebl
 *          Jacob Berkman
 *          Mark McLoughlin
 *
 * Modified by Paolo Bacchilega for the Quick Lounge applet
 */
static char*
panel_find_icon (GtkIconTheme  *icon_theme,
		 const char    *icon_name,
		 gint           size)
{
	char        *retval  = NULL;
	GtkIconInfo *icon_info = NULL;
	char        *icon_no_extension;
	char        *p;


	if (icon_name == NULL || strcmp (icon_name, "") == 0)
		return NULL;

	if (g_path_is_absolute (icon_name)) {
		if (g_file_test (icon_name, G_FILE_TEST_EXISTS)) {
			return g_strdup (icon_name);
		} else {
			char *basename;

			basename = g_path_get_basename (icon_name);
			retval = panel_find_icon (icon_theme,
						  basename,
						  size);
			g_free (basename);

			return retval;
		}
	}

	/* This is needed because some .desktop files have an icon name *and*
	 * an extension as icon */
	icon_no_extension = g_strdup (icon_name);
	p = strrchr (icon_no_extension, '.');
	if (p &&
	    (strcmp (p, ".png") == 0 ||
	     strcmp (p, ".xpm") == 0 ||
	     strcmp (p, ".svg") == 0)) {
	    *p = 0;
	}

	icon_info = gtk_icon_theme_lookup_icon (icon_theme,
						icon_no_extension,
						size,
						0);
	if (icon_info != NULL) {
		retval = g_strdup (gtk_icon_info_get_filename (icon_info));
		gtk_icon_info_free (icon_info);
	}
	
	g_free (icon_no_extension);

	return retval;
}


GdkPixbuf *
create_pixbuf (GtkIconTheme  *icon_theme,
	       const char    *icon_name,
	       int            icon_size)
{
	char      *icon_path;
	GdkPixbuf *pixbuf;
	int        iw;
	int        ih;

	icon_path = panel_find_icon (icon_theme, icon_name, icon_size);
	if (icon_path == NULL)
		return NULL;

	pixbuf = gdk_pixbuf_new_from_file (icon_path, NULL);
	g_free (icon_path);

	if (pixbuf == NULL)
		return NULL;

	iw = gdk_pixbuf_get_width (pixbuf);
	ih = gdk_pixbuf_get_height (pixbuf);

	if ((iw > icon_size) || (ih > icon_size)) {
		GdkPixbuf *scaled;
		gdouble    factor;
		gdouble    max = icon_size;
		int        new_w, new_h;

		factor = MIN (max / iw, max / ih);
		new_w  = MAX ((int) (iw * factor), 1);
		new_h = MAX ((int) (ih * factor), 1);

		scaled = gdk_pixbuf_scale_simple (pixbuf,
						  new_w,
						  new_h,
						  GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled;
	}

	return pixbuf;
}


int
get_folder_pixbuf_size_for_list (GtkWidget *widget)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (widget),
					   GTK_ICON_SIZE_SMALL_TOOLBAR,
					   &icon_width, &icon_height);
	return MAX (icon_width, icon_height);
}


void
show_help_dialog (GtkWindow  *parent,
		  const char *link_id)
{
	GError *err = NULL;
	char *command;
	const char *lang;
	char *uri = NULL;
	int i;
	GdkScreen *gscreen;

	const char * const * langs = g_get_language_names ();

	for (i = 0; langs[i]; i++) {
		lang = langs[i];
		if (strchr (lang, '.')) {
			continue;
		}

		uri = g_build_filename(FR_DATADIR,
				       "/gnome/help/file-roller/",
					lang,
				       "/file-roller.xml",
					NULL);
					
		if (g_file_test (uri, G_FILE_TEST_EXISTS)) {
                    break;
		}
	}
	
	if (link_id) {
		command = g_strconcat ("gnome-open ghelp://", uri, "?", link_id, NULL);
	} else {
		command = g_strconcat ("gnome-open ghelp://", uri,  NULL);
	}

	gscreen = gdk_screen_get_default();
	gdk_spawn_command_line_on_screen (gscreen, command, &err);

	if (err != NULL) {
		GtkWidget *dialog;

		dialog = _gtk_message_dialog_new (parent,
						  GTK_DIALOG_DESTROY_WITH_PARENT, 
						  GTK_STOCK_DIALOG_ERROR,
						  _("Could not display help"),
						  err->message,
						  GTK_STOCK_OK, GTK_RESPONSE_OK,
						  NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);

		g_error_free (err);
	}
}
