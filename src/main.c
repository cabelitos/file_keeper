#include <glib.h>
#include <git2.h>
#include <stdio.h>
#include "file_watcher.h"
#include "file_conn.h"
#include "file_message.h"

typedef struct _WatherContext {
	char *last_file;
	FileWatcher *watcher;
} WatherContext;

static void
client_connected(FileConn *conn, gpointer data)
{
	FileMsg *msg;
	GList *files = file_watcher_get_monitored_files(data);
	GList *itr;

	for (itr = files; itr; itr = itr->next) {
		msg = file_msg_from_operation_and_file_path_new(FILE_MESSAGE_NEW,
			itr->data);
		if (!file_conn_send_msg(conn, msg))
			printf("Could not send message to the client!\n");
		g_object_unref(msg);
	}
	g_list_free(files);
}

static void
handle_file_versions_request(FileWatcher *watcher, FileConn *conn,
	const char *file_name)
{
	GList *versions = file_watcher_request_file_versions(watcher, file_name);
	GList *itr;
	FileMsg *msg;
	gint64 *unix_time;

	for (itr = versions; itr; itr = itr->next) {
		unix_time = itr->data;
		msg = file_msg_from_operation_and_file_path_new(FILE_MESSAGE_VERSION,
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
watcher_ctx_last_file_replace(WatherContext *ctx, const char *str)
{
	g_free(ctx->last_file);
	ctx->last_file = g_strdup(str);
}

static void
client_request(FileConn *conn, FileMsg *msg, gpointer data)
{
	WatherContext *ctx = data;
	File_Message_Operation op = file_msg_get_operation_type(msg);
	if (op == FILE_MESSAGE_INVALID_TYPE)
		printf("Invalid message!\n");
	else if (op == FILE_MESSSAGE_VERSIONS)
		handle_file_versions_request(ctx->watcher, conn,
			file_msg_get_file_path(msg));
	else if (op == FILE_MESSAGE_REVERT &&
		file_msg_get_timestamp(msg) > 0) {
		file_watcher_request_revert_file(ctx->watcher,
			file_msg_get_file_path(msg), file_msg_get_timestamp(msg));
		watcher_ctx_last_file_replace(ctx, file_msg_get_file_path(msg));
	} else if (op == FILE_MESSAGE_REVERT_ABORT) {
		file_watcher_request_revert_end(ctx->watcher,
			file_msg_get_file_path(msg), TRUE);
		watcher_ctx_last_file_replace(ctx, NULL);
	 } else if (op == FILE_MESSAGE_REVERT_CONFIRM) {
		file_watcher_request_revert_end(ctx->watcher,
			file_msg_get_file_path(msg), FALSE);
		watcher_ctx_last_file_replace(ctx, NULL);
	} else
		printf("Could not process the command: %d\n", (int) op);
}

static void
client_disconnected(FileConn *conn, void *data)
{
	WatherContext *ctx = data;

	if (!ctx->last_file)
		return;

	file_watcher_request_revert_end(ctx->watcher, ctx->last_file, TRUE);
	watcher_ctx_last_file_replace(ctx, NULL);
}

static void
watcher_changed(FileWatcher *watcher, File_Watcher_Changed_Type type,
	const char *path, void *data)
{
	FileMsg *msg = file_msg_new();

	file_msg_set_file_path(msg, path);

	if (type == FILE_DELETED)
		file_msg_set_operation(msg, FILE_MESSAGE_DELETED);
	else
		file_msg_set_operation(msg, FILE_MESSAGE_NEW);

	file_conn_send_msg(data, msg);
	g_object_unref(msg);
	(void)watcher;
}

int
main(int argc, char **argv)
{
	GMainLoop *loop;
	FileWatcher *watcher;
	FileConn *conn;
	WatherContext ctx;

	(void) argc;
	(void) argv;

	watcher = file_watcher_new();

	if (!watcher)
		return -1;

	ctx.watcher = watcher;
	ctx.last_file = NULL;
	conn = file_conn_new();

	g_signal_connect(conn, "client_connected",
		G_CALLBACK(client_connected), watcher);
	g_signal_connect(conn, "client_request",
		G_CALLBACK(client_request), &ctx);
	g_signal_connect(conn, "client_disconnected",
		G_CALLBACK(client_disconnected), &ctx);

	file_watcher_set_file_watcher_changed_cb(watcher, watcher_changed,
		conn);

	file_conn_start_listen(conn, 8001);

	git_threads_init();

	loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	file_watcher_free(watcher);
	g_object_unref(conn);
	git_threads_shutdown();
	g_free(ctx.last_file);

	return 0;
}
