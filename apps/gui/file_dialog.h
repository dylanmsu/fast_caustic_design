// This file is part of otmap, an optimal transport solver.
//
// Copyright (C) 2017 Georges Nader
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

/**
 * @brief Native file dialog wrapper for Windows
 * 
 * Provides a simple interface to Windows native file dialogs
 * for opening and saving files with proper filtering.
 */
class FileDialog {
public:
    /**
     * @brief Open a file selection dialog
     * @param title Dialog window title
     * @param filter File type filter string (Windows format)
     * @param defaultPath Initial directory path
     * @return Selected file path, or empty string if cancelled
     */
    static std::string openFile(
        const char* title = "Open File",
        const char* filter = "All Files\0*.*\0",
        const char* defaultPath = nullptr
    );

    /**
     * @brief Open a save file dialog
     * @param title Dialog window title
     * @param filter File type filter string (Windows format)
     * @param defaultExt Default file extension
     * @param defaultPath Initial directory path
     * @return Selected file path, or empty string if cancelled
     */
    static std::string saveFile(
        const char* title = "Save File",
        const char* filter = "All Files\0*.*\0",
        const char* defaultExt = nullptr,
        const char* defaultPath = nullptr
    );

    /**
     * @brief Get image file filter string for common image formats
     * @return Filter string for PNG, JPG, BMP files
     */
    static const char* getImageFilter();

    /**
     * @brief Get OBJ file filter string
     * @return Filter string for OBJ files
     */
    static const char* getObjFilter();

private:
#ifdef _WIN32
    static std::string showDialog(bool isOpen, const char* title, const char* filter, 
                                const char* defaultExt, const char* defaultPath);
#endif
};
