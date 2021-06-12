#include "pch.h"
#include "src.h"

#include "tokenizer.h"

namespace exy {
static Identifier makeName(Identifier path, const CHAR *start, const CHAR *end, bool isaFile) {
    auto     alphas = 0;
    auto        pos = start;
    const CHAR* dot = nullptr;
    for (; pos < end; ++pos) {
        auto c = *pos;
        if (c >= '0' && c <= '9') {
            // Do nothing.
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            ++alphas;
        } else if (c == '_') {
            // Do nothing.
        } else if (c == '.') {
            if (dot) {
                break;
            } else if (isaFile) {
                dot = pos;
            } else {
                break;
            }
        } else {
            break; // Bad character.
        }
    }
    if (pos < end) {
        traceln("skipping invalid source name (at least 1 invalid character): %s#<yellow>", path);
    } else if (alphas == 0) {
        traceln("skipping invalid source name (no alpha characters): %s#<yellow>", path);
    } else if (dot) {
        return ids.get(start, dot);
    } else {
        return ids.get(start, end);
    }
    return nullptr;
}

static Identifier getNameFromPath(Identifier path, bool isaFile) {
    for (auto i = path->length; --i >= 0; ) {
        auto c = path->text[i];
        if (c == '\\') {
            return makeName(path, path->text + i + 1, path->end(), isaFile);
        }
    }
    traceln("skipping invalid source name (no slash found): %s#<yellow>", path);
    return nullptr;
}

bool SourceTree::initialize() {
    auto &list = compiler.config.sourceFolders;
    for (auto i = 0; i < list.length; i++) {
        visitSourceFolder(list.items[i]);
    }
    folders.compact();
    tokenize();
    if (compiler.errors == 0) {
        printTree();
    }
    return compiler.errors == 0;
}

void SourceTree::dispose() {
    folders.dispose([](auto x) { x->dispose(); });
    mem.dispose();
}

void SourceTree::visitSourceFolder(Identifier folderPath) {
    if (auto folderName = getNameFromPath(folderPath, /* isaFile = */ false)) {
        auto     folder = mem.New<SourceFolder>(nullptr, folderPath, folderName);
        folders.append(folder);
        folder->initialize();
    }
}

void SourceTree::printTree() {
    traceln("Source tree:");
    for (auto i = 0; i < folders.length; i++) {
        printFolder(folders.items[i], 1);
    }
}

void SourceTree::printFolder(SourceFolder *folder, INT indent) {
    for (auto i = 0; i < indent; i++) trace("  ");
    traceln("%s#<yellow>", folder->name);
    for (auto i = 0; i < folder->files.length; i++) {
        for (auto j = 0; j < indent + 1; j++) trace("  ");
        auto &file = folder->files.items[i];
        traceln("%s#<yellow underline> (%i#<darkyellow> B, %i#<darkyellow> characte%c, %i#<darkyellow> lin%c, %i#<darkyellow> toke%c)", file.name, 
                file.source.length,
                file.characters, file.characters == 1 ? "r" : "rs",
                file.lines, file.lines == 1 ? "e" : "es",
                file.tokens.length, file.tokens.length == 1 ? "n" : "ns");
    }
    for (auto i = 0; i < folder->folders.length; i++) {
        printFolder(folder->folders.items[i], indent + 1);
    }
}

void SourceTree::tokenize() {
    for (auto i = 0; i < folders.length; i++) {
        tokenize(folders.items[i]);
    }
}

void SourceTree::tokenize(SourceFolder *folder) {
    for (auto i = 0; i < folder->folders.length; i++) {
        tokenize(folder->folders.items[i]);
    }
    for (auto i = 0; i < folder->files.length; i++) {
        tokenize(folder->files.items[i]);
    }
}

void SourceTree::tokenize(SourceFile &file) {
    Tokenizer lexer{ file };
    lexer.run();
}
//----------------------------------------------------------
void SourceFolder::initialize() {
    const String extension{ S(EXY_EXTENSION) };
    auto &mem = compiler.source->mem;
    WIN32_FIND_DATA wfd{};
    String tmp{};
    tmp.append(path).append(S("\\*"));
    auto handle = FindFirstFile(tmp.text, &wfd);
    if (handle == INVALID_HANDLE_VALUE) {
        OsError("FindFirstFile", nullptr);
        ++compiler.errors;
    } else {
        do {
            String itemName{ wfd.cFileName };
            if (itemName.startsWith(S("."))) {
                // Do nothing.
            } else if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                String subFolderPath{};
                subFolderPath.append(path).append(S("\\")).append(itemName);
                if (compiler.config.subFolderHasAtLeastOneSourceFile(subFolderPath)) {
                    auto   subFolderPathId = ids.get(subFolderPath);
                    if (auto subFolderName = getNameFromPath(subFolderPathId, /* isaFile = */ false)) {
                        auto     subFolder = mem.New<SourceFolder>(this, subFolderPathId, subFolderName);
                        folders.append(subFolder);
                        subFolder->initialize();
                    }
                }
                subFolderPath.dispose();
            } else if (itemName.endsWith(extension)) {
                String filePath{};
                filePath.append(path).append(S("\\")).append(itemName);
                auto   filePathId = ids.get(filePath);
                if (auto fileName = getNameFromPath(filePathId, /* isaFile = */ true)) {
                    auto    &file = files.place(this, filePathId, fileName);
                    file.initialize();
                }
                filePath.dispose();
            }
        } while (FindNextFile(handle, &wfd) != FALSE);
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            OsError("FindNextFile", nullptr);
            ++compiler.errors;
        }
        FindClose(handle);
    }
    tmp.dispose();
    folders.compact();
    files.compact();
}

void SourceFolder::dispose() {
    folders.dispose([](auto x) { x->dispose(); });
    files.dispose([](auto &x) { x.dispose(); });
}
//----------------------------------------------------------
#define MAX_FILE_SIZE 0x10000
void SourceFile::initialize() {
    auto handle = CreateFile(path->text, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        OsError("FindFirstFile", nullptr);
        ++compiler.errors;
    } else {
        LARGE_INTEGER li{};
        if (GetFileSizeEx(handle, &li) == FALSE) {
            OsError("GetFileSizeEx", nullptr);
            ++compiler.errors;
        } else if (li.QuadPart > MAX_FILE_SIZE) {
            traceln("file too large: %s#<yellow> %i64#<red> B", path, li.QuadPart);
            ++compiler.errors;
        } else if (li.QuadPart == 0) {
            source.reserve(1);
        } else {
            DWORD b{};
            source.reserve(INT(li.QuadPart));
            if (ReadFile(handle, source.text, INT(li.QuadPart), &b, nullptr) == FALSE) {
                OsError("ReadFile", nullptr);
                ++compiler.errors;
            } else {
                source.length = INT(li.QuadPart);
            }
        }
        CloseHandle(handle);
    }
}

void SourceFile::dispose() {
    tokens.dispose();
    source.dispose();
}
} // namespace exy