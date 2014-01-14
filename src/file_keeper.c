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
	GHashTable *tracked_files;
};

#define FILE_KEEPER_DB_FOLDER ".db"
#define FILE_KEEPER_SUFFIX "-changes"

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

static char *_file_keeper_remove_file_name_suffix(const char *name)
{
	size_t total;
	char *r;

	total = strlen(name) - strlen(FILE_KEEPER_SUFFIX) + 1;
	r = g_malloc(total);
	memcpy(r, name, total);
	r[total - 1] = '\0';
	return r;
}

static char * _file_keeper_get_original_file_path(char *base_path, char *file_name)
{
	size_t stop;
	char *r;

	stop = strlen(base_path) - strlen(FILE_KEEPER_DB_FOLDER);
	r = g_malloc(stop + strlen(file_name) + 1);
	memcpy(r, base_path, stop);
	memcpy(r + stop, file_name, strlen(file_name) + 1);
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

static void _file_keeper_key_destroy(gpointer data)
{
	g_free(data);
}

File_Keeper *file_keeper_new(const char *base_path)
{
	char path[FILE_KEEPER_PATH_MAX];
	File_Keeper *keeper;
	
	g_snprintf(path, sizeof(path), "%s%c%s", base_path, G_DIR_SEPARATOR,
		FILE_KEEPER_DB_FOLDER);
	if (!utils_create_dir_if_not_present(path, NULL))
		return NULL;
	keeper = g_malloc0(sizeof(File_Keeper));
	keeper->path = g_strdup(path);
	keeper->tracked_files = g_hash_table_new_full(g_str_hash, g_str_equal,
		_file_keeper_key_destroy, NULL);
	return keeper;
}

void file_keeper_free(File_Keeper *keeper)
{
	g_return_if_fail(keeper);
	if (keeper->tracked_files)
		file_keeper_commit_deleted_files(keeper);
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
	return CreateHardLink(p2, p1, NULL);
#else
	if (link(p1, p2) < 0)
		return FALSE;
	return TRUE;
#endif
}

static void _file_keeper_get_file_paths(const char *base_path,
	const char *file_path, char **link_db_path, char **link_file)
{
	char db_path[FILE_KEEPER_PATH_MAX];
	char linked_file_path[FILE_KEEPER_PATH_MAX];
	char *name = _file_keeper_get_file_name(file_path);

	g_snprintf(db_path, sizeof(db_path), "%s%c%s%s", base_path,
		G_DIR_SEPARATOR, name, FILE_KEEPER_SUFFIX);
	g_snprintf(linked_file_path, sizeof(linked_file_path), "%s%c%s", db_path,
		G_DIR_SEPARATOR, name);
	g_free(name);
	*link_db_path = g_strdup(db_path);
	*link_file = g_strdup(linked_file_path);
}

static gboolean _file_keeper_repo_has_unstaged_changes(const char *path)
{
	gboolean r = FALSE;
	git_repository *repo;
	git_status_list *status;
	git_status_options statusopt;

	memset(&statusopt, 0, sizeof(git_status_options));

	statusopt.version = GIT_STATUS_OPTIONS_VERSION;
	statusopt.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	statusopt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
		GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
		GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

	if (git_repository_open(&repo, path) < 0)
		return r;
	if (git_status_list_new(&status, repo, &statusopt) < 0) {
		git_repository_free(repo);
		return r;
	}
	if (git_status_list_entrycount(status) > 0)
		r = TRUE;
	git_status_list_free(status);
	git_repository_free(repo);
	return r;
}

gboolean file_keeper_file_content_has_changed(File_Keeper *keeper,
	const char *path)
{
	char *db_path, *file_path;
	gboolean r;

	g_return_val_if_fail(path, FALSE);
	g_return_val_if_fail(keeper, FALSE);

	_file_keeper_get_file_paths(keeper->path, path, &db_path, &file_path);

	/* We do not track this file yet, we must create the repo ! */
	if (!g_file_test(db_path, G_FILE_TEST_EXISTS))
		r = TRUE;
	else
		r = _file_keeper_repo_has_unstaged_changes(db_path);
	g_free(file_path);
	g_free(db_path);
	return r;
}

void file_keeper_add_tracked_file(File_Keeper *keeper, const char *path)
{
	g_return_if_fail(keeper);

	g_hash_table_add(keeper->tracked_files, _file_keeper_get_file_name(path));
}

static gboolean _file_keeper_prepare_commit_changes(const char *final_path,
	const char *original_path, const char *file_db_path, gboolean deleting,
	gboolean exist)
{
	char commit_msg[50];
	char *rel_path;
	git_repository *repo;
	gboolean r;

	if (deleting) {
		if (g_remove(final_path) < 0) {
			return FALSE;
		}
	}

	if (!exist) {
		if (!_file_keeper_create_hard_link(original_path, final_path))
			return FALSE;
			if (git_repository_init(&repo, file_db_path, 0) < 0)
				return FALSE;
			g_snprintf(commit_msg, sizeof(commit_msg), "Initing");
	} else {
		if (git_repository_open(&repo, file_db_path) < 0)
			return FALSE;
		g_snprintf(commit_msg, sizeof(commit_msg), "Changing");
	}

	rel_path = _file_keeper_get_relative_path(file_db_path, final_path);
	r = _file_keeper_create_commit(repo, rel_path, commit_msg, deleting);
	git_repository_free(repo);
	g_free(rel_path);
	return r;
}

void file_keeper_commit_deleted_files(File_Keeper *keeper)
{
	GFile *db_dir;
	GFileEnumerator *f_enum;
	GFileInfo *info;
	char attr[50];
	char *name_no_suffix, *file_db_path, *final_path, *original_path;

	g_return_if_fail(keeper);

	db_dir = g_file_new_for_path(keeper->path);

	g_snprintf(attr, sizeof(attr), "%s", G_FILE_ATTRIBUTE_STANDARD_NAME);
	f_enum = g_file_enumerate_children(db_dir, attr, G_FILE_QUERY_INFO_NONE,
		NULL, NULL);

	if (f_enum) {
		while((info = g_file_enumerator_next_file(f_enum, NULL, NULL))) {
			if (!info)
				continue;
			name_no_suffix = _file_keeper_remove_file_name_suffix(
				g_file_info_get_name(info));

			/* The file was deleted! */
			if (!g_hash_table_contains(keeper->tracked_files, name_no_suffix)) {
				original_path = _file_keeper_get_original_file_path(keeper->path,
					name_no_suffix);
				_file_keeper_get_file_paths(keeper->path, original_path, &file_db_path,
					&final_path);
				/* second argument can be NULL here, because we won't use it in the case
				that the file exist */
				_file_keeper_prepare_commit_changes(final_path, NULL,
					file_db_path, TRUE, TRUE);
				g_free(file_db_path);
				g_free(final_path);
				g_free(original_path);
			}
			g_free(name_no_suffix);
			g_object_unref(info);
		}
		g_object_unref(f_enum);
	}
	g_object_unref(db_dir);
	g_hash_table_destroy(keeper->tracked_files);
	keeper->tracked_files = NULL;
}

void file_keeper_recreate_file_link(File_Keeper *keeper, char *path)
{
	char *file_db_path, *final_path;

	g_return_if_fail(path);
	g_return_if_fail(keeper);
	_file_keeper_get_file_paths(keeper->path, path, &file_db_path,
		&final_path);

	g_unlink(final_path);
	_file_keeper_create_hard_link(path, final_path);

	g_free(file_db_path);
	g_free(final_path);
}

gboolean file_keeper_save_changes(File_Keeper *keeper, const char *path,
	gboolean deleting)
{
	gboolean r = FALSE;
	char *file_db_path, *final_path;
	gboolean exist;

	g_return_val_if_fail(path, FALSE);
	g_return_val_if_fail(keeper, FALSE);

	_file_keeper_get_file_paths(keeper->path, path, &file_db_path,
		&final_path);

	if (!utils_create_dir_if_not_present(file_db_path, &exist))
		goto exit;

	r = _file_keeper_prepare_commit_changes(final_path, path, file_db_path,
			deleting, exist);
exit:
	g_free(final_path);
	g_free(file_db_path);
	return r;
}




