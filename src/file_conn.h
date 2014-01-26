#ifndef _FILE_CONN_H_
#define _FILE_CONN_H_

#include <glib.h>
#include <glib-object.h>
#include "file_message.h"


#define G_TYPE_FILE_CONN         (file_conn_get_type ())
#define G_FILE_CONN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_FILE_CONN, FileConn))
#define G_FILE_CONN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_FILE_CONN, FileConnClass))
#define G_IS_FILE_CONN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_FILE_CONN))
#define G_IS_FILE_CONN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_FILE_CONN))
#define G_FILE_CONN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_FILE_CONN, FileConnClass))

typedef struct _FileConn         FileConn;
typedef struct _FileConnClass       FileConnClass;
typedef struct _FileConnPrivate	FileConnPrivate;

struct _FileConn
{
  GObject parent_instance;

  FileConnPrivate *priv;
};

struct _FileConnClass
{
  GObjectClass parent_class;


  void     (* client_connected) (FileConn      *conn);
  void     (*client_request) (FileConn *conn, FileMsg *msg);
};

FileConn *file_conn_new(void);
gboolean file_conn_start_listen(FileConn *self, guint16 port);
GType file_conn_get_type(void);

gboolean file_conn_send_msg(FileConn *self, FileMsg *msg);

#endif
