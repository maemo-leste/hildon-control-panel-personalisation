#include <stdio.h>
#include <signal.h>

#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <hildon/hildon.h>
#include <gtk/gtk.h>

GPid child_pid = 0;

void child_done_cb(GPid pid, gint status, gpointer user_data)
{
	int *exit_code = (int *)user_data;

	g_debug("Exit code for child (%d): %d", (pid_t) pid, status);

	/* TODO: only return 0 upon success? */
	*exit_code = 0;

	gtk_main_quit();
}

void on_delete_event(void)
{
	g_debug("Received delete event.");
	kill(child_pid, SIGTERM);
	gtk_main_quit();
}

void signal_handler(int sig)
{
	(void)sig;
	g_debug("Received SIGTERM");
	kill(child_pid, SIGTERM);
	gtk_main_quit();
}

osso_return_t execute(osso_context_t * osso, gpointer user_data,
		      gboolean user_activated)
{
	int ret = -1;
	char *argv[] = { "/usr/bin/personalisation_app", NULL };

	(void)osso;
	(void)user_activated;

	g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL,
		      &child_pid, NULL);
	g_child_watch_add(child_pid, child_done_cb, &ret);
	signal(SIGTERM, signal_handler);
	g_signal_connect_data(GTK_WIDGET(user_data), "delete-event",
			      on_delete_event, NULL, NULL, G_CONNECT_AFTER);
	gtk_main();

	return OSSO_OK;
}

osso_return_t save_state(osso_context_t * osso, gpointer user_data)
{
	(void)osso;
	(void)user_data;
	return OSSO_OK;
}
