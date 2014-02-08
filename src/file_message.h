#ifndef _FILE_MESSAGE_H_
#define _FILE_MESSAGE_H_

#include <glib.h>
#include <glib-object.h>

typedef enum _File_Message_Operation {

	FILE_MESSAGE_INVALID_TYPE = 0,

	/* Server to client */
	FILE_MESSAGE_NEW, /* A new file to show in the GUI */
	FILE_MESSAGE_DELETED, /* A file was deleted */
	FILE_MESSAGE_VERSION, /* A  file commit */
	FILE_MESSAGE_ROOT_PATH, /* The file keeper root dir */

	/* Client to server */
	FILE_MESSAGE_REVERT, /* Request a file revert */
	FILE_MESSAGE_REVERT_ABORT,
	FILE_MESSAGE_REVERT_CONFIRM,
	FILE_MESSSAGE_VERSIONS, /* Request versions of a given file*/
} File_Message_Operation;

#define G_TYPE_FILE_MSG         (file_msg_get_type ())
#define G_FILE_MSG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_FILE_MSG, FileMsg))
#define G_FILE_MSG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_FILE_MSG, FileMsgClass))
#define G_IS_FILE_MSG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_FILE_MSG))
#define G_IS_FILE_MSG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_FILE_MSG))
#define G_FILE_MSG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_FILE_MSG, FileMsgClass))

typedef struct _FileMsg         FileMsg;
typedef struct _FileMsgClass    FileMsgClass;
typedef struct _FileMsgPrivate  FileMsgPrivate;

struct _FileMsg
{
  GObject parent_instance;
  FileMsgPrivate *priv;
};

struct _FileMsgClass
{
  GObjectClass parent_class;
};

void file_msg_set_operation(FileMsg *self, File_Message_Operation type);

void file_msg_set_file_path(FileMsg *self, const char *file_path);

const char *file_msg_get_file_path(FileMsg *self);

gint64 file_msg_get_timestamp(FileMsg *self);

void file_msg_set_timestamp(FileMsg *self, gint64 timestamp);

char *file_msg_to_string(FileMsg *self);

File_Message_Operation file_msg_get_operation_type(FileMsg *self);

FileMsg *file_msg_from_string_command_new(const char *str);
FileMsg *file_msg_from_operation_and_file_path_new(File_Message_Operation type, const char *file);

FileMsg *file_msg_new(void);

GType file_msg_get_type(void);

GType file_msg_operation_emum_get_type(void);

#define G_TYPE_FILE_MSG_OPERATION_TYPE (file_msg_operation_emum_get_type())

#endif