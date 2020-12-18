#include "pch.h"
#include "source.h"

namespace exy {
//------------------------------------------------------------------------------------------------
void SourceFile::dispose() {
    source.dispose();
    tokens.dispose();
}
//------------------------------------------------------------------------------------------------
void SourceFolder::dispose() {
    files.dispose([](auto x) { x->dispose(); });
    folders.dispose([](auto x) { x->dispose(); });
}
//------------------------------------------------------------------------------------------------
void SourceTree::dispose() {
    if (root) {
        root->dispose();
    }
    mem.dispose();
}

bool SourceTree::build() {
    traceln("\r\n%cl#<cyan|blue> { folder: %s#<underline yellow>, threads: 1 }", S("source"),
            &comp.options.path);

    String path{};
    path.append(comp.options.path);
    auto indent = 0;
    root = build(nullptr, path, path.getFileName(), indent);
    path.dispose();

    traceln("%cl#<cyan|blue> { errors: %i#<red>, files: %i#<magenta>, bytes: %u64#<magenta> }", S("source"),
            comp.errors, files, bytes);

    return comp.errors == 0;
}

SourceFolder* SourceTree::build(SourceFolder *parent, String &path, const String &name, int &indent) {
    auto folder = mem.New<SourceFolder>(parent, ids.get(path), ids.get(name));
    if (parent) {
        parent->folders.append(folder);
        makeDotName(path.clear(), folder);
        folder->dotName = ids.get(path);
    } else {
        folder->dotName = folder->name;
    }
    for (auto i = 0; i < indent; ++i) trace("  ");
    traceln("» %s", folder->name);
    ++indent;
    String search{};
    search.append(folder->path).append("\\*");
    WIN32_FIND_DATA wfd{};
    auto handle = FindFirstFile(search.text, &wfd);
    if (handle == INVALID_HANDLE_VALUE) {
        OsError("FindFirstFile", "Could not enumerate the folder %s#<underline yellow> with the search term %s#<yellow>",
                folder->path, &search);
    } else {
        do {
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                String shortName{ wfd.cFileName, cstrlen(wfd.cFileName), 0u };
                if (shortName == "." || shortName == "..") {
                    continue;
                }
                path.clear().append(folder->path).append(S("\\")).append(shortName);
                build(folder, path, shortName, indent);
            } else {
                String shortName{ wfd.cFileName, cstrlen(wfd.cFileName), 0u };
                auto  fileName = shortName.getFileName();
                auto extension = shortName.getFileExtension();
                path.clear().append(folder->path).append(S("\\")).append(shortName);
                ULARGE_INTEGER ul{};
                ul.LowPart = wfd.nFileSizeLow;
                ul.HighPart = wfd.nFileSizeHigh;
                for (auto i = 0; i < indent; ++i) trace("  ");
                if (extension == "exy") {
                    if (ul.QuadPart > comp.options.maxFileSize) {
                        traceln("› %s#<underline red> (%i64#<green> B) !file too big! maxFileSize = %i#<bold>",
                                &fileName, ul.QuadPart, comp.options.maxFileSize);
                    } else if (isaBadFileName(fileName)) {
                        traceln("› %s#<underline red> (%i64#<green> B) !bad file name!",
                                &fileName, ul.QuadPart);
                    } else {
                        auto file = mem.New<SourceFile>(folder, ids.get(path), ids.get(fileName));
                        folder->files.append(file);
                        makeDotName(path.clear(), file);
                        file->dotName = ids.get(path);
                        readFile(file, (int)ul.QuadPart);
                        traceln("› %s#<underline bold> (%i64#<green> B)", file->name, file->source.length);
                        ++files;
                        bytes += ul.QuadPart;
                    }
                } else if (extension.isEmpty()) {
                    traceln("› %s (%i64#<green> B)", &fileName, ul.QuadPart);
                } else {
                    traceln("› %s.%s#<yellow> (%i64#<green> B)", &fileName, &extension, ul.QuadPart);
                }
            }
        } while (FindNextFile(handle, &wfd) != FALSE);
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            OsError("FindNextFile", "Could not finish enumerating the folder %s#<underline yellow> with the search term %s#<yellow>",
                    folder->path, &search);
        }
    }
    FindClose(handle);
    search.dispose();
    --indent;
    return folder;
}

void SourceTree::readFile(SourceFile *file, int size) {
    auto handle = CreateFile(file->path->text, GENERIC_READ,
                             FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        OsError("CreateFile", "Could not open file %s#<underline yellow>", file->path);
    } else {
        DWORD bytesRead = 0;
        file->source.reserve(size + 1);
        if (ReadFile(handle, file->source.text + file->source.length, size, &bytesRead, nullptr) == FALSE) {
            OsError("ReadFile", "Could not read file %s#<underline yellow>", file->path);
        } else {
            file->source.length = size;
        }
        CloseHandle(handle);
    }
}

bool SourceTree::makeDotName(String &tmp, SourceFolder *folder) {
    if (auto parent = folder->parent) {
        if (makeDotName(tmp, parent)) tmp.append(S("."));
        tmp.append(folder->name);
        return true;
    }
    return false;
}

bool SourceTree::makeDotName(String &tmp, SourceFile *file) {
    if (auto parent = file->parent) {
        if (makeDotName(tmp, parent)) tmp.append(S("."));
        tmp.append(file->name);
        return true;
    }
    return false;
}

bool SourceTree::isaBadFileName(const String &v) {
    auto alphas = 0;
    for (auto i = v.length - 1; i >= 0; --i) {
        auto ch = v.text[i];
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            ++alphas;
        } else if ((ch >= '0' && ch <= '9') || ch == '_') {
            // Ok.
        } else {
            return false;
        }
    }
    return alphas == 0;
}
//------------------------------------------------------------------------------------------------
SourceFileProvider::SourceFileProvider() : WorkProvider(comp.options.defaultFilesPerThread) {
    collectFiles(comp.source->root);
}

void SourceFileProvider::dispose() {
    files.dispose();
    pos = 0;
}

bool SourceFileProvider::next(List<SourceFile*> &batch) {
    AcquireSRWLockExclusive(&srw);
    auto end = min(pos + perBatch, files.length);
    for (; pos < end; ++pos) {
        batch.append(files.items[pos]);
    }
    ReleaseSRWLockExclusive(&srw);
    return batch.isNotEmpty();
}

void SourceFileProvider::collectFiles(SourceFolder *folder) {
    for (auto i = 0; i < folder->files.length; ++i) {
        files.append(folder->files.items[i]);
    } for (auto i = 0; i < folder->folders.length; ++i) {
        collectFiles(folder->folders.items[i]);
    }
}
} // namespace exy