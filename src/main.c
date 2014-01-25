#include <glib.h>
#include <git2.h>
#include <stdio.h>
#include "file_watcher.h"
#include "file_conn.h"
#include "file_message.h"

static void
_client_connected(FileConn *self, gpointer data)
{
	FileMsg *msg;
	GList *files = file_watcher_get_monitored_files(data);
	GList *itr;

	for (itr = files; itr; itr = itr->next) {
		msg = file_msg_from_operation_and_file_new(FILE_MESSAGE_NEW,
			itr->data);
		if (!file_conn_send_msg(self, msg))
			printf("Could not send message to the client!\n");
		g_object_unref(msg);
	}
	g_list_free(files);
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
		G_CALLBACK(_client_connected), watcher);
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
