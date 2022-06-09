project(OpenALAudio C CXX)

file(GLOB OpenALAudioFiles
     "*.h"
     "*.cpp"
     "../external/libvorbis/*.c"
     "../external/libogg/*.c"
)

add_library(OpenALAudio SHARED ${OpenALAudioFiles})

target_link_directories(OpenALAudio PRIVATE ../external/libs/win32)
target_link_libraries(OpenALAudio PRIVATE Engine OpenAL32)

target_include_directories(OpenALAudio PRIVATE
    ../../Core/Inc
    ../../Engine/Inc
    ../External/include
    ../External/libogg
    ../External/libvorbis
)

target_compile_definitions(OpenALAudio PRIVATE _UNICODE UNICODE)
