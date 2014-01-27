#include "file_message.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct _FileMsgPrivate {
	File_Message_Operation type;
	char *file_name;
	gint64 timestamp; /* this is unix type. Will be used in
		FILE_MESSAGE_VERSION, to indicate the commit date */
};

enum {
	PROP_0,
	PROP_TYPE,
	PROP_FILE_NAME,
	PROP_TIMESTAMP
};

G_DEFINE_TYPE_WITH_PRIVATE (FileMsg, file_msg, G_TYPE_OBJECT)

GType
file_msg_operation_emum_get_type(void)
{
	static volatile gsize type_volatile = 0;
	if (g_once_init_enter(&type_volatile)) {
		static const GEnumValue values[] = {
			{ FILE_MESSAGE_INVALID_TYPE, "FILE_MESSAGE_INVALID_TYPE", "Invalid type" },
			{ FILE_MESSAGE_NEW, "FILE_MESSAGE_NEW", "A new file" },
			{ FILE_MESSAGE_DELETED, "FILE_MESSAGE_DELETED", "A deleted file" },
			{ FILE_MESSAGE_VERSION, "FILE_MESSAGE_VERSION", "New file version" },
			{ FILE_MESSAGE_REVERT, "FILE_MESSAGE_REVERT", "Revert a file" },
			{ FILE_MESSSAGE_VERSIONS, "FILE_MESSSAGE_VERSIONS",
				"Request commits of a given file" },
			{ 0, NULL, NULL }
		};
		GType type = g_enum_register_static(
			g_intern_static_string("File_Message_Type"), values);

		g_once_init_leave(&type_volatile, type);
	}
	return type_volatile;
}

static void
file_msg_set_property(GObject *obj, guint id, const GValue *value,
	GParamSpec *pspec)
{
	FileMsg *self = G_FILE_MSG(obj);
	switch(id)
	{
		case PROP_TYPE:
			file_msg_set_operation(self, g_value_get_enum(value));
			break;
		case PROP_FILE_NAME:
			file_msg_set_file(self, g_value_get_string(value));
			break;
		case PROP_TIMESTAMP:
			file_msg_set_timestamp(self, g_value_get_int64(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
	}
}

static void
file_msg_get_property(GObject *obj, guint id, GValue *value, GParamSpec *pspec)
{
	FileMsg *self = G_FILE_MSG(obj);
	switch(id)
	{
		case PROP_TYPE:
			g_value_set_enum(value, self->priv->type);
			break;
		case PROP_FILE_NAME:
			g_value_set_string(value, self->priv->file_name);
			break;
		case PROP_TIMESTAMP:
			g_value_set_int64(value, self->priv->timestamp);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
	}
}

static void
file_msg_finalize(GObject *obj)
{
	FileMsg *self = G_FILE_MSG(obj);
	g_free(self->priv->file_name);
	G_OBJECT_CLASS(file_msg_parent_class)->finalize(obj);
}

static void
file_msg_class_init(FileMsgClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->set_property = file_msg_set_property;
	gobject_class->get_property = file_msg_get_property;
	gobject_class->finalize = file_msg_finalize;

	g_object_class_install_property(gobject_class, PROP_TYPE,
		g_param_spec_enum("type", "Message type", "The message type",
		G_TYPE_FILE_MSG_OPERATION_TYPE, FILE_MESSAGE_INVALID_TYPE,
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, PROP_FILE_NAME,
		g_param_spec_string("file-name", "File name", "The file to operate",
		NULL, G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, PROP_TIMESTAMP,
		g_param_spec_int64("timestamp", "Timestamp", "A timestamp of a given commit",
		G_MININT64, G_MAXINT64, 0,
		G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
file_msg_init(FileMsg *self)
{
	self->priv = file_msg_get_instance_private(self);
}

static void
file_msg_parse_command(FileMsg *self, const char *str)
{
	GRegex *regx;
	GMatchInfo *info;
	GError *err = NULL;
	char *command;
	char *file;
	char *timestamp;

	regx = g_regex_new(
		"<command>(\\d+)</command><file>([\\s*\\w*\\.*]+)</file>(<timestamp>(\\d+)</timestamp>){0,1}",
		G_REGEX_CASELESS, 0, NULL);

	g_regex_match_full(regx, str, -1, 0, 0, &info, &err);
	g_regex_unref(regx);

	if (err) {
		g_printerr("Error: %s\n", err->message);
		g_error_free(err);
		return;
	}

	if (g_match_info_matches(info)) {
		command = g_match_info_fetch(info, 1);
		file = g_match_info_fetch(info, 2);
		timestamp = g_match_info_fetch(info, 3);
		file_msg_set_operation(self, (File_Message_Operation)atoi(command));
		file_msg_set_file(self, file);
		if (timestamp)
			file_msg_set_timestamp(self, g_ascii_strtoll(timestamp, NULL, 10));
		g_free(command);
		g_free(file);
		g_free(timestamp);
	}

	g_match_info_free(info);
}

void
file_msg_set_operation(FileMsg *self, File_Message_Operation type)
{
	g_return_if_fail(self);

	if (type  < FILE_MESSAGE_INVALID_TYPE || type > FILE_MESSAGE_REVERT)
		self->priv->type = FILE_MESSAGE_INVALID_TYPE;
	else
		self->priv->type = type;
	g_object_notify(G_OBJECT(self), "type");
}

void
file_msg_set_file(FileMsg *self, const char *file)
{
	g_return_if_fail(self);
	self->priv->file_name = g_strdup(file);
	g_object_notify(G_OBJECT(self), "file-name");
}

const char *
file_msg_get_file(FileMsg *self)
{
	g_return_val_if_fail(self, NULL);
	return self->priv->file_name;
}

File_Message_Operation
file_msg_get_operation_type(FileMsg *self)
{
	g_return_val_if_fail(self, FILE_MESSAGE_INVALID_TYPE);
	return self->priv->type;
}

gint64
file_msg_get_timestamp(FileMsg *self)
{
	g_return_val_if_fail(self, 0);
	return self->priv->timestamp;
}

void
file_msg_set_timestamp(FileMsg *self, gint64 timestamp)
{
	g_return_if_fail(self);
	self->priv->timestamp = timestamp;
	g_object_notify(G_OBJECT(self), "timestamp");
}

char *
file_msg_to_string(FileMsg *self)
{
	GString *str;
	g_return_val_if_fail(self, NULL);

	str = g_string_new(NULL);
	g_string_append_printf(str, "<command>%d</command>",
		(int)self->priv->type);
	g_string_append_printf(str, "<file>%s</file>", self->priv->file_name);
	if (self->priv->type == FILE_MESSAGE_VERSION) {
		g_string_append_printf(str,
			"<timestamp>%"G_GINT64_FORMAT"</timestamp>",
			self->priv->timestamp);
	}
	return g_string_free(str, FALSE);
}

FileMsg *
file_msg_from_string_command_new(const char *str)
{
	FileMsg *msg;

	g_return_val_if_fail(str, NULL);
	msg = g_object_new(G_TYPE_FILE_MSG, NULL);
	file_msg_parse_command(msg, str);
	return msg;
}

FileMsg *
file_msg_from_operation_and_file_new(File_Message_Operation type,
	const char *file)
{
	return g_object_new(G_TYPE_FILE_MSG, "type", type, "file-name",
		file, NULL);
}

FileMsg *
file_msg_new(void)
{
	return g_object_new(G_TYPE_FILE_MSG, NULL);
}
