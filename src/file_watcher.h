#ifndef _FILE_WATCHER_H_
#define _FILE_WATCHER_H_

#include <glib.h>

void file_watcher_init(void);

void file_watcher_shutdown(void);

void file_watcher_add_watches(const char *base_path);

void file_watcher_stop_watches(void);

#endif
