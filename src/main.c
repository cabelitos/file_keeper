#include <glib.h>
#include <git2.h>
#include <stdio.h>
#include "file_watcher.h"
#include "file_conn.h"

static void _client_connected(FileConn *self)
{
	/* TODO */
}

int main(int argc, char **argv)
{
	GMainLoop *loop;
	File_Watcher *watcher;
	FileConn *conn;

	(void) argc;
	(void) argv;

	watcher = file_watcher_new();

	if (!watcher)
		return -1;

	conn = file_conn_new();
	g_signal_connect(conn, "client_connected",
		G_CALLBACK(_client_connected), NULL);
	file_conn_start_listen(conn, 8001);

	git_threads_init();

	loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	file_watcher_free(watcher);
	g_object_unref(conn);
	git_threads_shutdown();

	return 0;
}
