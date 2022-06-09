project(dnGame CXX)


file(GLOB dnGameFiles
     "*.cpp"
     "../inc/*.h"     
)

file(GLOB dnGameClasses 
    "../classes/*.uc"
)

source_group("Classes" FILES ${dnGameClasses})

add_library(dnGame SHARED ${dnGameFiles} ${dnGameClasses})

target_link_libraries(dnGame PRIVATE Engine)

target_include_directories(dnGame PRIVATE
    ../../Core/Inc
    ../../Engine/Inc
    ../inc/
)

target_compile_definitions(dnGame PRIVATE
    "_UNICODE" "UNICODE"
    "DNGAME_API=__declspec(dllexport)"
)

target_compile_definitions(dnGame PRIVATE _UNICODE UNICODE)
