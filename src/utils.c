#include <glib/gstdio.h>
#include <string.h>

gboolean
utils_create_dir_if_not_present(const char *path, gboolean *exist)
{
	gboolean e = TRUE;
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		if (g_mkdir(path, 0777))
			return FALSE;
		e = FALSE;
	}
	if (exist)
		*exist = e;
	return TRUE;
}

char *
utils_get_relative_path(const char *base, const char *file)
{
	size_t r_size, start;
	char *r;

	/* +1 to skip the '/' */
	start = strlen(base) + 1;
	r_size = (strlen(file) - start) + 1;
	file += start;
	r = g_malloc(r_size);
	memcpy(r, file, r_size);
	return r;
}

char *
utils_get_file_name(const char *path)
{
	size_t size = strlen(path), i;
	char *r;
	i = size;
	while(path[i-1] != G_DIR_SEPARATOR)
		i--;
	path += i;
	r = g_malloc((size - i) + 1);
	memcpy(r, path, (size - i) + 1);
	return r;
}

char *
utils_remove_file_name_suffix(const char *suffix, const char *name)
{
	size_t total;
	char *r;

	total = strlen(name) - strlen(suffix) + 1;
	r = g_malloc(total);
	memcpy(r, name, total);
	r[total - 1] = '\0';
	return r;
}

char *
utils_get_original_file_path(const char *base_path, const char *file_name,
	const char *current_folder_name)
{
	size_t stop;
	char *r;

	stop = strlen(base_path) - strlen(current_folder_name);
	r = g_malloc(stop + strlen(file_name) + 1);
	memcpy(r, base_path, stop);
	memcpy(r + stop, file_name, strlen(file_name) + 1);
	return r;
}
