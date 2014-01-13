#ifndef _FILE_KEEPER_H_
#define _FILE_KEEPER_H_

#include <glib.h>
#include <gio/gio.h>

#define FILE_KEEPER_PATH_MAX 4096

typedef struct _File_Keeper File_Keeper;

File_Keeper *file_keeper_new(const char *base_path);

void file_keeper_free(File_Keeper *keeper);

gboolean file_keeper_save_changes(File_Keeper *keeper, const char *path, gboolean deleting);

const char *file_keeper_db_path_get(File_Keeper *keeper);

gboolean file_keeper_file_content_has_changed(File_Keeper *keeper, const char *path);

void file_keeper_add_tracked_file(File_Keeper *keeper, const char *path);

void file_keeper_recreate_file_link(File_Keeper *keeper, char *path);

void file_keeper_commit_deleted_files(File_Keeper *keeper);

#endif
