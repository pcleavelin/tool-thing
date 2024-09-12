#ifndef ED_STRING_INCLUDED
#define ED_STRING_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define _String(text)                                                          \
    ((string){.data = (uint8_t *)text, .len = sizeof(text) - 1, .owned = false})
#define _CString_To_String(text)                                               \
    ((string){.data = (uint8_t *)text, .len = strlen(text), .owned = false})
typedef struct {
    uint8_t *data;
    size_t len;

    // FIXME: this is so terribly bad please don't do this
    bool owned;
} string;

bool string_eq(string a, string b);
bool string_eq_cstring(string a, const char *b);
string string_copy(string s);
string string_copy_cstring(const char *str);
char *cstring_copy_string(string str);

#ifdef ED_STRING_IMPLEMENTATION
bool string_eq(string a, string b) {
    if (a.len != b.len || a.data == NULL || b.data == NULL)
        return false;

    for (size_t i = 0; i < a.len; ++i) {
        if (a.data[i] != b.data[i])
            return false;
    }

    return true;
}

bool string_eq_cstring(string a, const char *b) {
    if (b == NULL) {
        if (a.len == 0) {
            return true;
        }

        return false;
    }

    size_t b_len = strlen(b);
    if (a.len != b_len)
        return false;

    for (size_t i = 0; i < a.len; ++i) {
        if (a.data[i] != b[i])
            return false;
    }

    return true;
}

string string_copy(string a) {
    if (a.data == NULL || a.len == 0) {
        return (string){.data = NULL, .len = 0, .owned = false};
    }

    string new_string;

    new_string.data = malloc(a.len * sizeof(uint8_t));
    new_string.len = a.len;
    new_string.owned = true;

    memcpy(new_string.data, a.data, new_string.len * sizeof(uint8_t));

    return new_string;
}

string string_copy_cstring(const char *str) {
    if (str == NULL) {
        return (string){.data = NULL, .len = 0, .owned = false};
    }

    string new_string;

    size_t len = strlen(str);

    new_string.data = malloc(len * sizeof(uint8_t));
    new_string.len = len;
    new_string.owned = true;

    memcpy(new_string.data, str, new_string.len * sizeof(uint8_t));

    return new_string;
}

char *cstring_copy_string(string str) {
    char *new_str = malloc(str.len + 1);

    memcpy(new_str, str.data, str.len);
    new_str[str.len] = 0;

    return new_str;
}

#endif
#endif
