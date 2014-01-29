#ifndef _FILE_WATCHER_H_
#define _FILE_WATCHER_H_

#include <glib.h>

typedef struct _FileWatcher FileWatcher;

FileWatcher *file_watcher_new(void);

GList *file_watcher_get_monitored_files(FileWatcher *watcher);

GList *file_watcher_request_file_versions(FileWatcher *watcher, const char *path);

void file_watcher_free(FileWatcher *watcher);

void file_watcher_stop_watches(FileWatcher *watcher);

#endif
