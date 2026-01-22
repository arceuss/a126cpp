// defuturify.c - set timestamps of all files under a directory to "now" (Windows)
// Build: gcc -O2 -Wall -Wextra -o defuturify.exe defuturify.c
// Usage: defuturify.exe [path]   (default: .\src)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

static int touch_path(const char *path, const FILETIME *now_ft) {
    HANDLE h = CreateFileA(
        path,
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, // allows opening directories too
        NULL
    );

    if (h == INVALID_HANDLE_VALUE) {
        // Some files may be locked; report and continue.
        fprintf(stderr, "WARN: CreateFile failed (%lu): %s\n", GetLastError(), path);
        return 0;
    }

    // Set creation, access, and write times to now.
    if (!SetFileTime(h, now_ft, now_ft, now_ft)) {
        fprintf(stderr, "WARN: SetFileTime failed (%lu): %s\n", GetLastError(), path);
        CloseHandle(h);
        return 0;
    }

    CloseHandle(h);
    return 1;
}

static int walk_and_touch(const char *dir, const FILETIME *now_ft) {
    char pattern[MAX_PATH];
    WIN32_FIND_DATAA ffd;

    // Touch the directory itself (optional, but helps some build systems)
    touch_path(dir, now_ft);

    // pattern = dir\*
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);

    HANDLE find = FindFirstFileA(pattern, &ffd);
    if (find == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "ERROR: FindFirstFile failed (%lu): %s\n", GetLastError(), pattern);
        return 0;
    }

    int ok = 1;

    do {
        const char *name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char child[MAX_PATH];
        snprintf(child, sizeof(child), "%s\\%s", dir, name);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            // Skip symlinks/junctions to avoid looping into unexpected places.
            // If you want them touched too, remove this block.
            continue;
        }

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!walk_and_touch(child, now_ft)) ok = 0;
        } else {
            touch_path(child, now_ft);
        }
    } while (FindNextFileA(find, &ffd));

    DWORD err = GetLastError();
    FindClose(find);

    if (err != ERROR_NO_MORE_FILES) {
        fprintf(stderr, "WARN: FindNextFile ended with error (%lu) in %s\n", err, dir);
        ok = 0;
    }

    return ok;
}

int main(int argc, char **argv) {
    const char *root = (argc >= 2) ? argv[1] : "src";

    // Get "now" in FILETIME
    FILETIME now_ft;
    GetSystemTimeAsFileTime(&now_ft);

    printf("Defuturify: touching everything under: %s\n", root);

    if (!walk_and_touch(root, &now_ft)) {
        fprintf(stderr, "Completed with warnings/errors.\n");
        return 1;
    }

    printf("Done.\n");
    return 0;
}
