#include "../source/DownloadFiles.hpp"
#include <cassert>
#include <string>

static FILE* fixture(const std::string& text) {
    FILE* f = tmpfile();
    assert(f);
    assert(fwrite(text.data(), 1, text.size(), f) == text.size());
    rewind(f);
    return f;
}

int main(int argc, char** argv) {
    char version[16];
    const std::string prefix = "# FPSLocker Warehouse\n" + std::string(300000, 'x') +
        "\n| game | `0000000000000001` | ";
    FILE* f = fixture(prefix + "`0000000000000002` (\xE2\x97\xAF, v0, 1.0.0) |\n");
    assert(download_files::warehouseStatus(f, 1, 2, version, sizeof(version)) == 0x1001);
    assert(!strcmp(version, "1.0.0"));
    assert(download_files::warehouseStatus(f, 1, 3, version, sizeof(version)) == 0x1003);
    assert(download_files::warehouseStatus(f, 4, 3, version, sizeof(version)) == 0x1002);
    fclose(f);
    std::string row;
    for (int i = 0; i < 300; ++i) row += "`0000000000000002` (\xE2\x9D\x8C, v0, 1.0.0)<br/>";
    row += "`0000000000000003` ([patch](https://example.org/(nested)), v1, 12345678901234567890) |";
    f = fixture(prefix + row);
    assert(download_files::warehouseStatus(f, 1, 2, version, sizeof(version)) == 0x1006);
    assert(download_files::warehouseStatus(f, 1, 3, version, sizeof(version)) == 0x404);
    assert(download_files::warehouseStatus(f, 1, 4, version, sizeof(version)) == 0x1005);
    assert(strlen(version) == 15);
    fclose(f);
    f = fixture(prefix + "`0000000000000002` (" + std::string(600, 'x'));
    assert(download_files::warehouseStatus(f, 1, 2, version, sizeof(version)) == 0x1007);
    fclose(f);
    f = fixture(prefix + "`0000000000000002` (broken\n");
    assert(download_files::warehouseStatus(f, 1, 2, version, sizeof(version)) == 0x1007);
    fclose(f);
    FILE* a = fixture(std::string(32768, 'a'));
    FILE* b = fixture(std::string(32768, 'a'));
    assert(download_files::equal(a, b));
    fseek(b, 32767, SEEK_SET); fputc('b', b);
    assert(!download_files::equal(a, b));
    assert(!download_files::equal(a, nullptr));
    fclose(a); fclose(b);
    if (argc > 1) {
        f = fopen(argv[1], "rb");
        assert(f);
        assert(download_files::warehouseStatus(f, 0x0100BA9014A02000ULL,
            0x4C0ED5711263A6D9ULL, version, sizeof(version)) == 0x1006);
        fclose(f);
    }
}
