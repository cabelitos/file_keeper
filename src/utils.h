#ifndef _UTILS_H_
#define _UTILS_H_

#include <glib.h>

gboolean utils_create_dir_if_not_present(const char *path, gboolean *exist);
char *utils_get_relative_path(const char *base, const char *file);
char *utils_get_file_name(const char *path);
char *utils_remove_file_name_suffix(const char *suffix, const char *name);
char *utils_get_original_file_path(const char *base_path, const char *file_name, const char *current_folder_name);

#endif
