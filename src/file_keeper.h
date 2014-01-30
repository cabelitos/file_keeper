#ifndef _FILE_KEEPER_H_
#define _FILE_KEEPER_H_

#include <glib.h>
#include <gio/gio.h>

#define FILE_KEEPER_PATH_MAX 4096

typedef struct _FileKeeper FileKeeper;

FileKeeper *file_keeper_new(const char *base_path);

void file_keeper_free(FileKeeper *keeper);

gboolean file_keeper_save_changes(FileKeeper *keeper, const char *path, gboolean deleting);

const char *file_keeper_db_path_get(FileKeeper *keeper);

gboolean file_keeper_file_content_has_changed(FileKeeper *keeper, const char *path);

void file_keeper_add_tracked_file(FileKeeper *keeper, const char *path);

void file_keeper_recreate_file_link(FileKeeper *keeper, char *path);

void file_keeper_commit_deleted_files(FileKeeper *keeper);

GList *file_keeper_get_file_commits(FileKeeper *keeper, const char *path);

gboolean file_keeper_revert_file(FileKeeper *keeper, const char *path, gint64 timestamp);

gboolean file_keeper_reset_file(FileKeeper *keeper, const char *path, gboolean toHead);

#endif
