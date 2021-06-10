#include "pch.h"

namespace exy {
bool Configuration::initialize() {
    traceln("Finding top-level source folders...");
    if (setCompilerFolder()) {
        if (setAppFolders()) {

        }
    }
    return compiler.errors == 0;
}

void Configuration::dispose() {
    sourceFolders.dispose();
}

bool Configuration::setCompilerFolder() {
    auto res = GetModuleFileName(nullptr, tmpbuf, tmpbufcap);
    if (res == 0) {
        OsError("GetModuleFileName", nullptr);
        ++compiler.errors;
        return false;
    }
    auto len = cstrlen(tmpbuf);
    for (; --len >= 0; ) {
        if (tmpbuf[len] == '\\') { // Because it is windows.
            compilerFolderName = ids.get(tmpbuf, len); // Excludes the trailing '\'.
            return true;
        }
    }
    traceln("cannot get the name of the folder containing the compiler");
    ++compiler.errors;
    return false;
}

bool Configuration::setAppFolders() {
    WIN32_FIND_DATA wfd{};
    String path{};
    path.append(compilerFolderName).append(S("\\*"));
    auto handle = FindFirstFile(path.text, &wfd);
    if (handle == INVALID_HANDLE_VALUE) {
        OsError("FindFirstFile", nullptr);
        ++compiler.errors;
    } else {
        do {
            String itemName{ wfd.cFileName };
            if (itemName.startsWith(S("."))) {
                // Do nothing.
            } else if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                // This is a file. Skip it.
            } else {
                String subFolderName{};
                subFolderName.append(compilerFolderName).append(S("\\")).append(itemName);
                if (subFolderHasAtLeastOneSourceFile(subFolderName)) {
                    sourceFolders.append(ids.get(subFolderName));
                }
                subFolderName.dispose();
            }
        } while (FindNextFile(handle, &wfd) != FALSE);
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            OsError("FindNextFile", nullptr);
            ++compiler.errors;
        }
        FindClose(handle);
    }
    path.dispose();
    if (sourceFolders.isEmpty()) {
        traceln("...no source found in %s#<yellow>", compilerFolderName);
        ++compiler.errors;
    } else {
        traceln("...found %i#<green> top-level source folde%c:", sourceFolders.length, sourceFolders.length == 1 ? "r" : "rs");
        for (auto i = 0; i < sourceFolders.length; i++) {
            traceln("    %i#<green>. %s#<yellow>", i + 1, sourceFolders.items[i]);
        }
    }
    return compiler.errors == 0;
}

bool Configuration::subFolderHasAtLeastOneSourceFile(const String &subFolderName) {
    const String extension{ S(EXY_EXTENSION)};
    auto found = false;
    WIN32_FIND_DATA wfd{};
    String path{};
    path.append(subFolderName).append(S("\\*"));
    auto handle = FindFirstFile(path.text, &wfd);
    if (handle == INVALID_HANDLE_VALUE) {
        OsError("FindFirstFile", nullptr);
        ++compiler.errors;
    } else {
        do {
            String fileName{ wfd.cFileName };
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                if (fileName.endsWith(extension)) {
                    found = true;
                    break;
                }
            }
        } while (FindNextFile(handle, &wfd) != FALSE);
        if (!found && (GetLastError() != ERROR_NO_MORE_FILES)) {
            OsError("FindNextFile", nullptr);
            ++compiler.errors;
        }
        FindClose(handle);
    }
    path.dispose();
    return found && compiler.errors == 0;
}
} // namespace exy