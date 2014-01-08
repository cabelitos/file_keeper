#include <glib.h>
#include "file_watcher.h"

int main(int argc, char **argv)
{
	GMainLoop *loop;
	(void) argc;
	loop = g_main_loop_new(NULL, FALSE);
	file_watcher_init();
	file_watcher_add_watches(argv[1]);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
	file_watcher_shutdown();
	return 0;
}
