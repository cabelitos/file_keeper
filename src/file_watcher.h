#ifndef _FILE_WATCHER_H_
#define _FILE_WATCHER_H_

#include <glib.h>

typedef struct _FileWatcher FileWatcher;

typedef enum _File_Watcher_Changed_Type {
	FILE_ADDED,
	FILE_DELETED
} File_Watcher_Changed_Type;

typedef void (*file_watcher_changed_cb)(FileWatcher *watcher,
	File_Watcher_Changed_Type type, const char *path, void *data);

void file_watcher_set_file_watcher_changed_cb(FileWatcher *watcher,
	file_watcher_changed_cb cb, void *cb_data);

FileWatcher *file_watcher_new(void);

GList *file_watcher_get_monitored_files(FileWatcher *watcher);

GList *file_watcher_request_file_versions(FileWatcher *watcher, const char *path);

void file_watcher_free(FileWatcher *watcher);

void file_watcher_stop_watches(FileWatcher *watcher);

gboolean file_watcher_request_revert_file(FileWatcher *watcher, const char *path, gint64 timestamp);

gboolean file_watcher_request_revert_end(FileWatcher *watcher, const char *path, gboolean toHead);

#endif
