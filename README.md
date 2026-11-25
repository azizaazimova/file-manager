# file-manager

# Virtual File System (In-Memory File Manager)

A C/C++ implementation of a hierarchical in-memory file manager supporting Unix-style path navigation, storage quota management, and recursive directory operations.

## Features

- **Hierarchical Directory Tree:** N-ary tree representation (`first_child` / `next_sibling`) supporting arbitrary nesting depths.
- **Path Resolution:** Full support for absolute (`/dir1/file`) and relative (`./dir1`, `../dir2`) paths with strict validation (32-char name limit, 128-char full path limit).
- **Storage Quota Tracking:** Tracks total and available disk space on file creation and reclaims space upon deletion.
- **Recursive Operations & Metrics:** Supports recursive subtree removal, bulk file deletion (`remove_files`), and tree statistics (`size`, `files_count`, `dir_count`).

## Build & Run Tests

```bash
make test
```
