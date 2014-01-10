#include <glib.h>
#include <glib/gstdio.h>

gboolean utils_create_dir_if_not_present(const char *path, gboolean *exist)
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
