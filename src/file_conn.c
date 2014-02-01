#include "file_conn.h"
#include "utils.h"
#include <gio/gio.h>
#include <stdio.h>

struct _FileConnPrivate {
	GSocketService *service;
	GSocketConnection *connection;
	guint watch_id;
	GIOChannel *channel;
};

enum {
	CLIENT_CONNECTED,
	CLIENT_REQUEST,
	CLIENT_DISCONNECTED,
	LAST_SIGNAL
};

G_DEFINE_TYPE_WITH_PRIVATE (FileConn, file_conn, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0, 0 };

static gboolean
file_conn_can_read(GIOChannel *source, GIOCondition cond,
	gpointer data)
{
	FileMsg *msg;
	gboolean r = TRUE;
	FileConn *self = data;
	GString *s = g_string_new(NULL);
	GIOStatus ret = g_io_channel_read_line_string(source, s, NULL, NULL);

	if ((cond & G_IO_HUP) || (cond & G_IO_ERR) || ret == G_IO_STATUS_EOF ||
		ret == G_IO_STATUS_ERROR) {
		printf("Disconnected or error!\n");
		g_object_unref(self->priv->connection);
		self->priv->connection = NULL;
		r = FALSE;
		g_signal_emit(self, signals[CLIENT_DISCONNECTED], 0);
		g_socket_service_start(self->priv->service);
		self->priv->watch_id = 0;
	} else {
		msg = file_msg_from_string_command_new(s->str);
		g_signal_emit(self, signals[CLIENT_REQUEST], 0, msg);
		g_object_unref(msg);
	}
	g_string_free(s, TRUE);
	return r;
}

gboolean
file_conn_send_msg(FileConn *self, FileMsg *msg)
{
	GIOStatus status;
	char *str;
	g_return_val_if_fail(self, FALSE);
	g_return_val_if_fail(msg, FALSE);

	str = file_msg_to_string(msg);

	status = g_io_channel_write_chars(self->priv->channel,
		str, -1, NULL, NULL);

	printf("Message: %s sent to client\n", str);
	g_free(str);

	if (status != G_IO_STATUS_NORMAL)
		return FALSE;
	return TRUE;
}

static gboolean
file_conn_incoming_conn(GSocketService *service,
	GSocketConnection *connection, GObject *source, gpointer data)
{
	FileConn *self = data;
	GIOChannel *channel;
	gint fd;
	GSocket *socket = g_socket_connection_get_socket(connection);

	fd = g_socket_get_fd(socket);

#ifdef G_OS_WIN32
	channel = g_io_channel_win32_new_socket(fd);
#else
	channel = g_io_channel_unix_new(fd);
#endif

	self->priv->watch_id = g_io_add_watch(channel,
		G_IO_IN | G_IO_ERR | G_IO_HUP,
		file_conn_can_read, self);

	self->priv->channel = channel;
	self->priv->connection = g_object_ref(connection);

	g_socket_service_stop(service);
	printf("Client connected!\n");
	g_signal_emit(self, signals[CLIENT_CONNECTED], 0);
	(void)source;
	return TRUE;
}

static void
file_conn_dispose(GObject *gobject)
{
	FileConn *self = G_FILE_CONN(gobject);

	g_clear_object(&self->priv->connection);
	g_clear_object(&self->priv->service);

	G_OBJECT_CLASS(file_conn_parent_class)->dispose(gobject);
}

static void
file_conn_finalize(GObject *gobject)
{
  FileConn *self = G_FILE_CONN(gobject);

  if (self->priv->watch_id)
  	g_source_remove(self->priv->watch_id);
  g_io_channel_unref(self->priv->channel);
  G_OBJECT_CLASS(file_conn_parent_class)->finalize(gobject);
}

static void
file_conn_class_init(FileConnClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = file_conn_dispose;
	gobject_class->finalize = file_conn_finalize;

	signals[CLIENT_CONNECTED] =
		g_signal_new ("client_connected",
		  G_TYPE_FILE_CONN,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (FileConnClass, client_connected),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0, NULL);

	signals[CLIENT_REQUEST] =
		g_signal_new ("client_request",
		  G_TYPE_FILE_CONN,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (FileConnClass, client_request),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, G_TYPE_FILE_MSG);

	signals[CLIENT_DISCONNECTED] =
		g_signal_new ("client_disconnected",
		  G_TYPE_FILE_CONN,
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (FileConnClass, client_disconnected),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0, NULL);
}

static void
file_conn_init(FileConn *self)
{
	self->priv = file_conn_get_instance_private(self);
	self->priv->service = g_socket_service_new();

	g_socket_listener_add_inet_port(
		G_SOCKET_LISTENER(self->priv->service),
		8001, NULL, NULL);

	g_signal_connect(self->priv->service, "incoming",
		G_CALLBACK(file_conn_incoming_conn), self);
}

gboolean
file_conn_start_listen(FileConn *self, guint16 port)
{
	g_return_val_if_fail(self, FALSE);

	if (!g_socket_listener_add_inet_port(
		G_SOCKET_LISTENER(self->priv->service),
		port, NULL, NULL))
		return FALSE;

	g_socket_service_start(self->priv->service);

	return TRUE;
}

FileConn *
file_conn_new(void)
{
	return g_object_new (G_TYPE_FILE_CONN, NULL);
}

