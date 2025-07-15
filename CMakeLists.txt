project(otmap)

cmake_minimum_required(VERSION 3.2.0)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(USE_CIMAGE true)

# Option to build a single executable (static linking)
option(BUILD_SINGLE_EXECUTABLE "Build a single, self-contained executable" ON)

# Force static linking for single executable builds
if(BUILD_SINGLE_EXECUTABLE)
    message(STATUS "Building single executable with static linking")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

    # Force static runtime on Windows
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        # Additional static linking flags
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    endif()

    # Prefer static libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a)
endif()

# Force MSVC to use the static runtime (/MT or /MTd)
if(MSVC)
  foreach(flag_var CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                   CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
    string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
  endforeach()
endif()

if(USE_CIMAGE)
    add_definitions(-D_cimg_)
    add_definitions(-Dcimg_display=0)

    find_package(PNG REQUIRED)
    if(PNG_FOUND)
        add_definitions(-Dcimg_use_png)
        set(ALLLIBS ${ALLLIBS} PNG::PNG)
    endif()

    find_package(JPEG)
    if(JPEG_FOUND)
        add_definitions(-Dcimg_use_jpeg)
        set(ALLLIBS ${ALLLIBS} JPEG::JPEG)
    endif()

   if(WIN32)
      add_definitions(-Dcimg_OS=2)
   else()
      add_definitions(-Dcimg_OS=1)
   endif()
endif()

include_directories(extern/)
include_directories(extern/eigen/)
include_directories(otlib/)

find_package(Cholmod)
if(CHOLMOD_FOUND)
  message(STATUS "Enable Cholmod support")
  add_definitions("-DHAS_CHOLMOD")
  include_directories(${CHOLMOD_INCLUDES})
  set(ALLLIBS ${ALLLIBS} ${CHOLMOD_LIBRARIES})
else()
  message(STATUS "Disable Cholmod support (not found)")
  message(WARNING "Chomod not found, solving will be significantly slower than expected.")
endif()

find_package(Ceres REQUIRED)

# ImGui and OpenGL for GUI support
find_package(OpenGL REQUIRED)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

# Create ImGui as static library for single executable builds
if(BUILD_SINGLE_EXECUTABLE)
    add_library(imgui STATIC ${IMGUI_SOURCES})
else()
    add_library(imgui ${IMGUI_SOURCES})
endif()

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC OpenGL::GL)

## FILES ##########################################################################
set(SURFACE_MESH_SRC_FILES
    otlib/surface_mesh/Surface_mesh.cpp
)

set(SURFACE_MESH_HEADER_FILES
    otlib/surface_mesh/properties.h
    otlib/surface_mesh/Surface_mesh.h
    otlib/surface_mesh/types.h
    otlib/surface_mesh/Vector.h
)

set(OTSOLVER_SRC_FILES
    otlib/transport_map.cpp
    otlib/otsolver_2dgrid.cpp
    otlib/details/line_search.cpp
    otlib/details/nested_dissection.cpp
    otlib/utils/bvh2d.cpp
    otlib/utils/rasterizer.cpp
    otlib/utils/stochastic_rasterizer.cpp
    otlib/utils/mesh_utils.cpp
)

set(OTSOLVER_HEADER_FILES
    otlib/transport_map.h
    otlib/otsolver_2dgrid.h
    otlib/details/line_search.h
    otlib/details/nested_dissection.h
    otlib/utils/bvh2d.h
    otlib/utils/rasterizer.h
    otlib/utils/stochastic_rasterizer.h
    otlib/utils/mesh_utils.h
)

###################################################################################

add_definitions(-DDATA_DIR="${PROJECT_SOURCE_DIR}/data")

if(NOT WIN32)
   add_definitions(-DENABLE_SSE_MODE)
else()
    add_definitions(-D_USE_MATH_DEFINES)
endif()

# Create otlib as static for single executable builds
if(BUILD_SINGLE_EXECUTABLE)
    add_library(otlib STATIC ${SURFACE_MESH_SRC_FILES} ${OTSOLVER_SRC_FILES})
else()
    add_library(otlib ${SURFACE_MESH_SRC_FILES} ${OTSOLVER_SRC_FILES})
endif()



set(OTAPPUTILS_SRC_FILES
    apps/common/image_utils.cpp
    apps/common/inputparser.cpp
    apps/common/analytical_functions.cpp
    apps/common/generic_tasks.cpp
    apps/normal_integration/normal_integration.cpp
    apps/normal_integration/mesh.cpp
)

# GUI source files
set(GUI_SRC_FILES
    apps/gui/caustic_gui.cpp
    apps/gui/file_dialog.cpp
)

set(GUI_HEADER_FILES
    apps/gui/caustic_gui.h
    apps/gui/file_dialog.h
)

set(OTAPPUTILS_HEADER_FILES
    apps/common/image_utils.h
    apps/common/inputparser.h
    apps/common/analytical_functions.h
    apps/common/generic_tasks.h
    apps/common/otsolver_options.h
    apps/normal_integration/normal_integration.h
    apps/normal_integration/mesh.h
    apps/normal_integration/costFunctor.h
)

# Create otapputils as static for single executable builds
if(BUILD_SINGLE_EXECUTABLE)
    add_library(otapputils STATIC ${OTAPPUTILS_SRC_FILES})
else()
    add_library(otapputils ${OTAPPUTILS_SRC_FILES})
endif()



add_executable(caustic_design apps/caustic_design.cpp ${GUI_SRC_FILES})

target_link_libraries(caustic_design PRIVATE PNG::PNG)

target_link_libraries(otapputils
    PRIVATE
        Ceres::ceres
        PNG::PNG
        # JPEG::JPEG
)

target_link_libraries(caustic_design
    PRIVATE
        otapputils
        otlib
        ${ALLLIBS}
        Ceres::ceres
        imgui
        OpenGL::GL
        comctl32
        comdlg32
)

# Additional configuration for single executable builds
if(BUILD_SINGLE_EXECUTABLE)
    # Set static linking flags for Windows
    if(WIN32)
        # Link statically against the C++ runtime
        set_target_properties(caustic_design PROPERTIES
            LINK_FLAGS "/NODEFAULTLIB:MSVCRT /NODEFAULTLIB:MSVCRTD"
        )

        # Add additional system libraries that might be needed
        target_link_libraries(caustic_design PRIVATE
            kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid
            comdlg32 advapi32 ws2_32 winmm imm32 version
        )
    endif()

    # Strip debug symbols in release builds to reduce size
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(WIN32)
            set_target_properties(caustic_design PROPERTIES
                LINK_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF"
            )
        endif()
    endif()

    # Set executable properties
    set_target_properties(caustic_design PROPERTIES
        OUTPUT_NAME "caustic_design_standalone"
    )

    message(STATUS "Single executable will be named: caustic_design_standalone.exe")
endif()

# Add custom target for building single executable
if(BUILD_SINGLE_EXECUTABLE)
    add_custom_target(single_exe
        DEPENDS caustic_design
        COMMENT "Building single executable..."
    )

    # Add post-build step to show completion message
    add_custom_command(TARGET caustic_design POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Single executable created:"
        COMMAND ${CMAKE_COMMAND} -E echo "  Location: $<TARGET_FILE:caustic_design>"
        COMMENT "Single executable build complete"
    )
endif()

# Print build configuration summary
message(STATUS "=== Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Single Executable: ${BUILD_SINGLE_EXECUTABLE}")
if(BUILD_SINGLE_EXECUTABLE)
    message(STATUS "Output: caustic_design_standalone.exe (self-contained)")
else()
    message(STATUS "Output: caustic_design.exe (requires DLLs)")
endif()
message(STATUS "==========================")
