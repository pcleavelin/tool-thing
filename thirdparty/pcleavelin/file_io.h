#ifndef ED_FILE_IO_INCLUDED
#define ED_FILE_IO_INCLUDED
#include <stdint.h>
#include <stdbool.h>

#include "string.h"

uint64_t get_file_size(string file_path);
bool load_file(string file_path, size_t size, void *buffer);

#ifdef ED_FILE_IO_IMPLEMENTATION

#if defined(__unix__)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

unsigned long get_file_size(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    assert(file && "get_file_size: failed to open file");

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fclose(file);

    return fsize;
}

bool load_file(const char *filePath, unsigned long size, void *buffer) {
    FILE *file = fopen(filePath, "r");
    assert(file && "load_file: failed to open file");

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    fread(buffer, fsize, 1, file);

    fclose(file);

    return true;
}

#elif defined(_WIN32) || defined(WIN32)
#include <windows.h>
#define assertm(exp, msg) assert(((void)msg, exp))

VOID CALLBACK FileIOCompletionRoutine(
        __in  DWORD dwErrorCode,
        __in  DWORD dwNumberOfBytesTransfered,
        __in  LPOVERLAPPED lpOverlapped )
{
    printf("Error code: %li", dwErrorCode);
    printf("Number of bytes: %li", dwNumberOfBytesTransfered);
}

uint64_t get_file_size(string file_path) {
    char *file_path_with_sentinel = malloc(file_path.len + 1);
    memcpy(file_path_with_sentinel, file_path.data, file_path.len + 1);
    file_path_with_sentinel[file_path.len] = 0;

    // FIXME: convert to null teminated string
    HANDLE hFile = CreateFile(file_path_with_sentinel,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);

    LARGE_INTEGER size;
    assert(GetFileSizeEx(hFile, &size) && "get_file_size: failed to get file size");

    free(file_path_with_sentinel);
    return size.LowPart;
}

bool load_file(string file_path, size_t size, void *buffer) {
    char *file_path_with_sentinel = malloc(file_path.len + 1);
    memcpy(file_path_with_sentinel, file_path.data, file_path.len + 1);
    file_path_with_sentinel[file_path.len] = 0;

    // FIXME: convert to null teminated string
    HANDLE hFile = CreateFile(file_path_with_sentinel,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    OVERLAPPED ol = {0};
    unsigned long bytesRead = 0;
    if (!ReadFile(hFile, buffer, size, &bytesRead, NULL)) {
        unsigned long error = GetLastError();
        unsigned long msg = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, buffer, size, NULL);

        printf("failed to load file: %s\n", (char *)buffer);
        assert(false && "failed to read file");
    }
    assert(bytesRead == size && "load_file: didn't one-shot read the whole file");

    free(file_path_with_sentinel);
    return true;
}
#elif defined(__APPLE__)
#include <Foundation/Foundation.h>

uint64_t get_file_size(string file_path) {
    NSFileManager* file_manager = [NSFileManager defaultManager];
    NSString *path = [file_manager stringWithFileSystemRepresentation:(const char *)file_path.data length:file_path.len]; 

    NSError *error = NULL;
    NSDictionary<NSFileAttributeKey, id> *attributes = [file_manager attributesOfItemAtPath:path error:&error];

    if (error) {
        NSLog(@"error getting file attributes: %@\n", error.description);
        return 0;
    }

    NSNumber *file_size = [attributes objectForKey:NSFileSize];
    
    return file_size.unsignedLongLongValue;
}

bool load_file(string file_path, size_t size, void *buffer) {
    NSFileManager* file_manager = [NSFileManager defaultManager];
    NSString *path = [file_manager stringWithFileSystemRepresentation:(const char *)file_path.data length:file_path.len]; 

    NSData *data = [file_manager contentsAtPath:path];
    if (data == NULL) {
        fprintf(stderr, "failed to load file: %.*s\n", (int)file_path.len, file_path.data);
        return false;
    }

    if (data.length > size) {
        fprintf(stderr, "buffer not large enough for file: %.*s\n", (int)file_path.len, file_path.data);
    }

    memcpy(buffer, data.bytes, data.length);

    return true;
}

#endif

#endif
#endif
