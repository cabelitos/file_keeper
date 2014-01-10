#include <glib.h>
#include <git2.h>
#include "file_watcher.h"

int main(int argc, char **argv)
{
	GMainLoop *loop;

	(void) argc;
	(void) argv;
	git_threads_init();
	loop = g_main_loop_new(NULL, FALSE);
	file_watcher_init();
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	file_watcher_shutdown();
	git_threads_shutdown();
	return 0;
}
