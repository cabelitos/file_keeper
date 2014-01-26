#include <glib.h>
#include <git2.h>
#include <stdio.h>
#include "file_watcher.h"
#include "file_conn.h"
#include "file_message.h"

static void
_client_connected(FileConn *conn, gpointer data)
{
	FileMsg *msg;
	GList *files = file_watcher_get_monitored_files(data);
	GList *itr;

	for (itr = files; itr; itr = itr->next) {
		msg = file_msg_from_operation_and_file_new(FILE_MESSAGE_NEW,
			itr->data);
		if (!file_conn_send_msg(conn, msg))
			printf("Could not send message to the client!\n");
		g_object_unref(msg);
	}
	g_list_free(files);
}

static void
_handle_file_versions_request(File_Watcher *watcher, FileConn *conn,
	const char *file_name)
{
	GList *versions = file_watcher_request_file_versions(watcher, file_name);
	GList *itr;
	FileMsg *msg;
	gint64 *unix_time;

	for (itr = versions; itr; itr = itr->next) {
		unix_time = itr->data;
		msg = file_msg_from_operation_and_file_new(FILE_MESSAGE_VERSION,
			file_name);
		file_msg_set_timestamp(msg, *unix_time);
		if (!file_conn_send_msg(conn, msg))
			printf("Could not send message to the client!\n");
		g_free(unix_time);
		g_object_unref(msg);
	}
	g_list_free(versions);
}

static void
_client_request(FileConn *conn, FileMsg *msg, gpointer data)
{
	File_Message_Operation op = file_msg_get_operation_type(msg);
	if (op == FILE_MESSAGE_INVALID_TYPE) {
		printf("Invalid message!\n");
		return;
	} else if (op == FILE_MESSSAGE_VERSIONS)
		_handle_file_versions_request(data, conn, file_msg_get_file(msg));
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
	g_signal_connect(conn, "client_request",
		G_CALLBACK(_client_request), watcher);

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
