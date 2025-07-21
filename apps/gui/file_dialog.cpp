// This file is part of otmap, an optimal transport solver.
//
// Copyright (C) 2017 Georges Nader
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "file_dialog.h"
#include <iostream>

#ifdef _WIN32
#include <shlobj.h>
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#else
#include "libtinyfiledialogs/tinyfiledialogs.h"
#endif

std::string FileDialog::openFile(const char* title, const char* filter, const char* defaultPath) {
#ifdef _WIN32
    return showDialog(true, title, filter, nullptr, defaultPath);
#else
    const char* filters[] = { "*.png", "*.obj" };
    const char* result = tinyfd_openFileDialog(title, defaultPath, 5, filters, "Supported Files", 0);
    return result ? std::string(result) : "";
#endif
}

std::string FileDialog::saveFile(const char* title, const char* filter,
                                 const char* defaultExt, const char* defaultPath) {
#ifdef _WIN32
    return showDialog(false, title, filter, defaultExt, defaultPath);
#else
    const char* result = tinyfd_saveFileDialog(title, defaultPath, 0, nullptr, nullptr);
    return result ? std::string(result) : "";
#endif
}

const char* FileDialog::getImageFilter() {
    return "Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0PNG Files\0*.png\0JPEG Files\0*.jpg;*.jpeg\0BMP Files\0*.bmp\0All Files\0*.*\0";
}

const char* FileDialog::getObjFilter() {
    return "OBJ Files\0*.obj\0All Files\0*.*\0";
}

#ifdef _WIN32
std::string FileDialog::showDialog(bool isOpen, const char* title, const char* filter, 
                                  const char* defaultExt, const char* defaultPath) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = {0};
    
    // Initialize OPENFILENAME structure
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_EXPLORER;
    
    if (isOpen) {
        ofn.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    } else {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        if (defaultExt) {
            ofn.lpstrDefExt = defaultExt;
        }
    }
    
    // Set initial directory if provided
    if (defaultPath) {
        ofn.lpstrInitialDir = defaultPath;
    }
    
    BOOL result;
    if (isOpen) {
        result = GetOpenFileName(&ofn);
    } else {
        result = GetSaveFileName(&ofn);
    }
    
    if (result) {
        return std::string(szFile);
    }
    
    // Check for errors (optional, for debugging)
    DWORD error = CommDlgExtendedError();
    if (error != 0) {
        std::cerr << "File dialog error: " << error << std::endl;
    }
    
    return "";
}
#endif
