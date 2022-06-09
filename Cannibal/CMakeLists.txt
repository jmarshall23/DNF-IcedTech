project(Cannibal CXX)

add_library(Cannibal SHARED
    CorMain.cpp CorMain.h
    Kernel.cpp Kernel.h
    KrnBuild.h
    KrnDefs.h
    KrnInc.h
    KrnTypes.h
    msg.asm
    StrMain.cpp StrMain.h
    MathFlt.h
    VecMain.h
    VecPrim.h
    LogMain.cpp LogMain.h
    TimeMain.cpp TimeMain.h
    MemMain.cpp MemMain.h
    FileBox.cpp FileBox.h
    FileMain.cpp FileMain.h
    LexMain.cpp LexMain.h
    PrsMain.cpp PrsMain.h
    MsgMain.cpp MsgMain.h
    ObjMain.cpp ObjMain.h
    InDefs.h
    InMain.cpp InMain.h
    CpjFmt.h
    CpjFrm.cpp CpjFrm.h
    CpjGeo.cpp CpjGeo.h
    CpjLod.cpp CpjLod.h
    CpjMac.cpp CpjMac.h
    CpjMain.cpp CpjMain.h
    CpjProj.cpp CpjProj.h
    CpjSeq.cpp CpjSeq.h
    CpjSkl.cpp CpjSkl.h
    CpjSrf.cpp CpjSrf.h
    Res/CpjCpj.bmp
    Res/CpjFrm.bmp
    Res/CpjGeo.bmp
    Res/CpjLod.bmp
    Res/CpjMac.bmp
    Res/CpjSeq.bmp
    Res/CpjSkl.bmp
    Res/CpjSrf.bmp
    Res/FileClosed.bmp
    Res/FileOpen.bmp
    Res/MacEdit.rc
    Res/resource.h
    MacEdit.cpp MacEdit.h
    MacMain.cpp MacMain.h
    PlgMain.cpp PlgMain.h
    IpcMain.cpp IpcMain.h
    WinCtrl.cpp WinCtrl.h
    Cannibal.h
)

target_include_directories(Cannibal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ../Engine/Inc ../Core/Inc)
target_compile_definitions(Cannibal PRIVATE CANNIBAL_EXPORTS KRN_EXPORTS KRN_DLL XCORE_PURE)
target_link_directories(Cannibal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Cannibal PRIVATE comctl32)
