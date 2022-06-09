/*=============================================================================
	dnSinglePlayer.cpp

	IceColdDuke - Port of dnSinglePlayer.uc to C++
=============================================================================*/
#include <Engine.h>

#include "dnGameClasses.h"

IMPLEMENT_CLASS(AdnSinglePlayer)

/*-----------------------------------------------------------------------------
	ADukeNet object implementation.
-----------------------------------------------------------------------------*/

void AdnSinglePlayer::execAddDefaultInventory(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(APawn, InventoryPawn);
	P_FINISH

	static UClass* PistolClass = FindObject<UClass>(ANY_PACKAGE, TEXT("Pistol"));

	AGameInfo::eventAddDefaultInventoryBase(InventoryPawn);

	if (!bNoPistol)
	{
		eventGiveWeaponTo(InventoryPawn, PistolClass, true);
	}
}

