#include "file_conn.h"
#include "utils.h"
#include <gio/gio.h>
#include <stdio.h>

struct _FileConnPrivate {
	GSocketService *service;
	GSocketConnection *connection;
	guint watch_id;
};

enum {
	CLIENT_CONNECTED,
	LAST_SIGNAL
};

G_DEFINE_TYPE_WITH_PRIVATE (FileConn, file_conn, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL] = { 0 };

static gboolean _file_conn_can_read(GIOChannel *source, GIOCondition cond,
	gpointer data)
{
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
		g_socket_service_start(self->priv->service);
		self->priv->watch_id = 0;
	} else {
		/* TODO, notify msg */
		printf("%s", s->str);
	}
	g_string_free(s, TRUE);
	return r;
}

static gboolean _file_conn_incoming_conn(GSocketService *service,
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
		_file_conn_can_read, self);

	g_io_channel_unref(channel);
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
	g_clear_object (&self->priv->service);

	G_OBJECT_CLASS(file_conn_parent_class)->dispose(gobject);
}

static void
file_conn_finalize(GObject *gobject)
{
  FileConn *self = G_FILE_CONN(gobject);

  if (self->priv->watch_id)
  	g_source_remove(self->priv->watch_id);
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
		G_CALLBACK(_file_conn_incoming_conn), self);
}

gboolean file_conn_start_listen(FileConn *self, guint16 port)
{
	g_return_val_if_fail(self, FALSE);

	if (!g_socket_listener_add_inet_port(
		G_SOCKET_LISTENER(self->priv->service),
		port, NULL, NULL))
		return FALSE;

	g_socket_service_start(self->priv->service);

	return TRUE;
}
FileConn *file_conn_new(void)
{
	return g_object_new (G_TYPE_FILE_CONN, NULL);
}

