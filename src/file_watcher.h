#ifndef _FILE_WATCHER_H_
#define _FILE_WATCHER_H_

#include <glib.h>

typedef struct _File_Watcher File_Watcher;

File_Watcher *file_watcher_new(void);

GList *file_watcher_get_monitored_files(File_Watcher *watcher);

void file_watcher_free(File_Watcher *watcher);

void file_watcher_stop_watches(File_Watcher *watcher);

#endif
