#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <signal.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <hildon/hildon.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#define THEME_DIR "/usr/share/themes"
#define ETC_THEME_SYMLINK "/etc/hildon/theme"

#define GCONF_CPA_STATE "/apps/osso/apps/controlpanel/state"

/* TODO: Constants for all column indexes */
#define COLUMN_ICON 0
#define COLUMN_TRANSLATED_NAME 1
#define COLUMN_READABLE_NAME 2
#define COLUMN_HILDON_GTKTHEME 3
#define COLUMN_HILDON_MATCHBOXTHEME 4
#define COLUMN_HILDON_ADDITIONALRCFILES 6
#define COLUMN_DIRNAME 7

static GtkWidget *static_dialog = NULL;

static gchar *get_current_theme(void);

int stop_progress_indicator()
{

	gtk_widget_set_sensitive(static_dialog, TRUE);
	hildon_gtk_window_set_progress_indicator(GTK_WINDOW(static_dialog),
						 FALSE);

	// TODO: g_signal_handlers_disconnect_matched();

	gtk_dialog_response(GTK_DIALOG(static_dialog), GTK_RESPONSE_OK);
	return 0;
}

void select_current_theme(GtkTreeView * view)
{
	GConfClient *client = gconf_client_get_default();

	GtkTreeModel *treemodel = gtk_tree_view_get_model(view);
	if (!treemodel) {
		g_warning("Failed to initialise GtkTreeModel");
		goto err;
	}

	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter_first(treemodel, &iter)) {
		goto err;
	}

	gchar *saved_theme =
	    gconf_client_get_string(client, GCONF_CPA_STATE, NULL);
	gchar *current_theme = get_current_theme();

	gchar *hildon_gtktheme = NULL;
	gchar *theme_name = NULL;
	gchar *theme_dirname = NULL;
	GtkTreePath *path = NULL;

	while (1) {
		while (1) {
			gtk_tree_model_get(treemodel, &iter,
					   COLUMN_HILDON_GTKTHEME,
					   &hildon_gtktheme, -1);
			gtk_tree_model_get(treemodel, &iter,
					   COLUMN_READABLE_NAME, &theme_name,
					   -1);
			gtk_tree_model_get(treemodel, &iter, COLUMN_DIRNAME,
					   &theme_dirname, -1);

			if (theme_name && theme_dirname)
				break;

			if (!gtk_tree_model_iter_next(treemodel, &iter)) {
				goto done;
			}
		}

		char *translated_name =
		    dcgettext("hildon-control-panel-personalisation",
			      theme_name, LC_MESSAGES);
		char *translated_name_nl =
		    g_strconcat(translated_name, "\n", NULL);

		gtk_list_store_set(GTK_LIST_STORE(treemodel), &iter,
				   COLUMN_TRANSLATED_NAME, translated_name_nl,
				   -1);

		g_free(translated_name_nl);

		if (current_theme && theme_dirname
		    && g_str_equal(current_theme, theme_dirname)) {
			path = gtk_tree_model_get_path(treemodel, &iter);
		}

		g_free(hildon_gtktheme);
		hildon_gtktheme = NULL;
		g_free(theme_name);
		theme_name = NULL;
		g_free(theme_dirname);
		theme_dirname = NULL;

		if (!gtk_tree_model_iter_next(treemodel, &iter)) {
			goto done;
		}
	}

 done:
	g_free(hildon_gtktheme);
	g_free(theme_name);
	g_free(theme_dirname);

	if (path) {
		gtk_tree_view_set_cursor(view, path, FALSE, FALSE);
	} else {
		g_warning("Unable to resolve current theme path");
	}
	gtk_tree_path_free(path);

	g_free(saved_theme);
	g_free(current_theme);

 err:
	g_object_unref(client);
}

void save_theme_setting(GtkTreeView * view)
{
	g_debug("save_theme_setting");
	GtkTreeModel *treemodel = gtk_tree_view_get_model(view);

	if (!treemodel) {
		g_warning("Failed to initialise GtkTreeModel");
		goto err;
	}

	GtkTreeSelection *sel = gtk_tree_view_get_selection(view);
	if (!sel) {
		g_warning("Failed to initialise GtkTreeModel");
		g_warning("Failed to save state");
		return;
	}

	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected(sel, &treemodel, &iter)) {
		goto err;
	}

	GConfClient *client = gconf_client_get_default();
	char *dirname = NULL;
	GError *error = NULL;
	gtk_tree_model_get(treemodel, &iter, COLUMN_DIRNAME, &dirname, -1);

	if (dirname == NULL) {
		g_debug("set_state: state: \"NULL\"");
		gconf_client_set_string(client, GCONF_CPA_STATE, "NULL",
					&error);
	} else {
		g_debug("set_state: state: %s", dirname);
		gconf_client_set_string(client, GCONF_CPA_STATE, dirname,
					&error);
	}

	g_free(dirname);
	g_object_unref(client);

	if (error != NULL) {
		g_warning("gconf_client_set_string: error: %s\n",
			  error->message);
		g_error_free(error);
		goto err;
	}

	return;

 err:
	g_warning("Failed to save state");
	return;
}

int treesort_compare_callback(GtkTreeModel * model, GtkTreeIter * iter1,
			      GtkTreeIter * iter2)
{
	int ret = 0;
	gchar *name1 = NULL;
	gchar *name2 = NULL;

	gtk_tree_model_get(model, iter1, COLUMN_READABLE_NAME, &name1, -1);
	gtk_tree_model_get(model, iter2, COLUMN_READABLE_NAME, &name2, -1);

	ret = g_strcmp0(name1, name2);

	g_free(name1);
	g_free(name2);

	return ret;
}

gboolean change_theme(gchar * theme_name)
{
	gboolean ret = FALSE;

	if (!theme_name) {
		g_return_if_fail_warning(0, "update_symbolic_link", "newtheme");	// XXX: uuh?
	}

	gchar *command =
	    g_strdup_printf("sudo %s %s/%s", "/usr/bin/personalisation",
			    "/usr/share/themes", theme_name);
	ret = g_spawn_command_line_sync(command, NULL, NULL, NULL, NULL);

	return ret;
}

void on_row_tapped(GtkTreeView * view, GtkTreePath * path, GTypeInstance * foo)
{
	(void)foo;		// XXX
	char *theme_name = NULL;
	GtkTreeModel *treemodel = NULL;
	GtkTreeIter iter;

	g_debug("on_rop_tapped");

	treemodel = gtk_tree_view_get_model(view);

	if (!gtk_tree_model_get_iter(treemodel, &iter, path)) {
		// XXX: restore default theme?
	}

	gtk_tree_model_get(treemodel, &iter, COLUMN_DIRNAME, &theme_name, -1);

	fprintf(stderr, "Selected: %s\n", theme_name);

	hildon_gtk_window_set_progress_indicator(GTK_WINDOW(static_dialog),
						 TRUE);
	gtk_widget_set_sensitive(static_dialog, FALSE);
	while (1) {
		if (!gtk_events_pending())
			break;
		gtk_main_iteration_do(TRUE);
	}
	g_timeout_add_seconds(20, (GSourceFunc) stop_progress_indicator, NULL);

	// TODO: event processing + timeout after 20s

	change_theme(theme_name);

	GtkSettings *settings = gtk_settings_get_default();
	gtk_settings_set_string_property(settings, "gtk-theme-name", theme_name,
					 "");
	gtk_rc_reset_styles(settings);

	GdkEventClient event;
	memset(&event, 0, sizeof(GdkEventClient));
	event.type = GDK_CLIENT_EVENT;
	event.window = 0;
	event.send_event = 1;
	event.message_type = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);
	event.data_format = 8;
	gdk_event_send_clientmessage_toall((GdkEvent *) & event);

	// Send SIGHUP to maemo-launcher
	FILE *launcher_pid = fopen("/tmp/maemo-launcher.pid", "r");
	if (launcher_pid) {
		char pid_s[32];
		int pid = 0;
		if (fgets(pid_s, sizeof(pid_s), launcher_pid)) {
			fprintf(stderr, "Found launcher pid: %s\n", pid_s);
			pid = strtol(pid_s, NULL, 10);

			if (pid != 0)
				kill(pid, SIGHUP);

		} else {
			fprintf(stderr, "Could not find launcher pid\n");
		}
		fclose(launcher_pid);
	}

	char *display = getenv("DISPLAY");
	Display *xdisplay = XOpenDisplay(display);
	Window xwindow = DefaultRootWindow(xdisplay);

	Atom theme_atom = XInternAtom(xdisplay, "_MB_THEME", FALSE);
	if (theme_atom) {
		XChangeProperty(xdisplay, xwindow, theme_atom, XA_STRING, 8,
				PropModeReplace, (unsigned char *)theme_name,
				strlen(theme_name));
		Atom command_atom = XInternAtom(xdisplay, "_MB_COMMAND", FALSE);
		// https://tronche.com/gui/x/xlib/events/client-communication/client-message.html
		if (command_atom) {
			XEvent event;
			memset(&event, 0, sizeof(XEvent));
			event.type = ClientMessage;
			event.xclient.display = xdisplay;
			event.xclient.window = xwindow;
			event.xclient.message_type = command_atom;
			event.xclient.format = 8;	// 8 = list of bytes
			event.xclient.data.b[0] = 1;	// MB_CMD_SET_THEME = 1

			XSendEvent(xdisplay, xwindow, FALSE,
				   SubstructureNotifyMask |
				   SubstructureRedirectMask, &event);
			XSync(xdisplay, TRUE);
		} else {
			g_warning("No _MB_COMMAND Atom");
		}
	} else {
		g_warning("No _MB_THEME Atom");
	}
	XCloseDisplay(xdisplay);

	g_free(theme_name);

	// TODO: close dialog?
}

gchar *get_current_theme()
{
	GKeyFile *key_file = NULL;
	GError *error = NULL;
	gchar *ret = NULL;

	gchar *theme_symlink_index =
	    g_strconcat(ETC_THEME_SYMLINK, "/index.theme", NULL);
	if (!g_file_test(theme_symlink_index, G_FILE_TEST_EXISTS)) {
		g_error("Error reading current theme index\n");
		goto cleanup;
	}

	key_file = g_key_file_new();
	if (key_file) {
		g_key_file_load_from_file(key_file, theme_symlink_index,
					  G_KEY_FILE_NONE, &error);
		if (!error) {
			g_key_file_get_string(key_file, "Desktop Entry", "Name",
					      &error);
			if (error) {
				goto cleanup;
			}
		}
	}

	if (!ret) {
		if (error)
			g_error_free(error);

		gchar *link = g_file_read_link(ETC_THEME_SYMLINK, &error);
		if (error)
			goto cleanup;

		if (link == NULL) {
			g_warning("Error: cannot read symbolic link\n");
			goto cleanup;
		} else {
			gchar *link2 = g_file_read_link(link, NULL);
			if (link2) {
				ret = g_path_get_basename(link2);
				g_free(link2);
			} else {
				ret = g_path_get_basename(link);
			}
			g_free(link);
		}
	}

 cleanup:
	g_free(theme_symlink_index);
	if (error)
		g_error_free(error);
	return ret;
}

GdkPixbuf *load_icon_for_theme(const gchar * icon_path)
{
	GtkIconTheme *default_theme;
	GdkPixbuf *icon_pixbuf = NULL;
	GError *error = NULL;

	default_theme = gtk_icon_theme_get_default();
	if (!icon_path)
		return NULL;

	icon_pixbuf = gdk_pixbuf_new_from_file_at_size(icon_path, 80, 60, NULL);
	if (icon_pixbuf)
		return icon_pixbuf;

	icon_pixbuf =
	    gtk_icon_theme_load_icon(default_theme, icon_path, 60,
				     GTK_ICON_LOOKUP_NO_SVG, NULL);
	if (icon_pixbuf)
		return icon_pixbuf;

	icon_path = "qgn_list_gene_unknown_file";
	icon_pixbuf = gtk_icon_theme_load_icon(default_theme, icon_path, 60,
					       GTK_ICON_LOOKUP_NO_SVG, &error);

	if (error) {
		g_warning("Failed to load icon '%s': %s\n", icon_path,
			  error->message);
		g_error_free(error);
	}

	return icon_pixbuf;
}

int main(int argc, char **argv)
{
	GtkWidget *dialog;
	char *dialog_title;

	bindtextdomain("hildon-control-panel-personalisation",
		       "/usr/share/locale");

	hildon_gtk_init(&argc, &argv);
	dialog_title =
	    dcgettext("hildon-control-panel-personalisation", "pers_fi_label",
		      LC_MESSAGES);
	dialog =
	    gtk_dialog_new_with_buttons(dialog_title, NULL,
					GTK_DIALOG_NO_SEPARATOR |
					GTK_DIALOG_DESTROY_WITH_PARENT |
					GTK_DIALOG_MODAL, 0);
	static_dialog = dialog;

	//g_object_set(parent_of_dialog, "right-padding", 16, 0);

	gtk_widget_set_no_show_all(dialog, TRUE);
	gtk_widget_hide(dialog);

	GDir *dir = g_dir_open(THEME_DIR, 0, NULL);
	if (dir == NULL) {
		g_error("Unable to open theme directory: " THEME_DIR);

		return EXIT_FAILURE;
	}

	GtkListStore *themes =
	    gtk_list_store_new(9, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			       G_TYPE_STRING,
			       G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

	const gchar *dirname = NULL;
	while ((dirname = g_dir_read_name(dir))) {
		gchar *full_path = NULL;
		gchar *index_path = NULL;
		GKeyFile *key_file = NULL;

		gchar *desktopentry_icon = NULL;
		gchar *desktopentry_name = NULL;
		gchar *xhildon_gtktheme = NULL;
		gchar *xhildon_matchboxtheme = NULL;
		gchar *xhildon_additionalrcfiles = NULL;

		full_path = g_strconcat(THEME_DIR, "/", dirname, NULL);
		if (g_file_test(full_path, G_FILE_TEST_IS_SYMLINK))
			goto cleanup;

		index_path =
		    g_strconcat(THEME_DIR, "/", dirname, "/index.theme", NULL);
		if (!g_file_test(index_path, G_FILE_TEST_EXISTS))
			goto cleanup;

		key_file = g_key_file_new();
		GError *error = NULL;
		g_key_file_load_from_file(key_file, index_path, G_KEY_FILE_NONE,
					  &error);
		if (error != NULL) {
			g_error("Unable to open key file %s: %s", index_path,
				error->message);
			g_error_free(error);
			goto cleanup;
		}

		/* TODO: error checks */
		desktopentry_icon =
		    g_key_file_get_string(key_file, "Desktop Entry", "Icon",
					  &error);
		if (error)
			goto errorc;
		desktopentry_name =
		    g_key_file_get_string(key_file, "Desktop Entry", "Name",
					  &error);
		if (error)
			goto errorc;
		xhildon_gtktheme =
		    g_key_file_get_string(key_file, "X-Hildon-Metatheme",
					  "GtkTheme", &error);
		if (error)
			goto errorc;
		xhildon_matchboxtheme =
		    g_key_file_get_string(key_file, "X-Hildon-Metatheme",
					  "X-MatchboxTheme", &error);
		if (error)
			goto errorc;
		xhildon_additionalrcfiles =
		    g_key_file_get_string(key_file, "X-Hildon-Metatheme",
					  "X-AdditionalRCFiles", &error);
		if (error)
			goto errorc;

		GdkPixbuf *icon_pixbuf = load_icon_for_theme(desktopentry_icon);

		GtkTreeIter iter;
		gtk_list_store_append(themes, &iter);
		gtk_list_store_set(themes, &iter, COLUMN_ICON, icon_pixbuf,
				   // XXX: why the same twice
				   COLUMN_READABLE_NAME, desktopentry_name,
				   COLUMN_TRANSLATED_NAME, desktopentry_name,
				   COLUMN_HILDON_GTKTHEME, xhildon_gtktheme,
				   COLUMN_HILDON_MATCHBOXTHEME,
				   xhildon_matchboxtheme, 5, NULL,
				   COLUMN_HILDON_ADDITIONALRCFILES,
				   xhildon_additionalrcfiles, COLUMN_DIRNAME,
				   dirname, 8, NULL, -1);
		// TODO: g_object_unref(icon_pixbuf);
		goto cleanup;

 errorc:
		g_free(desktopentry_icon);
		g_free(desktopentry_name);
		g_free(xhildon_gtktheme);
		g_free(xhildon_matchboxtheme);
		g_free(xhildon_additionalrcfiles);
		if (error)
			g_error_free(error);

 cleanup:
		if (key_file)
			g_key_file_free(key_file);
		g_free(index_path);
		g_free(full_path);
	}

	GtkWidget *tree_view =
	    hildon_gtk_tree_view_new_with_model(HILDON_UI_MODE_EDIT,
						GTK_TREE_MODEL(themes));
	gtk_widget_set_can_focus(GTK_WIDGET(tree_view), TRUE);

	GtkTreeViewColumn *col = gtk_tree_view_column_new();
	GtkCellRenderer *render = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
	gtk_tree_view_column_pack_start(col, render, FALSE);
	gtk_tree_view_column_add_attribute(col, render, "pixbuf", 0);

	col = gtk_tree_view_column_new();
	render = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT
							  (render), 1);

	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
	gtk_tree_view_column_pack_end(col, render, TRUE);
	gtk_tree_view_column_add_attribute(col, render, "text", 1);

	g_signal_connect_data(GTK_WIDGET(tree_view), "cursor-changed",
			      (GCallback) save_theme_setting, NULL, 0,
			      G_CONNECT_AFTER);

	GtkWidget *pannable = hildon_pannable_area_new();
	gtk_container_add(GTK_CONTAINER(pannable), GTK_WIDGET(tree_view));

	gtk_widget_set_size_request(GTK_WIDGET(pannable), -1, 360);

	/* XXX: signal / sizes lol */

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), pannable);
	g_signal_connect_data(GTK_WIDGET(tree_view), "hildon_row_tapped",
			      (GCallback) on_row_tapped, tree_view, 0,
			      G_CONNECT_AFTER);

	select_current_theme(GTK_TREE_VIEW(tree_view));
	gtk_widget_set_no_show_all(dialog, FALSE);
	gtk_widget_show_all(GTK_WIDGET(dialog));
	gtk_dialog_run(GTK_DIALOG(dialog));
}
