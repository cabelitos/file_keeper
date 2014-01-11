#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include "file_watcher.h"
#include "file_keeper.h"
#include "utils.h"

static GHashTable *_file_monitors = NULL;
static int _refcount = 0;
static guint _timeout_id = 0;
static File_Keeper *_keeper = NULL;
static GSList *_changed_files = NULL;

#define FILE_WATCHER_FOLDER "file_keeper"
#define EXPIRE_TIME 3000

typedef struct _File_Changed_Info {
	char *path;
	guint f_hash; /* for faster comparison at _file_watcher_file_changed_info_already_marked()*/
	gboolean deleted;
} File_Changed_Info;

static File_Changed_Info *_file_watcher_file_changed_info_new(char *path,
	guint f_hash, gboolean deleted)
{
	File_Changed_Info *info = g_malloc(sizeof(File_Changed_Info));
	info->path = g_strdup(path);
	info->deleted = deleted;
	info->f_hash = f_hash;
	return info;
}

static void _file_watcher_free_changed_info_and_commit(gpointer data)
{
	File_Changed_Info *info = data;
	file_keeper_save_changes(_keeper, info->path, info->deleted);
	g_free(info->path);
	g_free(info);
}

static void _file_watcher_value_destroy(gpointer data)
{
	g_object_unref(data);
}

void file_watcher_init(void)
{
	const char *home;
	char path[FILE_KEEPER_PATH_MAX];

	if (_refcount++ > 1)
		return;
	home = g_get_home_dir();
	g_snprintf(path, sizeof(path), "%s%c%s", home, G_DIR_SEPARATOR,
		FILE_WATCHER_FOLDER);

	utils_create_dir_if_not_present(path, NULL);
	_keeper = file_keeper_new(path);
	g_return_if_fail(_keeper);

	_file_monitors = g_hash_table_new_full(g_direct_hash, g_direct_equal,
		NULL, _file_watcher_value_destroy);
	file_watcher_add_watches(path);
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
	if (_timeout_id)
		g_source_remove(_timeout_id);
	g_hash_table_destroy(_file_monitors);
	g_slist_free_full(_changed_files,
		_file_watcher_free_changed_info_and_commit);
	file_keeper_free(_keeper);
	_file_monitors = NULL;
	_keeper = NULL;
	_changed_files = NULL;
	_timeout_id = 0;
}

static gboolean _file_watcher_save_timeout(gpointer data)
{
	_timeout_id = 0;
	g_slist_free_full(_changed_files,
		_file_watcher_free_changed_info_and_commit);
	_changed_files = NULL;
	(void)data;
	return FALSE;
}

static gboolean _file_watcher_file_changed_info_already_marked(guint f_hash)
{
	GSList *itr = _changed_files;
	File_Changed_Info *info;

	for (itr = _changed_files; itr; itr = itr->next) {
		info = itr->data;
		if (info->f_hash == f_hash)
			return TRUE;
	}
	return FALSE;
}

static void _file_watcher_monitor_changed(GFileMonitor *monitor, GFile *file,
	GFile *other, GFileMonitorEvent event, gpointer data)
{
	char *path;
	GFileType type;
	gboolean deleting = FALSE;
	guint f_hash;

	if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
		event == G_FILE_MONITOR_EVENT_UNMOUNTED ||
		event == G_FILE_MONITOR_EVENT_PRE_UNMOUNT)
			return;

	path = g_file_get_path(file);
	g_return_if_fail(path);
	f_hash = g_file_hash(file);

	if (g_str_has_prefix(path, file_keeper_db_path_get(_keeper))) {
		g_free(path);
		return;
	}

	type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
	if (event == G_FILE_MONITOR_EVENT_DELETED) {
		g_hash_table_remove(_file_monitors, GINT_TO_POINTER(f_hash));
		deleting = TRUE;
	} else if (event == G_FILE_MONITOR_EVENT_CREATED)
			file_watcher_add_watches(path);

	/* We cannot track directories, only the files in it. */
	if (type == G_FILE_TYPE_DIRECTORY) {
		g_free(path);
		return;
	}
	if (_timeout_id)
		g_source_remove(_timeout_id);

	if (file_keeper_file_content_has_changed(_keeper, path) &&
		!_file_watcher_file_changed_info_already_marked(f_hash)) {
		_changed_files = g_slist_prepend(_changed_files,
			_file_watcher_file_changed_info_new(path, f_hash, deleting));
	}

	_timeout_id = g_timeout_add(EXPIRE_TIME, _file_watcher_save_timeout, NULL);
	g_free(path);
	(void) other;
	(void) monitor;
	(void) data;
}

static void _file_watcher_monitor_add(GFile *file, gboolean is_dir)
{
	GFileMonitor *monitor;

	if (is_dir) {
		monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL,
			NULL);
	} else {
		monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, NULL, NULL);
	}

	if (!monitor)
		return;
	g_hash_table_insert(_file_monitors, GINT_TO_POINTER(g_file_hash(file)),
		monitor);
	g_signal_connect(monitor, "changed",
		G_CALLBACK(_file_watcher_monitor_changed), NULL);
}

void file_watcher_add_watches(const char *base_path)
{
	char path[FILE_KEEPER_PATH_MAX];
	char attr[FILE_KEEPER_PATH_MAX];
	const char *name;
	GFile *file;
	GFileInfo *info;
	GFileEnumerator *f_enum;
	GFileType type;

	if (g_str_has_prefix(base_path, file_keeper_db_path_get(_keeper)))
		return;

	file = g_file_new_for_path(base_path);
	g_snprintf(attr, sizeof(attr), "%s,%s", G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_ATTRIBUTE_STANDARD_TYPE);
	info = g_file_query_info(file, attr, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (!info)
		goto exit;
	type = g_file_info_get_file_type(info);
	name = g_file_info_get_name(info);
	g_object_unref(info);
	if (type == G_FILE_TYPE_DIRECTORY) {
		_file_watcher_monitor_add(file, TRUE);
		f_enum = g_file_enumerate_children(file, attr, G_FILE_QUERY_INFO_NONE,
			NULL, NULL);
		if (!f_enum)
			goto exit;
		while((info = g_file_enumerator_next_file(f_enum, NULL, NULL))) {
			g_snprintf(path, sizeof(path), "%s%c%s", base_path,
				G_DIR_SEPARATOR, g_file_info_get_name(info));
			file_watcher_add_watches(path);
			g_object_unref(info);
		}
		g_object_unref(f_enum);
	} else if (type == G_FILE_TYPE_REGULAR || type == G_FILE_TYPE_SHORTCUT ||
		type == G_FILE_TYPE_SYMBOLIC_LINK)
		_file_watcher_monitor_add(file, FALSE);
exit:
	g_object_unref(file);
}

void file_watcher_stop_watches(void)
{
	g_hash_table_remove_all(_file_monitors);
}
