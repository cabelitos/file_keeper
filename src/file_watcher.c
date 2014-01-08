#include <gio/gio.h>
#include <stdio.h>
#include "file_watcher.h"

#define STR_MAX 4096

static GHashTable *_file_monitors = NULL;
static int _refcount = 0;

static void _file_watcher_key_destroy(gpointer data)
{
	g_free(data);
}

static void _file_watcher_value_destroy(gpointer data)
{
	g_object_unref(data);
}

void file_watcher_init(void)
{
	if (_refcount++ > 1)
		return;
	_file_monitors = g_hash_table_new_full(g_str_hash, g_str_equal,
		_file_watcher_key_destroy, _file_watcher_value_destroy);
}

void file_watcher_shutdown(void)
{
	--_refcount;
	if (_refcount > 0)
		return;
	else if (_refcount < 0) {
		_refcount = 0;
		return;
	}
	g_hash_table_destroy(_file_monitors);
	_file_monitors = NULL;
}

static void _monitor_changed(GFileMonitor *monitor, GFile *file, GFile *other,
	GFileMonitorEvent event, gpointer data)
{
	char *path;
	GFileType type;

	if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
		event == G_FILE_MONITOR_EVENT_UNMOUNTED ||
		event == G_FILE_MONITOR_EVENT_PRE_UNMOUNT)
			return;

	path = g_file_get_path(file);
	if (event == G_FILE_MONITOR_EVENT_DELETED)
		g_hash_table_remove(_file_monitors, path);
	else if (event == G_FILE_MONITOR_EVENT_CREATED) {
		type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
		if (type == G_FILE_TYPE_DIRECTORY)
			file_watcher_add_watches(path);
	}
	/* TODO: notify that something has changed! */
	g_free(path);
	(void) other;
	(void) monitor;
	(void) data;
}

static void _file_watcher_monitor_add(GFile *file)
{
	char *path;
	GFileMonitor *monitor;

	monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, NULL);

	if (!monitor)
		return;

	path = g_file_get_path(file);

	if (!path) {
		g_object_unref(monitor);
		return;
	}

	g_hash_table_insert(_file_monitors, path, monitor);
	g_signal_connect(monitor, "changed", G_CALLBACK(_monitor_changed), NULL);
}

void file_watcher_add_watches(const char *base_path)
{
	char path[STR_MAX];
	char attr[STR_MAX];
	GFile *file;
	GFileInfo *info;
	GFileEnumerator *f_enum;
	GFileType type;

	file = g_file_new_for_path(base_path);
	g_snprintf(attr, sizeof(attr), "%s,%s", G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_ATTRIBUTE_STANDARD_TYPE);
	info = g_file_query_info(file, attr, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (!info)
		goto exit;
	type = g_file_info_get_file_type(info);
	g_object_unref(info);
	if (type == G_FILE_TYPE_DIRECTORY) {
		_file_watcher_monitor_add(file);
		f_enum = g_file_enumerate_children(file, attr, G_FILE_QUERY_INFO_NONE,
			NULL, NULL);
		if (!f_enum)
			goto exit;
		while((info = g_file_enumerator_next_file(f_enum, NULL, NULL))) {
			g_snprintf(path, sizeof(path), "%s%s", base_path,
				g_file_info_get_name(info));
			file_watcher_add_watches(path);
			g_object_unref(info);
		}
		g_object_unref(f_enum);
	}
exit:
	g_object_unref(file);
}

void file_watcher_stop_watches(void)
{
	g_hash_table_remove_all(_file_monitors);
}
