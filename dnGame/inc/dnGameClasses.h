/*===========================================================================
    C++ class definitions exported from UnrealScript.
    This is automatically generated by the tools.
    DO NOT modify this manually! Edit the corresponding .uc files instead!
===========================================================================*/
#if _MSC_VER
#pragma pack (push,4)
#endif

#ifndef DNGAME_API
#define DNGAME_API DLL_IMPORT
#endif

#ifndef NAMES_ONLY
#define AUTOGENERATE_NAME(name) extern DNGAME_API FName DNGAME_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#endif


#if !defined(NAMES_ONLY) || defined(DN_FORCE_NAME_EXPORT)


class DNGAME_API AdnSinglePlayer : public AGameInfo
{
public:
    class ADukePlayer* SinglePlayerDuke;
    class AActor* SpeechCoordinator;
    BITFIELD bGruntSpeechDisabled:1 GCC_PACK(4);
    BITFIELD bNoPistol:1;
    DECLARE_FUNCTION(execAddDefaultInventory);
    DECLARE_CLASS(AdnSinglePlayer,AGameInfo,0|CLASS_Config)
    NO_DEFAULT_CONSTRUCTOR(AdnSinglePlayer)
};


class DNGAME_API AdnWeapon : public AWeapon
{
public:
    BITFIELD bAmmoItem:1 GCC_PACK(4);
    class AHUDIndexItem_Ammo* AmmoItem GCC_PACK(4);
    class UClass* AmmoItemClass;
    BITFIELD bAltAmmoItem:1 GCC_PACK(4);
    class AHUDIndexItem_AltAmmo* AltAmmoItem GCC_PACK(4);
    class UClass* AltAmmoItemClass;
    class AMightyFoot* DukeFoot;
    class AdnShellCaseMaster* ShellMaster;
    class AdnShellCaseMaster* ShellMaster3rd;
    INT CrosshairIndex;
    BITFIELD bReloadOnModeChange:1 GCC_PACK(4);
    DECLARE_CLASS(AdnWeapon,AWeapon,0)
    NO_DEFAULT_CONSTRUCTOR(AdnWeapon)
};


class DNGAME_API AShotgun : public AdnWeapon
{
public:
    INT PelletCount;
    FLOAT PelletRandHoriz;
    FLOAT PelletRandVert;
    INT Pellet;
    FLOAT ShotHoriz[10];
    FLOAT ShotVert[10];
    class AHitPackage_Shotgun* MetaHit;
    INT MetaHitIndex;
    BITFIELD ClientInterruptReload:1 GCC_PACK(4);
    DECLARE_CLASS(AShotgun,AdnWeapon,0)
    NO_DEFAULT_CONSTRUCTOR(AShotgun)
};


class DNGAME_API APistol : public AdnWeapon
{
public:
    FLOAT SustainedRefireTime;
    BITFIELD bReFireState:1 GCC_PACK(4);
    BITFIELD bReFireReady:1;
    BITFIELD bReFireDone:1;
    class AActor* FireOffsetMarker GCC_PACK(4);
    FLOAT LastRefireTime;
    class UTexture* PistolClips[3];
    FWAMEntry SAnimFireHP[4];
    FWAMEntry SAnimFireAP[4];
    DECLARE_CLASS(APistol,AdnWeapon,0)
    NO_DEFAULT_CONSTRUCTOR(APistol)
};

#endif

AUTOGENERATE_FUNCTION(AdnSinglePlayer,-1,execAddDefaultInventory);

#ifndef NAMES_ONLY
#undef AUTOGENERATE_NAME
#undef AUTOGENERATE_FUNCTION
#endif NAMES_ONLY

#if _MSC_VER
#pragma pack (pop)
#endif
