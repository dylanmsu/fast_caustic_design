// This file is part of otmap, an optimal transport solver.
//
// Copyright (C) 2017 Georges Nader
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

#include <GLFW/glfw3.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Forward declarations
struct CLIopts;

/**
 * @brief Dear ImGui-based GUI interface for caustic design application
 * 
 * Provides a complete graphical interface that mirrors all command-line
 * functionality while maintaining the existing CLI interface unchanged.
 */
class CausticGUI {
public:
    CausticGUI();
    ~CausticGUI();

    /**
     * @brief Initialize the GUI system (OpenGL context, ImGui setup)
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Run the main GUI loop
     * @return Exit code (0 for success)
     */
    int run();

    /**
     * @brief Cleanup GUI resources
     */
    void cleanup();

    bool should_quit;

private:
    // GUI state variables
    std::string target_image_path;
    std::string source_image_path;
    std::string output_path;
    int resolution;
    float focal_length;
    float thickness;
    float mesh_width;
    double refractive_index;
    int beta_method;
    int max_iterations;
    double threshold;
    double max_ratio;
    int verbose_level;

    // Processing state
    std::atomic<bool> is_processing;
    std::atomic<bool> processing_complete;
    std::atomic<bool> processing_error;
    std::string processing_status;
    std::string error_message;
    std::mutex status_mutex;
    std::thread processing_thread;

    // GUI window state
    bool show_advanced_options;
    bool show_command_preview;
    bool auto_generate_filename;
    std::string command_line_preview;

    GLFWwindow* window = nullptr;

    // Private methods
    void renderMainWindow();
    void renderFileSelection();
    void renderParameters();
    void renderAdvancedOptions();
    void renderCommandPreview();
    void renderProcessingStatus();
    void renderGenerateButton();

    void updateCommandLinePreview();
    bool validateInputs();
    void resetToDefaults();
    std::string generateParameterFilename(const std::string& baseFilename);
    
    CLIopts createCLIOptsFromGUI();
    void startProcessing();
    void processInBackground(CLIopts opts);
    
    // Platform-specific initialization
    bool initializeOpenGL();
    bool initializeImGui();
    void handleWindowEvents();
    
    // Window handle (Windows-specific)
#ifdef _WIN32
    void* hwnd;
    void* hdc;
    void* hglrc;
#endif
};
