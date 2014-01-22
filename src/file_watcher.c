#include <gio/gio.h>
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>
#include "file_watcher.h"
#include "file_keeper.h"
#include "utils.h"

#define FILE_WATCHER_FOLDER "file_keeper"
#define EXPIRE_TIME 3000

typedef struct _File_Changed_Info {
	char *path;
	guint f_hash; /* for faster comparison at _file_watcher_file_changed_info_already_marked()*/
	gboolean deleted;
	File_Keeper *keeper;
} File_Changed_Info;

struct _File_Watcher {
	guint timeout_id;
	GSList *changed_files;
	File_Keeper *keeper;
	GHashTable *file_monitors;
};

static void _file_watcher_add_watches(const char *base_path, gboolean commit_changes,
	File_Watcher *watcher);
static void _file_watcher_monitor_add(GFile *file, gboolean is_dir,
	File_Watcher *watcher);

static File_Changed_Info *_file_watcher_file_changed_info_new(char *path,
	guint f_hash, gboolean deleted, File_Keeper *keeper)
{
	File_Changed_Info *info = g_malloc(sizeof(File_Changed_Info));
	info->path = g_strdup(path);
	info->deleted = deleted;
	info->f_hash = f_hash;
	info->keeper = keeper;
	return info;
}

static void _file_watcher_free_changed_info_and_commit(gpointer data)
{
	File_Changed_Info *info = data;
	file_keeper_save_changes(info->keeper, info->path, info->deleted);
	g_free(info->path);
	g_free(info);
}

static void _file_watcher_value_destroy(gpointer data)
{
	GFileMonitor *monitor = data;
	g_file_monitor_cancel(monitor);
	g_object_unref(monitor);
}

File_Watcher *file_watcher_new(void)
{
	const char *home;
	char path[FILE_KEEPER_PATH_MAX];
	File_Watcher *watcher;
	File_Keeper *keeper;

	home = g_get_home_dir();
	g_snprintf(path, sizeof(path), "%s%c%s", home, G_DIR_SEPARATOR,
		FILE_WATCHER_FOLDER);

	utils_create_dir_if_not_present(path, NULL);
	keeper = file_keeper_new(path);
	g_return_val_if_fail(keeper, NULL);

	watcher = g_malloc0(sizeof(File_Watcher));
	watcher->keeper = keeper;

	watcher->file_monitors = g_hash_table_new_full(g_direct_hash,
		g_direct_equal, NULL, _file_watcher_value_destroy);
	_file_watcher_add_watches(path, TRUE, watcher);
	file_keeper_commit_deleted_files(watcher->keeper);
	return watcher;
}

void file_watcher_free(File_Watcher *watcher)
{
	g_return_if_fail(watcher);
	if (watcher->timeout_id)
		g_source_remove(watcher->timeout_id);
	g_hash_table_destroy(watcher->file_monitors);
	g_slist_free_full(watcher->changed_files,
		_file_watcher_free_changed_info_and_commit);
	file_keeper_free(watcher->keeper);
	g_free(watcher);
}

static gboolean _file_watcher_save_timeout(gpointer data)
{
	File_Watcher *watcher = data;
	watcher->timeout_id = 0;
	g_slist_free_full(watcher->changed_files,
		_file_watcher_free_changed_info_and_commit);
	watcher->changed_files = NULL;
	return FALSE;
}

static File_Changed_Info *_file_watcher_file_changed_info_already_marked(
	guint f_hash, GSList *head)
{
	GSList *itr;
	File_Changed_Info *info;

	for (itr = head; itr; itr = itr->next) {
		info = itr->data;
		if (info->f_hash == f_hash)
			return info;
	}
	return NULL;
}

static gboolean _file_watcher_file_really_deleted(const char *path,
	File_Watcher *watcher)
{
	GFile *file;

	if (!g_file_test(path, G_FILE_TEST_EXISTS))
		return TRUE;

	file = g_file_new_for_path(path);
	_file_watcher_monitor_add(file, FALSE, watcher);
	g_object_unref(file);
	return FALSE;
}

static void _file_watcher_monitor_changed(GFileMonitor *monitor, GFile *file,
	GFile *other, GFileMonitorEvent event, gpointer data)
{
	char *path;
	GFileType type;
	gboolean deleting = FALSE;
	guint f_hash;
	File_Changed_Info *info;
	File_Watcher *watcher = data;

	if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
		event == G_FILE_MONITOR_EVENT_UNMOUNTED ||
		event == G_FILE_MONITOR_EVENT_PRE_UNMOUNT)
			return;

	path = g_file_get_path(file);
	g_return_if_fail(path);
	f_hash = g_file_hash(file);

	if (g_str_has_prefix(path, file_keeper_db_path_get(watcher->keeper))) {
		g_free(path);
		return;
	}

	type = g_file_query_file_type(file, G_FILE_QUERY_INFO_NONE, NULL);
	if (event == G_FILE_MONITOR_EVENT_DELETED) {
		g_hash_table_remove(watcher->file_monitors, GUINT_TO_POINTER(f_hash));
		/* When we save a binary file (JPG, doc, etc) the original file is deleted and
			a new one is created. This function will help to check if the file was really
			deleted. Because if does not we need to create another file monitor to it.
		*/
		deleting = _file_watcher_file_really_deleted(path, watcher);
		if (!deleting)
			file_keeper_recreate_file_link(watcher->keeper, path);
	} else if (event == G_FILE_MONITOR_EVENT_CREATED)
			_file_watcher_add_watches(path, FALSE, watcher);

	/* We cannot track directories, only the files in it. */
	if (type == G_FILE_TYPE_DIRECTORY) {
		g_free(path);
		return;
	}

	info = _file_watcher_file_changed_info_already_marked(f_hash,
		watcher->changed_files);
	if (deleting && info)
			info->deleted = TRUE;
	else if ((deleting ||
		file_keeper_file_content_has_changed(watcher->keeper, path)) &&
		!info) {
		watcher->changed_files = g_slist_prepend(watcher->changed_files,
			_file_watcher_file_changed_info_new(path, f_hash, deleting,
				watcher->keeper));
	}

	if (!watcher->timeout_id && g_slist_length(watcher->changed_files) > 0)
		watcher->timeout_id = g_timeout_add(EXPIRE_TIME,
			_file_watcher_save_timeout, watcher);
	g_free(path);
	(void) other;
	(void) monitor;
}

static void _file_watcher_monitor_add(GFile *file, gboolean is_dir, File_Watcher *watcher)
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
	g_hash_table_insert(watcher->file_monitors,
		GUINT_TO_POINTER(g_file_hash(file)), monitor);
	g_signal_connect(monitor, "changed",
		G_CALLBACK(_file_watcher_monitor_changed), watcher);
}


static void _file_watcher_add_watches(const char *base_path,
	gboolean commit_changes, File_Watcher *watcher)
{
	char path[FILE_KEEPER_PATH_MAX];
	char attr[FILE_KEEPER_PATH_MAX];
	char *f_path;
	GFile *file;
	GFileInfo *info;
	GFileEnumerator *f_enum;
	GFileType type;

	if (g_str_has_prefix(base_path, file_keeper_db_path_get(watcher->keeper)))
		return;

	file = g_file_new_for_path(base_path);
	g_snprintf(attr, sizeof(attr), "%s,%s", G_FILE_ATTRIBUTE_STANDARD_NAME,
		G_FILE_ATTRIBUTE_STANDARD_TYPE);
	info = g_file_query_info(file, attr, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	if (!info)
		goto exit;
	type = g_file_info_get_file_type(info);
	g_object_unref(info);
	if (type == G_FILE_TYPE_DIRECTORY) {
		_file_watcher_monitor_add(file, TRUE, watcher);
		f_enum = g_file_enumerate_children(file, attr, G_FILE_QUERY_INFO_NONE,
			NULL, NULL);
		if (!f_enum)
			goto exit;
		while((info = g_file_enumerator_next_file(f_enum, NULL, NULL))) {
			if (!info)
				continue;
			g_snprintf(path, sizeof(path), "%s%c%s", base_path,
				G_DIR_SEPARATOR, g_file_info_get_name(info));
			_file_watcher_add_watches(path, commit_changes, watcher);
			g_object_unref(info);
		}
		g_object_unref(f_enum);
	} else if (type == G_FILE_TYPE_REGULAR || type == G_FILE_TYPE_SHORTCUT ||
		type == G_FILE_TYPE_SYMBOLIC_LINK) {
		if (commit_changes) {
			f_path = g_file_get_path(file);
			if (file_keeper_file_content_has_changed(watcher->keeper, f_path))
				file_keeper_save_changes(watcher->keeper, f_path, FALSE);
			file_keeper_add_tracked_file(watcher->keeper, f_path);
			g_free(f_path);
		}
		_file_watcher_monitor_add(file, commit_changes, watcher);
	}
exit:
	g_object_unref(file);
}

void file_watcher_stop_watches(File_Watcher *watcher)
{
	g_return_if_fail(watcher);
	g_hash_table_remove_all(watcher->file_monitors);
}
