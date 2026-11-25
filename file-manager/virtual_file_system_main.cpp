#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "os_file_ex.h"

int main()
{
    file_manager_t fm;
    char cur_dir[256];
    const int TOTAL_SIZE = 1000;

    // Test 1: Lifecycle & invalid initialization
    setup_file_manager(&fm);
    assert(fm.create(1000) == 1);
    assert(fm.create_file("test_file", 1001) == 0);
    assert(fm.destroy() == 1);
    assert(fm.destroy() == 0);
    assert(fm.create(1000) == 1);
    assert(fm.create(1000) == 0);
    assert(fm.destroy() == 1);

    // Test 2: Disk space tracking & duplicate checks
    setup_file_manager(&fm);
    assert(fm.create(1000) == 1);
    assert(fm.create_file("file1.txt", 1000) == 1);
    assert(fm.free_space() == 0);
    assert(fm.create_file("file2.txt", 1) == 0);
    assert(fm.remove("file1.txt", 0) == 1);
    assert(fm.remove("file1.txt", 0) == 0);
    assert(fm.free_space() == 1000);
    assert(fm.create_file("file2.txt", 500) == 1);
    assert(fm.create_file("file2.txt", 500) == 0);
    assert(fm.create_file("file3.txt", 500) == 1);
    assert(fm.create_dir("test_dir") == 1);
    assert(fm.remove("file2.txt", 0) == 1);
    assert(fm.destroy() == 1);

    // Test 3: Path resolution (., .., relative & absolute paths) & recursive removal
    setup_file_manager(&fm);
    assert(fm.create(1000) == 1);
    assert(fm.create_dir("dir1") == 1);
    assert(fm.create_dir("dir1/dir11") == 1);
    assert(fm.change_dir("dir1") == 1);
    assert(fm.create_dir("../dir2") == 1);
    assert(fm.create_dir("../dir2/dir3") == 1);
    assert(fm.remove("/dir2/dir3", 0) == 1);
    assert(fm.create_dir("/dir3/dir31") == 0);
    assert(fm.create_dir("../dir3/dir31") == 0);
    assert(fm.create_dir("../dir2") == 0);
    assert(fm.create_file("/dir2/file1", 1) == 1);
    assert(fm.create_dir("/dir2/dir21") == 1);
    assert(fm.create_dir("/dir2/file1") == 0);
    assert(fm.create_dir("../dir2/file1") == 0);
    assert(fm.create_dir("../dir2/file1/dir") == 0);
    assert(fm.create_dir("../dir2/dir22") == 1);
    assert(fm.create_dir("..") == 0);
    assert(fm.create_dir("../dir2/.") == 0);
    assert(fm.change_dir("dir2") == 0);
    assert(fm.change_dir("dir11") == 1);
    assert(fm.remove("../../dir2/file1", 0) == 1);
    assert(fm.create_dir("../../dir2/file1") == 1);
    assert(fm.remove("../../dir2/file1", 0) == 1);
    assert(fm.create_dir("../../dir2/file1") == 1);
    assert(fm.remove("../../dir2/file1", 0) == 1);
    assert(fm.create_file("../../dir2/file1", 1) == 1);
    assert(fm.change_dir(".") == 1);
    assert(fm.change_dir("/dir1/dir11") == 1);
    assert(fm.change_dir("/dir1/dir11/dir3") == 0);
    assert(fm.change_dir("/dir1") == 1);
    assert(fm.change_dir("./dir11") == 1);
    assert(fm.change_dir("..") == 1);
    assert(fm.create_file("ddir2/file1", 999) == 0);
    assert(fm.create_file("./dir11/file.txt", 1) == 1);
    assert(fm.remove("dir11", 0) == 0);
    assert(fm.remove("./dir11", 0) == 0);
    assert(fm.remove("./dir11", 1) == 1);
    fm.get_cur_dir(cur_dir);
    assert(strcmp(cur_dir, "/dir1") == 0);
    assert(fm.destroy() == 1);

    // Test 4: File/dir type checks, remove_files, and tree metrics
    setup_file_manager(&fm);
    assert(fm.create(TOTAL_SIZE) == 1);
    assert(fm.create_dir("dir1") == 1);
    assert(fm.create_dir("dir1/dir11") == 1);
    assert(fm.change_dir("dir1") == 1);
    assert(fm.create_dir("../dir2") == 1);
    assert(fm.create_file("file11.txt", 150) == 1);
    assert(fm.create_file("../dir2/a_file22.txt", 250) == 1);
    assert(fm.create_dir("../dir2/empty_sub") == 1);

    assert(fm.exists("../dir2/a_file22.txt") == 1);
    assert(fm.is_file("../dir2/a_file22.txt") == 1);
    assert(fm.is_dir("../dir2/a_file22.txt") == 0);
    assert(fm.exists(".") == 1);
    assert(fm.is_dir(".") == 1);
    assert(fm.is_file(".") == 0);
    assert(fm.exists("../dir2/b_file22.txt") == 0);

    // Verify new size, files_count, dir_count, and remove_files functions
    assert(fm.size("/") == 400);
    assert(fm.free_space() == 600);
    assert(fm.files_count("/") == 2);
    assert(fm.dir_count("/") == 2); // dir1 and dir2 are non-empty; dir11 and empty_sub are empty
    assert(fm.remove_files("/") == 2);
    assert(fm.free_space() == 1000);
    assert(fm.files_count("/") == 0);

    assert(fm.destroy() == 1);
    printf("All virtual file system tests passed successfully!\n");
    return 0;
}