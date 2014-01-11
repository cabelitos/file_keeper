#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>
#include "file_keeper.h"
#include "utils.h"

#ifdef G_OS_WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

struct _File_Keeper {
	char *path;
};

#define FILE_KEEPER_DB_FOLDER ".db"

static char * _file_keeper_get_relative_path(const char *base, const char *file)
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

static char * _file_keeper_get_file_name(const char *path)
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

static gboolean _file_keeper_create_commit(git_repository *repo, const char *path,
	const char *msg, gboolean deleting)
{
	git_signature *sig;
	git_index *index;
    git_oid tree_id, commit_id;
    git_commit *commit = NULL;
    git_tree *tree;
    git_reference *head = NULL;
    unsigned int parents = 0;
        
	if (git_signature_now(&sig, g_get_user_name(), g_get_host_name()) < 0)
		return FALSE;
	if (git_repository_index(&index, repo) < 0)
		return FALSE;
	if (deleting) {
		if (git_index_remove_bypath(index, path) < 0)
			return FALSE;
	} else {
		if (git_index_add_bypath(index, path) < 0)
			return FALSE;
	}
	if (git_index_write(index) < 0)
		return FALSE;
	if (!git_repository_head(&head, repo)) {
		if (git_commit_lookup(&commit, repo, git_reference_target(head)) < 0 )
			return FALSE;
		parents = 1;
	}

	if (git_index_write_tree(&tree_id, index) < 0)
		return FALSE;
	if (git_tree_lookup(&tree, repo, &tree_id) < 0)
		return FALSE;
	if (git_commit_create_v(&commit_id, repo, "HEAD", sig, sig,
		NULL, msg, tree, parents, commit) < 0)
		return FALSE;
	if (commit)
		git_commit_free(commit);
	if (head)
		git_reference_free(head);
	git_index_free(index);
	git_tree_free(tree);
	git_signature_free(sig);
	return TRUE;
}

File_Keeper *file_keeper_new(const char *base_path)
{
	char path[FILE_KEEPER_PATH_MAX];
	File_Keeper *keeper;
	
	g_snprintf(path, sizeof(path), "%s%c%s", base_path, G_DIR_SEPARATOR,
		FILE_KEEPER_DB_FOLDER);
	if (!utils_create_dir_if_not_present(path, NULL))
		return NULL;
	keeper = g_malloc(sizeof(File_Keeper));
	keeper->path = g_strdup(path);
	return keeper;
}

void file_keeper_free(File_Keeper *keeper)
{
	g_return_if_fail(keeper);
	g_free(keeper->path);
	g_free(keeper);
}

const char *file_keeper_db_path_get(File_Keeper *keeper)
{
		g_return_val_if_fail(keeper, NULL);
		return keeper->path;
}

static gboolean _file_keeper_create_hard_link(const char *p1, const char *p2)
{
#ifdef G_OS_WIN32
	/* TODO ! */
	return FALSE;
#else
	if (link(p1, p2) < 0)
		return FALSE;
	return TRUE;
#endif
}

gboolean file_keeper_save_changes(File_Keeper *keeper, const char *path,
	gboolean deleting)
{
	gboolean r = FALSE;
	char *rel_path, *name;
	gboolean exist;
	char file_db_path[FILE_KEEPER_PATH_MAX];
	char final_path[FILE_KEEPER_PATH_MAX];
	char commit_msg[50];
	git_repository *repo = NULL;

	g_return_val_if_fail(path, FALSE);
	g_return_val_if_fail(keeper, FALSE);

	name = _file_keeper_get_file_name(path);
	g_snprintf(file_db_path, sizeof(file_db_path), "%s%c%s-changes", keeper->path,
		G_DIR_SEPARATOR, name);

	if (!utils_create_dir_if_not_present(file_db_path, &exist))
		goto exit;

	g_snprintf(final_path, sizeof(final_path), "%s%c%s", file_db_path,
		G_DIR_SEPARATOR, name);

	if (deleting) {
		if (g_remove(final_path) < 0) {
			goto exit;
		}
	}

	if (!exist) {
		if (!_file_keeper_create_hard_link(path, final_path))
			goto exit;
			if (git_repository_init(&repo, file_db_path, 0) < 0)
				goto exit;
			g_snprintf(commit_msg, sizeof(commit_msg), "Initing");
	} else {
		if (git_repository_open(&repo, file_db_path) < 0)
			goto exit;
		g_snprintf(commit_msg, sizeof(commit_msg), "Changing");
	}

	rel_path = _file_keeper_get_relative_path(file_db_path, final_path);
	r = _file_keeper_create_commit(repo, rel_path, commit_msg, deleting);
	git_repository_free(repo);
	g_free(rel_path);

exit:
	g_free(name);
	return r;
}



