project(NullAudio CXX)

add_library(NullAudio SHARED NullAudio.cpp)

target_link_libraries(NullAudio PRIVATE Engine)

target_include_directories(NullAudio PRIVATE
    ../../Core/Inc
    ../../Engine/Inc
)

target_compile_definitions(NullAudio PRIVATE _UNICODE UNICODE)
