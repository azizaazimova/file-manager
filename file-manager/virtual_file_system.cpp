#include <iostream>
#include <stdio.h>
#include "os_file_ex.h"
#include <string.h>
#include <stdlib.h>

static int available_disk_space = 0;
static int deleted_files_counter = 0;

typedef struct fs_node
{
    char* name;
    fs_node* parent;
    fs_node* first_child;  // NULL for files
    fs_node* next_sibling;
    int size;              // 0 for directories, > 0 for files
} fs_node;

typedef struct cwd_state
{
    fs_node* current;
    int path_len;
} cwd_state;

static fs_node* root_dir = NULL;
static cwd_state* cwd = NULL;

static void build_full_path(fs_node* node, char* dst_path)
{
    if (node->parent)
    {
        build_full_path(node->parent, dst_path);
        strcat(dst_path, "/");
        strcat(dst_path, node->name);
    }
}

// Validates path length limits, allowed characters, and duplicate names in parent_dir
static int is_valid_new_entry(fs_node* parent_dir, const char* path)
{
    if ((cwd->current != root_dir) || (path[0] != '/'))
    {
        if ((cwd->current != root_dir) && (path[0] != '/'))
        {
            if (strlen(path) + cwd->path_len + 1 > 128) return 0;
        }
        else if (strlen(path) + cwd->path_len > 128) return 0;
    }
    else if (strlen(path) > 128) return 0;

    const char* entry_name = path;
    if (strchr(path, '/'))
    {
        entry_name = strrchr(path, '/') + 1;
    }

    if ((strlen(entry_name) <= 32) && (entry_name[0] != '\0') &&
        (strcmp(entry_name, ".")) && (strcmp(entry_name, "..")))
    {
        int i = 0;
        while (entry_name[i] != '\0')
        {
            if ((entry_name[i] == '.') || (entry_name[i] == '_') ||
                (entry_name[i] >= '0' && entry_name[i] <= '9') ||
                (entry_name[i] >= 'A' && entry_name[i] <= 'Z') ||
                (entry_name[i] >= 'a' && entry_name[i] <= 'z'))
            {
                i++;
                continue;
            }
            return 0;
        }

        fs_node* sibling = parent_dir->first_child;
        while (sibling)
        {
            if (strcmp(entry_name, sibling->name))
            {
                sibling = sibling->next_sibling;
                continue;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

// Resolves absolute or relative paths; if parent_only is non-zero, returns the target parent directory
static fs_node* resolve_path(const char* path, int parent_only)
{
    if (cwd)
    {
        if (strcmp(path, "/") == 0)
        {
            if (parent_only) return NULL;
            return root_dir;
        }

        if ((strlen(path) <= 128) && (path[strlen(path) - 1] != '/') && (strncmp(path, "/..", 3) != 0))
        {
            int segment_len = 0;
            const char* cursor = path;
            char* segment = NULL;
            fs_node* curr_node = cwd->current;

            if (strncmp(cursor, "/", 1) == 0)
            {
                cursor += 1;
                curr_node = root_dir;
            }

            while (strchr(cursor, '/'))
            {
                if (strncmp(cursor, "../", 3) == 0)
                {
                    cursor += 3;
                    if (curr_node != root_dir)
                    {
                        curr_node = curr_node->parent;
                        continue;
                    }
                    return NULL;
                }
                else if (strncmp(cursor, "./", 2) == 0)
                {
                    cursor += 2;
                    continue;
                }
                else if (strncmp(cursor, "/", 1) == 0)
                {
                    if (segment) free(segment);
                    return NULL;
                }

                segment_len = strchr(cursor, '/') - cursor;
                char* resized = (char*)realloc(segment, segment_len + 1);
                if (resized)
                {
                    segment = resized;
                    strncpy(segment, cursor, segment_len);
                    segment[segment_len] = '\0';
                }
                else
                {
                    if (segment) free(segment);
                    return NULL;
                }

                fs_node* child = curr_node->first_child;
                if ((segment) && (child))
                {
                    while (child)
                    {
                        if (strcmp(child->name, segment))
                        {
                            child = child->next_sibling;
                            continue;
                        }
                        break;
                    }
                    if (child)
                    {
                        curr_node = child;
                        cursor += segment_len + 1;
                        continue;
                    }
                }
                if (segment) free(segment);
                return NULL;
            }

            if (segment) free(segment);

            if (parent_only)
            {
                if (curr_node->size) return NULL; // Parent cannot be a file
                return curr_node;
            }

            if (strcmp(cursor, "..") == 0) return curr_node->parent;
            else if (strcmp(cursor, ".") == 0) return curr_node;

            curr_node = curr_node->first_child;
            while (curr_node)
            {
                if (strcmp(curr_node->name, cursor) != 0)
                {
                    curr_node = curr_node->next_sibling;
                    continue;
                }
                return curr_node;
            }
        }
    }
    return NULL;
}

static void unlink_from_siblings(fs_node* target)
{
    fs_node* curr = target->parent->first_child;
    if (target == curr)
    {
        target->parent->first_child = target->parent->first_child->next_sibling;
        return;
    }
    while (curr)
    {
        if (target == curr->next_sibling)
        {
            curr->next_sibling = target->next_sibling;
            break;
        }
        curr = curr->next_sibling;
    }
}

static void delete_files_recursively(fs_node* dir)
{
    fs_node* next_node;
    fs_node* child = dir->first_child;
    while (child)
    {
        next_node = child->next_sibling;
        delete_files_recursively(child);
        if (child->size)
        {
            unlink_from_siblings(child);
            free(child->name);
            available_disk_space += child->size;
            deleted_files_counter++;
            free(child);
        }
        child = next_node;
    }
}

int vfs_remove_files(const char* path)
{
    fs_node* target_dir = resolve_path(path, 0);
    if (target_dir)
    {
        if (target_dir->size == 0)
        {
            delete_files_recursively(target_dir);
            int removed_count = deleted_files_counter;
            deleted_files_counter = 0;
            return removed_count;
        }
    }
    return -1;
}

static void delete_subtree_recursively(fs_node* node)
{
    fs_node* next_node;
    while (node->first_child)
    {
        if (cwd->current == node->first_child)
        {
            cwd->current = root_dir;
            cwd->path_len = 1;
        }
        next_node = node->first_child->next_sibling;
        delete_subtree_recursively(node->first_child);
        free(node->first_child->name);
        available_disk_space += node->first_child->size;
        free(node->first_child);
        node->first_child = next_node;
    }
}

int vfs_create(int disk_size)
{
    if (root_dir) return 0;

    root_dir = (fs_node*)malloc(sizeof(fs_node));
    cwd = (cwd_state*)malloc(sizeof(cwd_state));
    if ((!root_dir) || (!cwd) || (disk_size <= 0))
    {
        if (root_dir) { free(root_dir); root_dir = NULL; }
        if (cwd) { free(cwd); cwd = NULL; }
        return 0;
    }

    root_dir->name = (char*)malloc(2);
    if (root_dir->name)
    {
        strcpy(root_dir->name, "/");
        root_dir->parent = NULL;
        root_dir->first_child = NULL;
        root_dir->next_sibling = NULL;
        root_dir->size = 0;

        cwd->current = root_dir;
        cwd->path_len = 1;
        available_disk_space = disk_size;
        return 1;
    }

    free(root_dir);
    free(cwd);
    root_dir = NULL;
    cwd = NULL;
    return 0;
}

int vfs_remove(const char* path, int recursive)
{
    fs_node* target = resolve_path(path, 0);
    if (target)
    {
        if ((target->first_child == NULL) || (recursive))
        {
            delete_subtree_recursively(target);
            if (target == root_dir)
            {
                root_dir = NULL;
                free(cwd);
                cwd = NULL;
            }
            else
            {
                if (cwd->current == target)
                {
                    cwd->current = root_dir;
                    cwd->path_len = 1;
                }
                fs_node* curr = target->parent->first_child;
                if (target == curr)
                {
                    target->parent->first_child = target->parent->first_child->next_sibling;
                    curr = NULL;
                }
                while (curr)
                {
                    if (target == curr->next_sibling)
                    {
                        curr->next_sibling = target->next_sibling;
                    }
                    curr = curr->next_sibling;
                }
            }
            free(target->name);
            available_disk_space += target->size;
            free(target);
            target = NULL;
            return 1;
        }
    }
    return 0;
}

int vfs_destroy()
{
    if (!root_dir) return 0;
    if (root_dir->first_child) return vfs_remove("/", 1);
    else return vfs_remove("/", 0);
}

int vfs_create_dir(const char* path)
{
    fs_node* parent_dir = resolve_path(path, 1);
    if ((parent_dir) && (is_valid_new_entry(parent_dir, path)))
    {
        fs_node* new_dir = (fs_node*)malloc(sizeof(fs_node));
        if (new_dir)
        {
            new_dir->next_sibling = parent_dir->first_child ? parent_dir->first_child : NULL;
            parent_dir->first_child = new_dir;
            const char* dir_name = path;
            if (strchr(path, '/'))
            {
                dir_name = strrchr(path, '/') + 1;
            }
            new_dir->name = (char*)malloc(strlen(dir_name) + 1);
            if (new_dir->name)
            {
                strcpy(new_dir->name, dir_name);
                new_dir->parent = parent_dir;
                new_dir->first_child = NULL;
                new_dir->size = 0;
                return 1;
            }
            parent_dir->first_child = new_dir->next_sibling;
            free(new_dir);
        }
    }
    return 0;
}
int vfs_create_file(const char* path, int file_size)
{
    fs_node* parent_dir = resolve_path(path, 1);
    if ((parent_dir) && (available_disk_space >= file_size) && (file_size > 0) &&
        (is_valid_new_entry(parent_dir, path)))
    {
        fs_node* new_file = (fs_node*)malloc(sizeof(fs_node));
        if (new_file)
        {
            new_file->next_sibling = parent_dir->first_child ? parent_dir->first_child : NULL;
            parent_dir->first_child = new_file;
            const char* file_name = path;
            if (strchr(path, '/'))
            {
                file_name = strrchr(path, '/') + 1;
            }
            new_file->name = (char*)malloc(strlen(file_name) + 1);
            if (new_file->name)
            {
                strcpy(new_file->name, file_name);
                new_file->parent = parent_dir;
                new_file->first_child = NULL;
                new_file->size = file_size;
                available_disk_space -= file_size;
                return 1;
            }
            parent_dir->first_child = new_file->next_sibling;
            free(new_file);
        }
    }
    return 0;
}
void vfs_get_cur_dir(char* dst)
{
    if (!cwd) return;
    if (cwd->current == root_dir)
    {
        strcpy(dst, "/");
        return;
    }
    char path_buf[129];
    path_buf[0] = '\0';
    build_full_path(cwd->current, path_buf);
    strcpy(dst, path_buf);
}
int vfs_change_dir(const char* path)
{
    fs_node* target_dir = resolve_path(path, 0);
    if ((target_dir) && (target_dir->size == 0))
    {
        cwd->current = target_dir;
        char path_buf[129];
        path_buf[0] = '\0';
        vfs_get_cur_dir(path_buf);
        cwd->path_len = strlen(path_buf);
        return 1;
    }
    return 0;
}
int vfs_exists(const char* path)
{
    return resolve_path(path, 0) ? 1 : 0;
}
int vfs_is_file(const char* path)
{
    fs_node* node = resolve_path(path, 0);
    return (node && node->size > 0) ? 1 : 0;
}
int vfs_is_dir(const char* path)
{
    fs_node* node = resolve_path(path, 0);
    return (node && node->size == 0) ? 1 : 0;
}
// --- Additional Metrics & Disk Space Queries ---
static int sum_subtree_size(fs_node* node)
{
    if (!node) return 0;
    if (node->size > 0) return node->size;
    int total = 0;
    fs_node* child = node->first_child;
    while (child)
    {
        total += sum_subtree_size(child);
        child = child->next_sibling;
    }
    return total;
}
static int count_subtree_files(fs_node* node)
{
    if (!node) return 0;
    if (node->size > 0) return 1;
    int total = 0;
    fs_node* child = node->first_child;
    while (child)
    {
        total += count_subtree_files(child);
        child = child->next_sibling;
    }
    return total;
}
static int count_non_empty_subdirs(fs_node* dir)
{
    int total = 0;
    fs_node* child = dir->first_child;
    while (child)
    {
        if (child->size == 0)
        {
            if (child->first_child != NULL)
            {
                total += 1;
            }
            total += count_non_empty_subdirs(child);
        }
        child = child->next_sibling;
    }
    return total;
}
int vfs_free_space()
{
    return root_dir ? available_disk_space : -1;
}
int vfs_size(const char* path)
{
    fs_node* node = resolve_path(path, 0);
    if (!node) return -1;
    return sum_subtree_size(node);
}
int vfs_files_count(const char* path)
{
    fs_node* node = resolve_path(path, 0);
    if (!node || node->size > 0) return -1;
    return count_subtree_files(node);
}
int vfs_dir_count(const char* path)
{
    fs_node* node = resolve_path(path, 0);
    if (!node || node->size > 0) return -1;
    return count_non_empty_subdirs(node);
}
void setup_file_manager(file_manager_t* fm)
{
    fm->create = vfs_create;
    fm->destroy = vfs_destroy;
    fm->create_dir = vfs_create_dir;
    fm->create_file = vfs_create_file;
    fm->remove = vfs_remove;
    fm->remove_files = vfs_remove_files;
    fm->change_dir = vfs_change_dir;
    fm->get_cur_dir = vfs_get_cur_dir;
    fm->exists = vfs_exists;
    fm->is_file = vfs_is_file;
    fm->is_dir = vfs_is_dir;
    fm->free_space = vfs_free_space;
    fm->size = vfs_size;
    fm->files_count = vfs_files_count;
    fm->dir_count = vfs_dir_count;
}