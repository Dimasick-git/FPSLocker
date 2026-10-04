#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>

namespace download_files {
// Warehouse grows independently of the overlay's small heap. Read entries from
// disk with fixed buffers, including rows longer than a read buffer.
inline unsigned warehouseStatus(FILE* file, uint64_t title, uint64_t build,
                                char* version, size_t versionSize) {
    if (!file || !versionSize) return 0x1007;
    version[0] = 0;
    rewind(file);
    char header[22] = {};
    if (!fgets(header, sizeof(header), file) ||
        strncmp(header, "# FPSLocker Warehouse", 21)) return 0x1007;
    char wanted[19];
    snprintf(wanted, sizeof(wanted), "`%016llX`", (unsigned long long)title);
    char window[19] = {};
    size_t count = 0;
    int c;
    bool found = false;
    while ((c = fgetc(file)) != EOF) {
        if (count < 18) window[count++] = (char)c;
        else { memmove(window, window + 1, 17); window[17] = (char)c; }
        if (count == 18 && !memcmp(window, wanted, 18)) { found = true; break; }
    }
    if (!found) return ferror(file) ? 0x101 : 0x1002;
    unsigned matching = 0x404, latest = 0x1007;
    bool buildFound = false;
    while ((c = fgetc(file)) != EOF && c != '\n') {
        if (c != '`') continue;
        char id[17] = {};
        for (size_t i = 0; i < 16; ++i) {
            c = fgetc(file);
            if (c == EOF || c == '\n') return 0x1007;
            id[i] = (char)c;
        }
        if (fgetc(file) != '`') return 0x1007;
        do { c = fgetc(file); } while (c == ' ');
        if (c != '(') return 0x1007;
        char metadata[512] = {};
        size_t length = 0;
        unsigned depth = 1;
        while ((c = fgetc(file)) != EOF && c != '\n') {
            if (c == '(') ++depth;
            if (c == ')' && !--depth) break;
            if (length + 1 >= sizeof(metadata)) return 0x1007;
            metadata[length++] = (char)c;
        }
        if (c != ')') return 0x1007;
        const bool unnecessary = !strncmp(metadata, "\xE2\x97\xAF", 3);
        const bool unavailable = !strncmp(metadata, "\xE2\x9D\x8C", 3);
        latest = unnecessary ? 0x1003 : unavailable ? 0x1004 :
                 metadata[0] == '[' ? 0x1005 : 0x1007;
        char buildText[17];
        snprintf(buildText, sizeof(buildText), "%016llX", (unsigned long long)build);
        if (!strcmp(id, buildText)) {
            buildFound = true;
            matching = unnecessary ? 0x1001 : unavailable ? 0x1006 : 0x404;
        }
        version[0] = 0;
        const char* comma = strrchr(metadata, ',');
        if (comma && comma[1] == ' ' && comma[2] && comma[2] != 'v') {
            snprintf(version, versionSize, "%s", comma + 2);
        }
    }
    if (ferror(file)) return 0x101;
    return buildFound ? matching : latest;
}

// Never allocate a copy of either file just to compare them.
inline bool equal(FILE* first, FILE* second) {
    if (!first || !second) return false;
    rewind(first); rewind(second);
    unsigned char a[1024], b[1024];
    for (;;) {
        size_t na = fread(a, 1, sizeof(a), first);
        size_t nb = fread(b, 1, sizeof(b), second);
        if (na != nb || memcmp(a, b, na)) return false;
        if (na < sizeof(a)) return !ferror(first) && !ferror(second);
    }
}
}
