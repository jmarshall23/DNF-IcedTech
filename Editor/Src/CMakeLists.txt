project(Editor CXX)

add_library(Editor SHARED
    CoolBsp.cpp
    EdHook.cpp
    Editor.cpp
    EditorPrivate.h
    TTFontImport.cpp
    UBatchExportCommandlet.cpp
    UBrushBuilder.cpp
    UConformCommandlet.cpp
    UKillUnrealCommandlet.cpp
    UMakeCommandlet.cpp
    UMapConvertCommandlet.cpp
    UMasterCommandlet.cpp
    UMergeDXTCommandlet.cpp
    UnBsp.cpp
    UnEdAct.cpp
    UnEdCam.cpp
    UnEdClick.cpp
    UnEdCnst.cpp
    UnEdCsg.cpp
    UnEdExp.cpp
    UnEdFact.cpp
    UnEditor.cpp
    UnEdRend.cpp
    UnEdSrv.cpp
    UnEdTran.cpp UnEdTran.h
    UnMeshEd.cpp
    UnMeshLP.cpp
    UnParams.cpp
    UnScrCom.cpp UnScrCom.h
    UnShadow.cpp
    UnSyntax.cpp
    UnTopics.cpp UnTopics.h
    UnVisi.cpp
    UTextureCheckCommandlet.cpp
    ../Inc/Editor.h
    ../Inc/EditorClasses.h
    ../Inc/UBrushBuilder.h
)

target_compile_definitions(Editor PRIVATE
    _UNICODE UNICODE
    "EDITOR_API=__declspec(dllexport)"
)

target_link_libraries(Editor PRIVATE Engine dnAI dnGame OpenALAudio)
target_include_directories(Editor PRIVATE
    ./
    ../Inc
    ../../Core/Inc
    ../../Engine/Inc
    ../../Window/Inc
)
