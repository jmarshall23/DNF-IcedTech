project(dnAI CXX)


file(GLOB dnAIFiles
     "*.cpp"
     "../inc/*.h"     
)

file(GLOB dnAIClasses 
    "../classes/*.uc"
)

source_group("Classes" FILES ${dnAIClasses})

add_library(dnAI SHARED ${dnAIFiles} ${dnAIClasses})

target_link_libraries(dnAI PRIVATE Engine)

target_include_directories(dnAI PRIVATE
    ../../Core/Inc
    ../../Engine/Inc
    ../inc/
)

target_compile_definitions(dnAI PRIVATE
    "_UNICODE" "UNICODE"
    "DNAI_API=__declspec(dllexport)"
)

target_compile_definitions(dnAI PRIVATE _UNICODE UNICODE)
