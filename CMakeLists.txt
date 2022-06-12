cmake_minimum_required(VERSION 3.22)
project(Duke4)

# Output binaries to the System directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/System)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Allow addressing more memory on a 64-bit host.
add_link_options("/LARGEADDRESSAWARE")

# Project is not compatible with data execution prevention.
add_link_options("/NXCOMPAT:NO")

# Project is not compatible with safe exception handlers.
add_link_options("/SAFESEH:NO")

# There are multiple locations in the source that invoke
# va_start with a C++ reference. This is not portable, but
# can be made to compile by disabling validation.
add_compile_definitions(_CRT_NO_VA_START_VALIDATION)

# 4-byte structure packing is required for compatibility.
add_compile_options("/Zp4")
add_compile_definitions(WINDOWS_IGNORE_PACKING_MISMATCH)

# Microsoft extension that allows locals defined in a for
# loop initializer to leak into the outside scope:
#
# for (int i = 0; i < N; i++);
# for (i = 0; i < N; i++);
# i = 0;
#
# Required by a significant portion of the source.
add_compile_options("/Zc:forScope-")

# The code doesn't like wchar_t being treated as a built-in type.
add_compile_options("/Zc:wchar_t-")

# Perform multi-process builds.
add_compile_options("/MP")

# DirectX
include_directories(DirectX8/Inc)
link_directories(DirectX8/Lib)

# Projects
add_subdirectory(dnGame/Src)
add_subdirectory(dnAI/Src)
add_subdirectory(Engine/Src)
add_subdirectory(Launch)
add_subdirectory(UCC/Src)
add_subdirectory(NullAudio/Src)
add_subdirectory(Fire/Src)
add_subdirectory(D3DDrv/Src)
add_subdirectory(Editor/Src)
add_subdirectory(DukeEd)
add_subdirectory(OpenALAudio/Src)
