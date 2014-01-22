#include <glib.h>
#include <git2.h>
#include "file_watcher.h"

int main(int argc, char **argv)
{
	GMainLoop *loop;
	File_Watcher *watcher;

	(void) argc;
	(void) argv;
	watcher = file_watcher_new();
	if (!watcher)
		return -1;
	git_threads_init();
	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	file_watcher_free(watcher);
	git_threads_shutdown();
	return 0;
}
