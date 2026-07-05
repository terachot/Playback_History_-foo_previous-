cmake_minimum_required(VERSION 3.15)
project(my_fb2k_extension)

# ===== Settings =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PROJECT_VERSION "1.0")

# SDK Path - เปลี่ยนให้ตรงกับเครื่องคุณ
set(FB2K_SDK_PATH "D:/Doc/Work/foobar2000/my_fb2k_extension/sdk/foobar2000/SDK" CACHE PATH "foobar2000 SDK path")

if(NOT EXISTS "${FB2K_SDK_PATH}")
    message(FATAL_ERROR "SDK not found at ${FB2K_SDK_PATH}")
endif()

# ===== Source Files =====
set(SOURCES
    src/stdafx.cpp
    src/extension.cpp
    src/dllmain.cpp
    src/history_manager.cpp
    src/history_panel.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/foobar2000/foobar2000_component_client/component_client.cpp"
)

set(HEADERS
    src/stdafx.h
    src/history_manager.h
    src/history_panel.h
)

# ===== Create DLL =====
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# ===== Include Directories =====
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/foobar2000"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk"
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ===== Compiler Flags =====
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3           # Warning level 3
        /WX-          # Don't treat warnings as errors
        /O2           # Optimize for speed
        /Oy-          # Don't omit frame pointers
    )

    # Precompiled header - เฉพาะไฟล์ของเราเอง ไม่รวม component_client.cpp
    set_source_files_properties(src/stdafx.cpp PROPERTIES
        COMPILE_FLAGS "/Yc\"stdafx.h\""
    )

    set_source_files_properties(src/extension.cpp src/dllmain.cpp src/history_manager.cpp src/history_panel.cpp PROPERTIES
        COMPILE_FLAGS "/Yu\"stdafx.h\""
    )

    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS)
elseif(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -O2)
endif()

# ===== External SDK Projects =====
include_external_msproject(foobar2000_SDK_proj "${FB2K_SDK_PATH}/foobar2000_SDK.vcxproj")
include_external_msproject(pfc_proj "${CMAKE_CURRENT_SOURCE_DIR}/sdk/pfc/pfc.vcxproj")
include_external_msproject(shared_proj "${CMAKE_CURRENT_SOURCE_DIR}/sdk/foobar2000/shared/shared.vcxproj")

# ===== Link Libraries =====
target_link_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/foobar2000/shared/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/foobar2000/shared/$<CONFIG>"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/pfc/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdk/pfc/$<CONFIG>"
    "${FB2K_SDK_PATH}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>"
    "${FB2K_SDK_PATH}/$<CONFIG>"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    shared
    pfc
    foobar2000_SDK
    ole32
    kernel32
    user32
    gdi32
    winspool
    shell32
    oleaut32
    uuid
    comdlg32
    advapi32
)

add_dependencies(${PROJECT_NAME} foobar2000_SDK_proj pfc_proj shared_proj)

# ===== Definitions =====
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32
    _WINDOWS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# ===== Output Settings =====
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "foo_previous"
    PREFIX ""
)

# ===== Message =====
message(STATUS "")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "SDK: ${FB2K_SDK_PATH}")
message(STATUS "")
message(STATUS "Build with:")
message(STATUS "  cmake -B build")
message(STATUS "  cmake --build build --config Release")
message(STATUS "")