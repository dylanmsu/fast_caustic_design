// This file is part of otmap, an optimal transport solver.
//
// Copyright (C) 2017 Georges Nader
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#define NOMINMAX
#include "caustic_gui.h"
#include "file_dialog.h"
#include "../common/cli_options.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <limits>
#include <cstring>
#include <iomanip>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif

// Forward declaration of processing function (to be implemented)
extern int processCausticGeneration(CLIopts opts);

CausticGUI::CausticGUI() 
    : target_image_path("")
    , source_image_path("")
    , output_path("./output.obj")
    , resolution(100)
    , focal_length(1.0f)
    , thickness(0.2f)
    , mesh_width(1.0f)
    , refractive_index(1.55)
    , beta_method(1)  // 0=zero, 1=cj
    , max_iterations(1000)
    , threshold(1e-7)
    , max_ratio((std::numeric_limits<double>::max)())
    , verbose_level(1)
    , is_processing(false)
    , processing_complete(false)
    , processing_error(false)
    , processing_status("Ready")
    , show_advanced_options(false)
    , show_command_preview(true)
    , auto_generate_filename(false)
#ifdef _WIN32
    , hwnd(nullptr)
    , hdc(nullptr)
    , hglrc(nullptr)
#endif
{
    updateCommandLinePreview();
}

CausticGUI::~CausticGUI() {
    cleanup();
}

bool CausticGUI::initialize() {
    if (!initializeOpenGL()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return false;
    }

    if (!initializeImGui()) {
        std::cerr << "Failed to initialize ImGui" << std::endl;
        return false;
    }

    return true;
}

int CausticGUI::run() {
    if (!initialize()) {
        return -1;
    }

#ifdef _WIN32
    MSG msg;
    bool done = false;
    
    while (!done) {
        // Poll and handle messages
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render our GUI
        renderMainWindow();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, 1200, 800);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SwapBuffers((HDC)hdc);
    }
#endif

    cleanup();
    return 0;
}

void CausticGUI::renderMainWindow() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_MenuBar;

    if (ImGui::Begin("Caustic Design Generator", nullptr, window_flags)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reset to Defaults")) {
                    resetToDefaults();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    PostQuitMessage(0);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Advanced Options", nullptr, &show_advanced_options);
                ImGui::MenuItem("Command Preview", nullptr, &show_command_preview);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Main content
        renderFileSelection();
        ImGui::Separator();
        renderParameters();

        if (show_advanced_options) {
            ImGui::Separator();
            renderAdvancedOptions();
        }

        if (show_command_preview) {
            ImGui::Separator();
            renderCommandPreview();
        }

        ImGui::Separator();
        renderProcessingStatus();
        renderGenerateButton();
    }
    ImGui::End();
}

void CausticGUI::renderFileSelection() {
    ImGui::Text("File Selection");
    ImGui::Spacing();

    // Target image selection (required)
    ImGui::Text("Target Image (Required):");
    ImGui::SameLine();
    if (ImGui::Button("Browse##target")) {
        std::string filename = FileDialog::openFile("Select Target Image",
                                                   FileDialog::getImageFilter());
        if (!filename.empty()) {
            target_image_path = filename;
            updateCommandLinePreview();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear##target")) {
        target_image_path.clear();
        updateCommandLinePreview();
    }

    // Display selected file or placeholder
    if (target_image_path.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No target image selected");
    } else {
        std::filesystem::path path(target_image_path);
        ImGui::Text("Selected: %s", path.filename().string().c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", target_image_path.c_str());
        }
    }

    ImGui::Spacing();

    // Source image selection (optional)
    ImGui::Text("Source Image (Optional):");
    ImGui::SameLine();
    if (ImGui::Button("Browse##source")) {
        std::string filename = FileDialog::openFile("Select Source Image",
                                                   FileDialog::getImageFilter());
        if (!filename.empty()) {
            source_image_path = filename;
            updateCommandLinePreview();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear##source")) {
        source_image_path.clear();
        updateCommandLinePreview();
    }

    // Display selected file or placeholder
    if (source_image_path.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Using uniform light distribution");
    } else {
        std::filesystem::path path(source_image_path);
        ImGui::Text("Selected: %s", path.filename().string().c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", source_image_path.c_str());
        }
    }

    ImGui::Spacing();

    // Output path selection
    ImGui::Text("Output File:");
    ImGui::SameLine();
    if (ImGui::Button("Browse##output")) {
        std::string filename = FileDialog::saveFile("Save Output File",
                                                   FileDialog::getObjFilter(), "obj");
        if (!filename.empty()) {
            output_path = filename;
            updateCommandLinePreview();
        }
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(300);

    // Use char buffer for ImGui compatibility
    static char output_buffer[512];
    strncpy_s(output_buffer, output_path.c_str(), sizeof(output_buffer) - 1);
    output_buffer[sizeof(output_buffer) - 1] = '\0';

    if (ImGui::InputText("##output_path", output_buffer, sizeof(output_buffer))) {
        output_path = std::string(output_buffer);
        updateCommandLinePreview();
    }

    // Auto-generate filename checkbox
    ImGui::Spacing();
    if (ImGui::Checkbox("Auto-generate filename from parameters", &auto_generate_filename)) {
        updateCommandLinePreview();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, adds parameter values to the filename\nExample: res-100--focal_l-1.0--thickness-0.2_output.obj\nHelps track different parameter combinations");
    }

    // Show preview of generated filename if enabled
    if (auto_generate_filename) {
        std::string previewFilename = generateParameterFilename(output_path);
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Generated filename:");
        ImGui::SameLine();

        // Show just the filename part if it's too long
        std::filesystem::path previewPath(previewFilename);
        std::string displayName = previewPath.filename().string();
        if (displayName.length() > 60) {
            displayName = displayName.substr(0, 57) + "...";
        }

        ImGui::TextWrapped("%s", displayName.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Full path: %s", previewFilename.c_str());
        }
    }
}

void CausticGUI::renderParameters() {
    ImGui::Text("Lens Parameters");
    ImGui::Spacing();

    // Resolution slider
    if (ImGui::SliderInt("Resolution", &resolution, 10, 500)) {
        updateCommandLinePreview();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Mesh resolution for the caustic surface\nHigher values = more detail but slower computation");
    }

    // Focal length
    if (ImGui::SliderFloat("Focal Length", &focal_length, 0.1f, 10.0f, "%.2f")) {
        updateCommandLinePreview();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Distance from lens to projection plane");
    }

    // Thickness
    if (ImGui::SliderFloat("Thickness", &thickness, 0.01f, 1.0f, "%.3f")) {
        updateCommandLinePreview();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Physical thickness of the lens");
    }

    // Mesh width
    if (ImGui::SliderFloat("Mesh Width", &mesh_width, 0.1f, 5.0f, "%.2f")) {
        updateCommandLinePreview();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Physical width and height of the lens");
    }

    // Refractive index with inline input and preset button
    ImGui::Text("Refractive Index");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputDouble("##refractive_index", &refractive_index, 0.0, 0.0, "%.3f")) {
        if (refractive_index < 1.0) refractive_index = 1.0;
        if (refractive_index > 3.0) refractive_index = 3.0;
        updateCommandLinePreview();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Refractive index of the lens material\nControls how light bends through the lens\nHigher values = stronger light bending\nTypical range: 1.0 to 3.0");
    }

    // Quick refractive index presets with dynamic button text
    ImGui::SameLine();

    // Determine current material name for button text
    std::string buttonText = "Material Presets";
    if (std::abs(refractive_index - 1.45) < 0.001) buttonText = "PLA (1.45)";
    else if (std::abs(refractive_index - 1.49) < 0.001) buttonText = "Acrylic (1.49)";
    else if (std::abs(refractive_index - 1.50) < 0.001) buttonText = "Glass (1.50)";
    else if (std::abs(refractive_index - 1.51) < 0.001) buttonText = "Formlabs V4 (1.51)";
    else if (std::abs(refractive_index - 1.52) < 0.001) buttonText = "Siraya Tech (1.52)";
    else if (std::abs(refractive_index - 1.55) < 0.001) buttonText = "Default (1.55)";
    else {
        std::stringstream ss;
        ss << "Custom (" << std::fixed << std::setprecision(3) << refractive_index << ")";
        buttonText = ss.str();
    }

    if (ImGui::Button(buttonText.c_str())) {
        ImGui::OpenPopup("refractive_presets");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to select from common material presets");
    }

    if (ImGui::BeginPopup("refractive_presets")) {
        ImGui::Text("Common Materials:");
        ImGui::Separator();
        if (ImGui::MenuItem("PLA 3D Print (1.45)")) { refractive_index = 1.45; updateCommandLinePreview(); }
        if (ImGui::MenuItem("Acrylic (1.49)")) { refractive_index = 1.49; updateCommandLinePreview(); }
        if (ImGui::MenuItem("Glass (1.50)")) { refractive_index = 1.50; updateCommandLinePreview(); }
        if (ImGui::MenuItem("Formlabs Clear V4 (1.51)")) { refractive_index = 1.51; updateCommandLinePreview(); }
        if (ImGui::MenuItem("Siraya Tech Ultra Clear (1.52)")) { refractive_index = 1.52; updateCommandLinePreview(); }
        if (ImGui::MenuItem("Anycubic High Clear (1.50)")) { refractive_index = 1.50; updateCommandLinePreview(); }
        ImGui::Separator();
        if (ImGui::MenuItem("Default (1.55)")) { refractive_index = 1.55; updateCommandLinePreview(); }
        ImGui::EndPopup();
    }
}

void CausticGUI::renderAdvancedOptions() {
    if (ImGui::CollapsingHeader("Advanced Solver Options", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        // Beta method
        const char* beta_items[] = { "Zero", "Conjugate Jacobian" };
        if (ImGui::Combo("Beta Method", &beta_method, beta_items, 2)) {
            updateCommandLinePreview();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Optimization method\nConjugate Jacobian is recommended for better performance");
        }

        ImGui::Separator();
        ImGui::Text("Convergence Parameters:");

        // Max iterations with slider and input
        if (ImGui::SliderInt("Max Iterations", &max_iterations, 10, 5000)) {
            updateCommandLinePreview();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Maximum number of solver iterations\nHigher values allow more time to converge but take longer\nTypical range: 100-2000");
        }

        // Advanced input for max iterations
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputInt("##max_iter_input", &max_iterations)) {
            if (max_iterations < 1) max_iterations = 1;
            if (max_iterations > 50000) max_iterations = 50000;
            updateCommandLinePreview();
        }

        // Convergence threshold with inline input and preset button
        ImGui::Text("Convergence Threshold");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        if (ImGui::InputDouble("##threshold", &threshold, 0.0, 0.0, "%.2e")) {
            if (threshold <= 0) threshold = 1e-12;
            if (threshold > 1.0) threshold = 1.0;
            updateCommandLinePreview();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Stopping criterion for optimization\nLower values = more precise but slower convergence\nTypical range: 1e-12 to 1e-3\nDefault: 1e-7");
        }

        // Quick threshold presets with dynamic button text
        ImGui::SameLine();

        // Determine current threshold preset for button text
        std::string thresholdButtonText = "Precision Presets";
        if (std::abs(threshold - 1e-4) < 1e-5) thresholdButtonText = "Fast (1e-4)";
        else if (std::abs(threshold - 1e-7) < 1e-8) thresholdButtonText = "Normal (1e-7)";
        else if (std::abs(threshold - 1e-10) < 1e-11) thresholdButtonText = "Precise (1e-10)";
        else if (std::abs(threshold - 1e-12) < 1e-13) thresholdButtonText = "Very Precise (1e-12)";
        else {
            std::stringstream ss;
            ss << "Custom (" << std::scientific << std::setprecision(0) << threshold << ")";
            thresholdButtonText = ss.str();
        }

        if (ImGui::Button(thresholdButtonText.c_str())) {
            ImGui::OpenPopup("threshold_presets");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to select from common precision presets");
        }

        if (ImGui::BeginPopup("threshold_presets")) {
            ImGui::Text("Convergence Precision:");
            ImGui::Separator();
            if (ImGui::MenuItem("Fast (1e-4)")) { threshold = 1e-4; updateCommandLinePreview(); }
            if (ImGui::MenuItem("Normal (1e-7)")) { threshold = 1e-7; updateCommandLinePreview(); }
            if (ImGui::MenuItem("Precise (1e-10)")) { threshold = 1e-10; updateCommandLinePreview(); }
            if (ImGui::MenuItem("Very Precise (1e-12)")) { threshold = 1e-12; updateCommandLinePreview(); }
            ImGui::EndPopup();
        }

        // Max ratio parameter
        ImGui::Separator();
        ImGui::Text("Density Control:");

        // Checkbox for unlimited ratio
        bool unlimited_ratio = (max_ratio >= 1e15);
        if (ImGui::Checkbox("Unlimited Density Ratio", &unlimited_ratio)) {
            if (unlimited_ratio) {
                max_ratio = (std::numeric_limits<double>::max)();
            } else {
                max_ratio = 1000.0;
            }
            updateCommandLinePreview();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Controls maximum ratio between brightest and darkest regions\nUnlimited allows extreme contrasts but may cause numerical issues");
        }

        // Ratio slider (only if not unlimited)
        if (!unlimited_ratio) {
            // Use SliderFloat for compatibility, convert to double
            float ratio_float = static_cast<float>(max_ratio);
            if (ImGui::SliderFloat("Max Density Ratio", &ratio_float, 1.0f, 10000.0f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
                max_ratio = static_cast<double>(ratio_float);
                updateCommandLinePreview();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Maximum ratio between brightest and darkest regions\nHigher values preserve more contrast but may be harder to solve\nTypical range: 10-1000");
            }
        }

        ImGui::Separator();
        ImGui::Text("Output Control:");

        // Verbose level
        if (ImGui::SliderInt("Verbose Level", &verbose_level, 0, 10)) {
            updateCommandLinePreview();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Amount of diagnostic output during processing\n0=silent, 1=normal, 5=detailed, 10=very detailed");
        }
    }
}

void CausticGUI::renderCommandPreview() {
    if (ImGui::CollapsingHeader("Command Line Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        ImGui::Text("Equivalent command line:");
        ImGui::Separator();

        // Make the text selectable
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

        // Use char buffer for ImGui compatibility
        static char cmd_buffer[2048];
        strncpy_s(cmd_buffer, command_line_preview.c_str(), sizeof(cmd_buffer) - 1);
        cmd_buffer[sizeof(cmd_buffer) - 1] = '\0';

        ImGui::InputTextMultiline("##command_preview", cmd_buffer, sizeof(cmd_buffer),
                                 ImVec2(-1, 60), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        if (ImGui::Button("Copy to Clipboard")) {
            ImGui::SetClipboardText(command_line_preview.c_str());
        }
    }
}

void CausticGUI::renderProcessingStatus() {
    if (is_processing) {
        ImGui::Text("Status: Processing...");

        // Simple progress indicator
        static float progress = 0.0f;
        progress += 0.01f;
        if (progress > 1.0f) progress = 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1, 0));

        std::lock_guard<std::mutex> lock(status_mutex);
        if (!processing_status.empty()) {
            ImGui::Text("%s", processing_status.c_str());
        }
    } else if (processing_complete) {
        if (processing_error) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", error_message.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Processing completed successfully!");
        }
    } else {
        ImGui::Text("Status: Ready");
    }
}

void CausticGUI::renderGenerateButton() {
    ImGui::Spacing();

    bool can_generate = !target_image_path.empty() && !is_processing;

    if (!can_generate) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Generate Caustic", ImVec2(-1, 50))) {
        startProcessing();
    }

    if (!can_generate) {
        ImGui::EndDisabled();

        if (target_image_path.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                             "Please select a target image first");
        } else if (is_processing) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f),
                             "Processing in progress...");
        }
    }
}

void CausticGUI::updateCommandLinePreview() {
    std::stringstream cmd;
    cmd << "caustic_design";

    if (!target_image_path.empty()) {
        cmd << " -in_trg \"" << target_image_path << "\"";
    }

    if (!source_image_path.empty()) {
        cmd << " -in_src \"" << source_image_path << "\"";
    }

    cmd << " -output \"" << output_path << "\"";
    cmd << " -res " << resolution;
    cmd << " -focal_l " << focal_length;
    cmd << " -thickness " << thickness;
    cmd << " -mesh_width " << mesh_width;

    // Only include refractive index if different from default
    if (refractive_index != 1.55) {
        cmd << " -ri " << std::fixed << std::setprecision(3) << refractive_index;
    }

    cmd << " -beta " << (beta_method == 0 ? "0" : "cj");

    // Advanced solver options (only include if different from defaults)
    if (max_iterations != 1000) {
        cmd << " -itr " << max_iterations;
    }
    if (threshold != 1e-7) {
        cmd << " -th " << std::scientific << threshold;
        cmd.unsetf(std::ios::scientific); // Reset to default formatting
    }
    if (max_ratio < 1e15) { // Only include if not unlimited
        cmd << " -ratio " << max_ratio;
    }
    if (verbose_level != 1) {
        cmd << " -v " << verbose_level;
    }

    command_line_preview = cmd.str();
}

bool CausticGUI::validateInputs() {
    if (target_image_path.empty()) {
        error_message = "Target image is required";
        return false;
    }

    if (!std::filesystem::exists(target_image_path)) {
        error_message = "Target image file does not exist: " + target_image_path;
        return false;
    }

    if (!source_image_path.empty() && !std::filesystem::exists(source_image_path)) {
        error_message = "Source image file does not exist: " + source_image_path;
        return false;
    }

    if (resolution < 10 || resolution > 1000) {
        error_message = "Resolution must be between 10 and 1000";
        return false;
    }

    if (focal_length <= 0) {
        error_message = "Focal length must be positive";
        return false;
    }

    if (thickness <= 0) {
        error_message = "Thickness must be positive";
        return false;
    }

    if (mesh_width <= 0) {
        error_message = "Mesh width must be positive";
        return false;
    }

    if (refractive_index < 1.0 || refractive_index > 3.0) {
        error_message = "Refractive index must be between 1.0 and 3.0";
        return false;
    }

    if (max_iterations < 1 || max_iterations > 50000) {
        error_message = "Max iterations must be between 1 and 50000";
        return false;
    }

    if (threshold <= 0 || threshold > 1.0) {
        error_message = "Convergence threshold must be between 0 and 1.0";
        return false;
    }

    if (max_ratio < 1.0) {
        error_message = "Max density ratio must be at least 1.0";
        return false;
    }

    if (verbose_level < 0 || verbose_level > 10) {
        error_message = "Verbose level must be between 0 and 10";
        return false;
    }

    return true;
}

void CausticGUI::resetToDefaults() {
    target_image_path.clear();
    source_image_path.clear();
    output_path = "./output.obj";
    resolution = 100;
    focal_length = 1.0f;
    thickness = 0.2f;
    mesh_width = 1.0f;
    refractive_index = 1.55;
    beta_method = 1;
    max_iterations = 1000;
    threshold = 1e-7;
    max_ratio = (std::numeric_limits<double>::max)();
    verbose_level = 1;

    processing_complete = false;
    processing_error = false;
    error_message.clear();
    auto_generate_filename = false;

    updateCommandLinePreview();
}

std::string CausticGUI::generateParameterFilename(const std::string& baseFilename) {
    if (!auto_generate_filename) {
        return baseFilename;
    }

    // Extract directory and base name from the original filename
    std::filesystem::path originalPath(baseFilename);
    std::string directory = originalPath.parent_path().string();
    std::string baseName = originalPath.stem().string();
    std::string extension = originalPath.extension().string();

    // If no extension, assume .obj
    if (extension.empty()) {
        extension = ".obj";
    }

    // Build parameter string
    std::stringstream paramStr;

    // Add resolution
    paramStr << "res-" << resolution;

    // Add focal length (format to avoid decimals when possible)
    if (focal_length == static_cast<int>(focal_length)) {
        paramStr << "--focal_l-" << static_cast<int>(focal_length);
    } else {
        paramStr << "--focal_l-" << std::fixed << std::setprecision(1) << focal_length;
    }

    // Add thickness
    if (thickness == static_cast<int>(thickness)) {
        paramStr << "--thickness-" << static_cast<int>(thickness);
    } else {
        paramStr << "--thickness-" << std::fixed << std::setprecision(1) << thickness;
    }

    // Add mesh width
    if (mesh_width == static_cast<int>(mesh_width)) {
        paramStr << "--mesh_width-" << static_cast<int>(mesh_width);
    } else {
        paramStr << "--mesh_width-" << std::fixed << std::setprecision(1) << mesh_width;
    }

    // Add refractive index if different from default
    if (refractive_index != 1.55) {
        paramStr << "--ri-" << std::fixed << std::setprecision(2) << refractive_index;
    }

    // Add beta method
    paramStr << "--beta-" << (beta_method == 0 ? "0" : "cj");

    // Add advanced parameters if they differ from defaults
    if (max_iterations != 1000) {
        paramStr << "--itr-" << max_iterations;
    }

    if (threshold != 1e-7) {
        paramStr << "--th-1e" << static_cast<int>(std::log10(threshold));
    }

    if (max_ratio < 1e15) {
        paramStr << "--ratio-" << static_cast<int>(max_ratio);
    }

    // Construct final filename
    std::string finalFilename;
    if (!directory.empty()) {
        finalFilename = directory + "/" + paramStr.str() + "_" + baseName + extension;
    } else {
        finalFilename = paramStr.str() + "_" + baseName + extension;
    }

    return finalFilename;
}

CLIopts CausticGUI::createCLIOptsFromGUI() {
    CLIopts opts;
    opts.set_default();

    opts.filename_trg = target_image_path;
    opts.filename_src = source_image_path;
    opts.uniform_src = source_image_path.empty();
    opts.output_path = generateParameterFilename(output_path);
    opts.resolution = static_cast<unsigned int>(resolution);
    opts.focal_l = static_cast<double>(focal_length);
    opts.thickness = static_cast<double>(thickness);
    opts.mesh_width = static_cast<double>(mesh_width);
    opts.refractive_index = refractive_index;

    // Solver options
    opts.solver_opt.beta = (beta_method == 0) ? otmap::BetaOpt::Zero : otmap::BetaOpt::ConjugateJacobian;
    opts.solver_opt.max_iter = max_iterations;
    opts.solver_opt.threshold = threshold;
    opts.solver_opt.max_ratio = max_ratio;
    opts.verbose_level = verbose_level;

    return opts;
}

void CausticGUI::startProcessing() {
    if (!validateInputs()) {
        processing_error = true;
        processing_complete = true;
        return;
    }

    processing_error = false;
    processing_complete = false;
    is_processing = true;

    {
        std::lock_guard<std::mutex> lock(status_mutex);
        processing_status = "Initializing...";
    }

    CLIopts opts = createCLIOptsFromGUI();

    // Start processing in background thread
    if (processing_thread.joinable()) {
        processing_thread.join();
    }

    processing_thread = std::thread(&CausticGUI::processInBackground, this, opts);
}

void CausticGUI::processInBackground(CLIopts opts) {
    try {
        {
            std::lock_guard<std::mutex> lock(status_mutex);
            processing_status = "Processing caustic generation...";
        }

        // Call the main processing function (to be implemented)
        int result = processCausticGeneration(opts);

        if (result == 0) {
            processing_error = false;
            {
                std::lock_guard<std::mutex> lock(status_mutex);
                processing_status = "Completed successfully!";
            }
        } else {
            processing_error = true;
            error_message = "Processing failed with error code: " + std::to_string(result);
        }
    } catch (const std::exception& e) {
        processing_error = true;
        error_message = std::string("Exception during processing: ") + e.what();
    } catch (...) {
        processing_error = true;
        error_message = "Unknown error during processing";
    }

    processing_complete = true;
    is_processing = false;
}

#ifdef _WIN32
bool CausticGUI::initializeOpenGL() {
    // Create window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
                     GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                     "CausticDesignGUI", NULL };
    RegisterClassEx(&wc);

    // Create window
    HWND hwnd_temp = CreateWindow(wc.lpszClassName, "Caustic Design Generator",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 1200, 800,
                                 NULL, NULL, wc.hInstance, NULL);

    if (!hwnd_temp) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }

    hwnd = hwnd_temp;

    // Get device context
    HDC hdc_temp = GetDC(hwnd_temp);
    if (!hdc_temp) {
        std::cerr << "Failed to get device context" << std::endl;
        return false;
    }
    hdc = hdc_temp;

    // Set pixel format
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,
        8,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    int pixelFormat = ChoosePixelFormat((HDC)hdc, &pfd);
    if (!pixelFormat) {
        std::cerr << "Failed to choose pixel format" << std::endl;
        return false;
    }

    if (!SetPixelFormat((HDC)hdc, pixelFormat, &pfd)) {
        std::cerr << "Failed to set pixel format" << std::endl;
        return false;
    }

    // Create OpenGL context
    HGLRC hglrc_temp = wglCreateContext((HDC)hdc);
    if (!hglrc_temp) {
        std::cerr << "Failed to create OpenGL context" << std::endl;
        return false;
    }
    hglrc = hglrc_temp;

    if (!wglMakeCurrent((HDC)hdc, (HGLRC)hglrc)) {
        std::cerr << "Failed to make OpenGL context current" << std::endl;
        return false;
    }

    // Show window
    ShowWindow(hwnd_temp, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_temp);

    return true;
}
#else
bool CausticGUI::initializeOpenGL() {
    std::cerr << "OpenGL initialization not implemented for this platform" << std::endl;
    return false;
}
#endif

bool CausticGUI::initializeImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
#ifdef _WIN32
    if (!ImGui_ImplWin32_Init(hwnd)) {
        std::cerr << "Failed to initialize ImGui Win32 backend" << std::endl;
        return false;
    }
#endif

    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        std::cerr << "Failed to initialize ImGui OpenGL3 backend" << std::endl;
        return false;
    }

    return true;
}

void CausticGUI::cleanup() {
    // Wait for processing thread to finish
    if (processing_thread.joinable()) {
        processing_thread.join();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
#ifdef _WIN32
    ImGui_ImplWin32_Shutdown();
#endif
    ImGui::DestroyContext();

#ifdef _WIN32
    // Cleanup OpenGL
    if (hglrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext((HGLRC)hglrc);
        hglrc = nullptr;
    }

    if (hdc && hwnd) {
        ReleaseDC((HWND)hwnd, (HDC)hdc);
        hdc = nullptr;
    }

    if (hwnd) {
        DestroyWindow((HWND)hwnd);
        hwnd = nullptr;
    }

    UnregisterClass("CausticDesignGUI", GetModuleHandle(NULL));
#endif
}

// Entry point for GUI mode
int launchGUI() {
    CausticGUI gui;
    return gui.run();
}
