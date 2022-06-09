/*=============================================================================
	pigcop.cpp

	Rev 1: new Pigcop AI in C++
=============================================================================*/
#include <Engine.h>

#include "dnAI.h"
#include "dnAIClasses.h"

IMPLEMENT_CLASS(APigCop)

#define APPROACH_STATE_BEGIN 0
#define APPROACH_STATE_MOVEING 1

static DnHDSound roam1Sound(TEXT("PIGRM"));
static DnHDSound roam2Sound(TEXT("roam67"));
static DnHDSound roam3Sound(TEXT("pigrm"));
static DnHDSound fireSound(TEXT("SHOTGUN7"));


void APigCop::execBeginAI(FFrame& Stack, RESULT_DECL)
{
	P_FINISH

	ApproachState = APPROACH_STATE_BEGIN;
	NativeRequestedAnimation = TEXT("IdleA");

	static UClass* ShotgunClass = FindObject<UClass>(ANY_PACKAGE, TEXT("Shotgun"));
	Weapon = Cast<AWeapon>(GetLevel()->SpawnActor(ShotgunClass));	
}

void APigCop::execTickAI(FFrame& Stack, RESULT_DECL)
{
	P_GET_FLOAT(DeltaTime)
	P_FINISH

	UAudioSubsystem* audio = GetLevel()->Engine->Audio;

	EnemyIsVisible = false;

	// If we don't have a current enemy then we are assumed to be idling and playing the roaming sounds.
	if (Enemy == nullptr)
	{
		if (!audio->IsActorPlayingAudio(this))
		{
			if (dnRand.ifrnd(1))
			{
				if (dnRand.ifrnd(32))
				{					
					audio->PlaySound(this, 0, &roam1Sound, this->Location, 1.0f, 1600.f, 1.0f, false);
				}
				else
				{
					if (dnRand.ifrnd(64))
					{						
						audio->PlaySound(this, 0, &roam2Sound, this->Location, 1.0f, 1600.f, 1.0f, false);
					}
					else
					{						
						audio->PlaySound(this, 0, &roam3Sound, this->Location, 1.0f, 1600.f, 1.0f, false);
					}
				}
			}
		}
	}
	else
	{
		EnemyIsVisible = GetLevel()->Model->FastLineCheck(Location, Enemy->Location);
		if (EnemyIsVisible)
		{
			enemyLastSeenLocation = Enemy->Location;
			MoveTimer = -1;
		}
	}
}

void APigCop::execStateSeePlayer(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(AActor, SeenPlayer);
	P_FINISH

	UAudioSubsystem* audio = GetLevel()->Engine->Audio;

	static UClass* PlayerPawnClass = FindObject<UClass>(ANY_PACKAGE, TEXT("PlayerPawn"));
	if (SeenPlayer->IsA(PlayerPawnClass))
	{
		static DnHDSound wakeupSound(TEXT("PIGRG"));
		audio->PlaySound(this, 0, &wakeupSound, this->Location, 1.0f, 1600.f, 1.0f, false);

		Enemy = SeenPlayer;
		*(DWORD*)Result = 1;
		return;
	}

	*(DWORD*)Result = 0;
}

void APigCop::execStateShootEnemy(FFrame& Stack, RESULT_DECL)
{
	P_FINISH

	UAudioSubsystem* audio = GetLevel()->Engine->Audio;

	static UClass* fakeDamageClass = FindObject<UClass>(ANY_PACKAGE, TEXT("CrushingDamage"));

	// If not visible don't shoot.
	if (!EnemyIsVisible)
	{
		*(DWORD*)Result = 0;
		return;
	}

	Enemy->eventTakeDamage(5, this, Enemy->Location, Velocity, fakeDamageClass);

	audio->PlaySound(this, 0, &fireSound, this->Location, 1.0f, 1600.f, 1.0f, false);

	*(DWORD*)Result = 1;
}

void APigCop::execStateApproachingEnemy(FFrame& Stack, RESULT_DECL)
{
	P_FINISH

	ARenderActor* EnemeyRenderActor = Cast<ARenderActor>(Enemy);

	float distToEnemy = 0.0f;

	if (Enemy)
	{
		distToEnemy = (Enemy->Location - Location).Size();
	}

	switch (ApproachState)
	{
		case APPROACH_STATE_BEGIN:
			if (Enemy != nullptr)
			{
				Destination = Enemy->Location;
				HeadTrackingActor = Enemy;
			}
			else if (Enemy == nullptr || EnemeyRenderActor->Health <= 0)
			{
				Enemy = nullptr;
				eventStopMovingNative();
				NativeRequestState = TEXT("Idling");
			}
			ApproachState = 1;
			break;

		case APPROACH_STATE_MOVEING:
			NativeRequestedAnimation = TEXT("A_Run_ShotGun");
			if (distToEnemy > 96)
			{
				NativeRequestMoveLocation = Enemy->Location;
			}
			break;
	}
}