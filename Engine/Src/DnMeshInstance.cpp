#include "EnginePrivate.h"

/*
	UDukeMeshInstance
*/
IMPLEMENT_CLASS(UDukeMeshInstance);

UDukeMeshInstance::UDukeMeshInstance()
{
	Mesh = NULL;
	Actor = NULL;

	mDukeScale = FVector(1, 1, 1);
	mDukeOrigin = FVector(0, 0, 0);
}

FDukeSkelBone* UDukeMeshInstance::FindBone(FName& name)
{
	for (int i = 0; i < instanceBones.Num(); i++)
	{
		if (instanceBones(i).name == name)
		{
			return &instanceBones(i);
		}
	}

	return nullptr;
}

void UDukeMeshInstance::Destroy()
{
	Super::Destroy();
}

UMesh* UDukeMeshInstance::GetMesh()
{
	return(Mesh);
}


void UDukeMeshInstance::SetMesh(UMesh* InMesh)
{
	STAT(clock(GStat.SetMeshCycles));

	Mesh = Cast<UDukeMesh>(InMesh);
	if (!Mesh)
	{
		STAT(unclock(GStat.SetMeshCycles));
		return;
	}

	STAT(unclock(GStat.SetMeshCycles));
}

AActor* UDukeMeshInstance::GetActor()
{
	return(Actor);
}
void UDukeMeshInstance::SetActor(AActor* InActor)
{
	Actor = InActor;
}

INT UDukeMeshInstance::GetNumSequences()
{
	return Mesh->NumSequences();	
}
HMeshSequence UDukeMeshInstance::GetSequence(INT SeqIndex)
{
	return Mesh->GetSequence(SeqIndex);
}
HMeshSequence UDukeMeshInstance::FindSequence(FName SeqName)
{
	return Mesh->FindSequence(SeqName);
}

FName UDukeMeshInstance::GetSeqName(HMeshSequence Seq)
{
	FDukeSkelSequence* S = (FDukeSkelSequence*)Seq;
	return S->name;	
}
void UDukeMeshInstance::SetSeqGroupName(FName SequenceName, FName GroupName)
{
	Mesh->SequenceGroupMap.Set(SequenceName, GroupName);
	return;
}
FName UDukeMeshInstance::GetSeqGroupName(FName SequenceName)
{
	FName* GroupName = Mesh->SequenceGroupMap.Find(SequenceName);
	if (GroupName == NULL)
		return NAME_None;
	return *GroupName;
}
INT UDukeMeshInstance::GetSeqNumFrames(HMeshSequence Seq)
{
	FDukeSkelSequence* S = (FDukeSkelSequence*)Seq;
	return(S->frames.Num());
}
FLOAT UDukeMeshInstance::GetSeqRate(HMeshSequence Seq)
{
	return 24; // (S->m_Rate);
}
INT UDukeMeshInstance::GetSeqNumEvents(HMeshSequence Seq)
{
	FDukeSkelSequence* S = (FDukeSkelSequence*)Seq;
	return(S->events.Num());
}
EMeshSeqEvent UDukeMeshInstance::GetSeqEventType(HMeshSequence Seq, INT Index)
{
	FDukeSkelSequence* S = (FDukeSkelSequence*)Seq;
	return((EMeshSeqEvent)S->events(Index).eventType);
}
FLOAT UDukeMeshInstance::GetSeqEventTime(HMeshSequence Seq, INT Index)
{
	FDukeSkelSequence* S = (FDukeSkelSequence*)Seq;
	return(S->events(Index).frame / S->frames.Num());
}
const TCHAR* UDukeMeshInstance::GetSeqEventString(HMeshSequence Seq, INT Index)
{
	FDukeSkelSequence* S = (FDukeSkelSequence*)Seq;
	return(*S->events(Index).paramString);
}

UBOOL UDukeMeshInstance::PlaySequence(HMeshSequence Seq, BYTE Channel, UBOOL bLoop, FLOAT Rate, FLOAT MinRate, FLOAT TweenTime)
{
	if (!Actor || !Seq)
		return(0);

	FMeshChannel* Chan = &MeshChannels[Channel];

	if (Rate != 0.f)
	{
		// PlayAnim or LoopAnim
		if (!bLoop)
		{
			// PlayAnim - Set one-shot animation
			if (Chan->AnimSequence == NAME_None)
				TweenTime = 0.0;
			Chan->AnimSequence = GetSeqName(Seq);
			Chan->AnimRate = Rate * GetSeqRate(Seq) / GetSeqNumFrames(Seq);
			Chan->AnimLast = 1.0 - 1.0 / GetSeqNumFrames(Seq);
			Chan->bAnimNotify = GetSeqNumEvents(Seq) != 0;
			Chan->bAnimFinished = 0;
			Chan->bAnimLoop = 0;
			if (Chan->AnimLast == 0.0)
			{
				Chan->AnimMinRate = 0.0;
				Chan->bAnimNotify = 0;
				Chan->OldAnimRate = 0;
				if (TweenTime > 0.0)
					Chan->TweenRate = 1.0 / TweenTime;
				else
					Chan->TweenRate = 10.0; //tween in 0.1 sec
				Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
				Chan->AnimRate = 0;
			}
			else if (TweenTime > 0.0)
			{
				Chan->TweenRate = 1.0 / (TweenTime * GetSeqNumFrames(Seq));
				Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
			}
			else if (TweenTime == -1.0)
			{
				Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
				if (Chan->OldAnimRate > 0)
					Chan->TweenRate = Chan->OldAnimRate;
				else if (Chan->OldAnimRate < 0) //was velocity based looping
					Chan->TweenRate = ::Max(0.5f * Chan->AnimRate, -1 * Actor->Velocity.Size() * Chan->OldAnimRate);
				else
					Chan->TweenRate = 1.0 / (0.025 * GetSeqNumFrames(Seq));
			}
			else
			{
				Chan->TweenRate = 0.0;
				Chan->AnimFrame = 0.001;
			}
			FPlane OldSimAnim = Chan->SimAnim;
			Chan->OldAnimRate = Chan->AnimRate;
			Chan->SimAnim.X = 10000 * Chan->AnimFrame;
			Chan->SimAnim.Y = 5000 * Chan->AnimRate;
			if (Chan->SimAnim.Y > 32767)
				Chan->SimAnim.Y = 32767;
			Chan->SimAnim.Z = 1000 * Chan->TweenRate;
			Chan->SimAnim.W = 10000 * Chan->AnimLast;

			if (OldSimAnim == Chan->SimAnim)
				Chan->SimAnim.W = Chan->SimAnim.W + 1;
		}
		else
		{
			// LoopAnim - Set looping animation
			if ((Chan->AnimSequence == GetSeqName(Seq)) && Chan->bAnimLoop && Actor->IsAnimating(Channel))
			{
				Chan->AnimRate = Rate * GetSeqRate(Seq) / GetSeqNumFrames(Seq);
				Chan->bAnimFinished = 0;
				Chan->AnimMinRate = MinRate != 0.0 ? MinRate * (GetSeqRate(Seq) / GetSeqNumFrames(Seq)) : 0.0;
				FPlane OldSimAnim = Chan->SimAnim;
				Chan->OldAnimRate = Chan->AnimRate;
				Chan->SimAnim.Y = 5000 * Chan->AnimRate;
				Chan->SimAnim.W = -10000 * (1.0 - 1.0 / GetSeqNumFrames(Seq));
				if (OldSimAnim == Chan->SimAnim)
					Chan->SimAnim.W = Chan->SimAnim.W + 1;

				// Copy channel zero information to stock animation info
				if (!Channel)
				{
					Actor->bAnimFinished = Chan->bAnimFinished;
					Actor->bAnimLoop = Chan->bAnimLoop;
					Actor->bAnimNotify = Chan->bAnimNotify;
					Actor->bAnimBlendAdditive = Chan->bAnimBlendAdditive;
					Actor->AnimSequence = Chan->AnimSequence;
					Actor->AnimFrame = Chan->AnimFrame;
					Actor->AnimRate = Chan->AnimRate;
					Actor->AnimBlend = Chan->AnimBlend;
					Actor->TweenRate = Chan->TweenRate;
					Actor->AnimLast = Chan->AnimLast;
					Actor->AnimMinRate = Chan->AnimMinRate;
					Actor->OldAnimRate = Chan->OldAnimRate;
					Actor->SimAnim = Chan->SimAnim;
				}

				return(1);
			}
			if (Chan->AnimSequence == NAME_None)
				TweenTime = 0.0;
			Chan->AnimSequence = GetSeqName(Seq);
			Chan->AnimRate = Rate * GetSeqRate(Seq) / GetSeqNumFrames(Seq);
			Chan->AnimLast = 1.0 - 1.0 / GetSeqNumFrames(Seq);
			Chan->AnimMinRate = MinRate != 0.0 ? MinRate * (GetSeqRate(Seq) / GetSeqNumFrames(Seq)) : 0.0;
			Chan->bAnimNotify = GetSeqNumEvents(Seq) != 0;
			Chan->bAnimFinished = 0;
			Chan->bAnimLoop = 1;
			if (Chan->AnimLast == 0.0)
			{
				Chan->AnimMinRate = 0.0;
				Chan->bAnimNotify = 0;
				Chan->OldAnimRate = 0;
				if (TweenTime > 0.0)
					Chan->TweenRate = 1.0 / TweenTime;
				else
					Chan->TweenRate = 10.0; //tween in 0.1 sec
				Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
				Chan->AnimRate = 0;
			}
			else if (TweenTime > 0.0)
			{
				Chan->TweenRate = 1.0 / (TweenTime * GetSeqNumFrames(Seq));
				Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
			}
			else if (TweenTime == -1.0)
			{
				Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
				if (Chan->OldAnimRate > 0)
					Chan->TweenRate = Chan->OldAnimRate;
				else if (Chan->OldAnimRate < 0) //was velocity based looping
					Chan->TweenRate = ::Max(0.5f * Chan->AnimRate, -1 * Actor->Velocity.Size() * Chan->OldAnimRate);
				else
					Chan->TweenRate = 1.0 / (0.025 * GetSeqNumFrames(Seq));
			}
			else
			{
				Chan->TweenRate = 0.0;
				Chan->AnimFrame = 0.0001;
			}
			Chan->OldAnimRate = Chan->AnimRate;
			Chan->SimAnim.X = 10000 * Chan->AnimFrame;
			Chan->SimAnim.Y = 5000 * Chan->AnimRate;
			if (Chan->SimAnim.Y > 32767)
				Chan->SimAnim.Y = 32767;
			Chan->SimAnim.Z = 1000 * Chan->TweenRate;
			Chan->SimAnim.W = -10000 * Chan->AnimLast;
		}
	}
	else // (Rate == 0.f)
	{
		// TweenAnim - Tweening an animation from wherever it is, to the start of a specified sequence.
		Chan->AnimSequence = GetSeqName(Seq);
		Chan->AnimLast = 0.0;
		Chan->AnimMinRate = 0.0;
		Chan->bAnimNotify = 0;
		Chan->bAnimFinished = 0;
		Chan->bAnimLoop = 0;
		Chan->AnimRate = 0;
		Chan->OldAnimRate = 0;
		if (TweenTime > 0.0)
		{
			Chan->TweenRate = 1.0 / (TweenTime * GetSeqNumFrames(Seq));
			Chan->AnimFrame = -1.0 / GetSeqNumFrames(Seq);
		}
		else
		{
			Chan->TweenRate = 0.0;
			Chan->AnimFrame = 0.0;
		}
		Chan->SimAnim.X = 10000 * Chan->AnimFrame;
		Chan->SimAnim.Y = 5000 * Chan->AnimRate;
		if (Chan->SimAnim.Y > 32767)
			Chan->SimAnim.Y = 32767;
		Chan->SimAnim.Z = 1000 * Chan->TweenRate;
		Chan->SimAnim.W = 10000 * Chan->AnimLast;
	}

	// Copy channel zero information to stock animation info
	if (!Channel)
	{
		Actor->bAnimFinished = Chan->bAnimFinished;
		Actor->bAnimLoop = Chan->bAnimLoop;
		Actor->bAnimNotify = Chan->bAnimNotify;
		Actor->bAnimBlendAdditive = Chan->bAnimBlendAdditive;
		Actor->AnimSequence = Chan->AnimSequence;
		Actor->AnimFrame = Chan->AnimFrame;
		Actor->AnimRate = Chan->AnimRate;
		Actor->AnimBlend = Chan->AnimBlend;
		Actor->TweenRate = Chan->TweenRate;
		Actor->AnimLast = Chan->AnimLast;
		Actor->AnimMinRate = Chan->AnimMinRate;
		Actor->OldAnimRate = Chan->OldAnimRate;
		Actor->SimAnim = Chan->SimAnim;
	}

	return(1);
}
static void DumpAnimData(AActor* Actor)
{
	debugf(_T("bAnimFinished %d"), Actor->bAnimFinished);
	debugf(_T("bAnimLoop %d"), Actor->bAnimLoop);
	debugf(_T("bAnimNotify %d"), Actor->bAnimNotify);
	debugf(_T("bAnimBlendAdditive %d"), Actor->bAnimBlendAdditive);
	debugf(_T("AnimSequence %s"), *Actor->AnimSequence);
	debugf(_T("AnimFrame %f"), Actor->AnimFrame);
	debugf(_T("AnimBlend %f"), Actor->AnimBlend);
	debugf(_T("TweenRate %f"), Actor->TweenRate);
	debugf(_T("AnimLast %f"), Actor->AnimLast);
	debugf(_T("AnimMinRate %f"), Actor->AnimMinRate);
	debugf(_T("OldAnimRate %f"), Actor->OldAnimRate);
	debugf(_T("bAnimFinished %d"), Actor->bAnimFinished);
	debugf(_T("\n\n"));
}

void UDukeMeshInstance::DriveSequences(FLOAT DeltaSeconds)
{

	if (!Actor)
		return;

	UBOOL bSimulatedPawn = (Cast<APawn>(Actor) && (Actor->Role == ROLE_SimulatedProxy));

	for (INT Channel = 0; Channel < 16; Channel++)
	{
		FMeshChannel* Chan = &MeshChannels[Channel];

		// Copy stock animation information to channel zero info
		if (!Channel)
		{
			Chan->bAnimFinished = Actor->bAnimFinished;
			Chan->bAnimLoop = Actor->bAnimLoop;
			Chan->bAnimNotify = Actor->bAnimNotify;
			Chan->bAnimBlendAdditive = Actor->bAnimBlendAdditive;
			Chan->AnimSequence = Actor->AnimSequence;
			Chan->AnimFrame = Actor->AnimFrame;
			Chan->AnimRate = Actor->AnimRate;
			Chan->AnimBlend = Actor->AnimBlend;
			Chan->TweenRate = Actor->TweenRate;
			Chan->AnimLast = Actor->AnimLast;
			Chan->AnimMinRate = Actor->AnimMinRate;
			Chan->OldAnimRate = Actor->OldAnimRate;
			Chan->SimAnim = Actor->SimAnim;
		}

		// Update all animation, including multiple passes if necessary.
		INT Iterations = 0;
		FLOAT Seconds = DeltaSeconds;
		while
			(Actor->IsAnimating(Channel)
				&& (Seconds > 0.0)
				&& (++Iterations <= 4))
		{
			// Remember the old frame.
			FLOAT OldAnimFrame = Chan->AnimFrame;

			// Update animation, and possibly overflow it.
			if (Chan->AnimFrame >= 0.0)
			{
				// Update regular or velocity-scaled animation.
				if (Chan->AnimRate >= 0.0)
					Chan->AnimFrame += Chan->AnimRate * Seconds;
				else
					Chan->AnimFrame += ::Max(Chan->AnimMinRate, Actor->Velocity.Size() * -Chan->AnimRate) * Seconds;

				// Handle all animation sequence notifys.
				if (Chan->bAnimNotify)
				{
					HMeshSequence Seq = FindSequence(Chan->AnimSequence);
					if (Seq)
					{
						FLOAT BestElapsedFrames = 100000.0;
						INT BestNotify = -1;
						for (INT i = 0; i < GetSeqNumEvents(Seq); i++)
						{
							if (GetSeqEventType(Seq, i) != MESHSEQEV_Trigger)
								continue;
							FLOAT EventTime = GetSeqEventTime(Seq, i);
							if (OldAnimFrame < EventTime && Chan->AnimFrame >= EventTime)
							{
								FLOAT ElapsedFrames = EventTime - OldAnimFrame;
								if ((BestNotify == -1) || (ElapsedFrames < BestElapsedFrames))
								{
									BestElapsedFrames = ElapsedFrames;
									BestNotify = i;
								}
							}
						}
						if (BestNotify != -1)
						{
							Seconds = Seconds * (Chan->AnimFrame - GetSeqEventTime(Seq, BestNotify)) / (Chan->AnimFrame - OldAnimFrame);
							Chan->AnimFrame = GetSeqEventTime(Seq, BestNotify);
							UFunction* Function = Actor->FindFunction(FName(GetSeqEventString(Seq, BestNotify)));
							if (Function)
								Actor->ProcessEvent(Function, NULL);
							continue;
						}
					}
				}

				// Handle end of animation sequence.
				if (Chan->AnimFrame < Chan->AnimLast)
				{
					// We have finished the animation updating for this tick.
					break;
				}
				else if (Chan->bAnimLoop)
				{
					if (Chan->AnimFrame < 1.0)
					{
						// Still looping.
						Seconds = 0.0;
					}
					else
					{
						// Just passed end, so loop it.
						Seconds = Seconds * (Chan->AnimFrame - 1.0) / (Chan->AnimFrame - OldAnimFrame);
						Chan->AnimFrame = 0.0;
					}
					if (OldAnimFrame < Chan->AnimLast)
					{
						if ((Actor->GetStateFrame()->LatentAction == EPOLL_FinishAnim)
							&& (Actor->LatentInt == Channel))
						{
							Chan->bAnimFinished = 1;
							if (!Channel)
								Actor->bAnimFinished = 1;
						}
						if (!bSimulatedPawn)
						{
							AActor* Act = Actor; // need to duplicate since this meshinstance may be destroyed if actor dies in animend
							if (!Channel)
							{
								Actor->bAnimFinished = Chan->bAnimFinished;
								Actor->bAnimLoop = Chan->bAnimLoop;
								Actor->bAnimNotify = Chan->bAnimNotify;
								Actor->bAnimBlendAdditive = Chan->bAnimBlendAdditive;
								Actor->AnimSequence = Chan->AnimSequence;
								Actor->AnimFrame = Chan->AnimFrame;
								Actor->AnimRate = Chan->AnimRate;
								Actor->AnimBlend = Chan->AnimBlend;
								Actor->TweenRate = Chan->TweenRate;
								Actor->AnimLast = Chan->AnimLast;
								Actor->AnimMinRate = Chan->AnimMinRate;
								Actor->OldAnimRate = Chan->OldAnimRate;
								Actor->SimAnim = Chan->SimAnim;
								Actor->eventAnimEnd();
							}
							else if (Actor->IsProbing(NAME_AnimEnd))
								Actor->eventAnimEndEx(Channel);

							if (!Act->IsValid() || Act->bDeleteMe)
								return;
						}
					}
				}
				else
				{
					// Just passed end-minus-one frame.
					Seconds = Seconds * (Chan->AnimFrame - Chan->AnimLast) / (Chan->AnimFrame - OldAnimFrame);
					Chan->AnimFrame = Chan->AnimLast;
					Chan->bAnimFinished = 1;
					Chan->AnimRate = 0.0;
					if (!bSimulatedPawn)
					{
						AActor* Act = Actor; // need to duplicate since this meshinstance may be destroyed if actor dies in animend
						if (!Channel)
						{
							Actor->bAnimFinished = Chan->bAnimFinished;
							Actor->bAnimLoop = Chan->bAnimLoop;
							Actor->bAnimNotify = Chan->bAnimNotify;
							Actor->bAnimBlendAdditive = Chan->bAnimBlendAdditive;
							Actor->AnimSequence = Chan->AnimSequence;
							Actor->AnimFrame = Chan->AnimFrame;
							Actor->AnimRate = Chan->AnimRate;
							Actor->AnimBlend = Chan->AnimBlend;
							Actor->TweenRate = Chan->TweenRate;
							Actor->AnimLast = Chan->AnimLast;
							Actor->AnimMinRate = Chan->AnimMinRate;
							Actor->OldAnimRate = Chan->OldAnimRate;
							Actor->SimAnim = Chan->SimAnim;
							Actor->eventAnimEnd();
						}
						else if (Actor->IsProbing(NAME_AnimEnd))
							Actor->eventAnimEndEx(Channel);

						if (!Act->IsValid() || Act->bDeleteMe)
							return;
					}

					if (
						(Actor->bUpdateSimAnim) ||
						((Actor->RemoteRole < ROLE_SimulatedProxy) && !Actor->IsA(AWeapon::StaticClass()))
						)
					{
						Chan->SimAnim.X = 10000 * Chan->AnimFrame;
						Chan->SimAnim.Y = 5000 * Chan->AnimRate;
						if (Chan->SimAnim.Y > 32767)
							Chan->SimAnim.Y = 32767;
					}
				}
			}
			else
			{
				// Update tweening.
				Chan->AnimFrame += Chan->TweenRate * Seconds;
				if (Chan->AnimFrame >= 0.0)
				{
					// Finished tweening.
					Seconds = Seconds * (Chan->AnimFrame - 0) / (Chan->AnimFrame - OldAnimFrame);
					Chan->AnimFrame = 0.0;
					if (Chan->AnimRate == 0.0)
					{
						Chan->bAnimFinished = 1;
						if (!bSimulatedPawn)
						{
							AActor* Act = Actor; // need to duplicate since this meshinstance may be destroyed if actor dies in animend
							if (!Channel)
							{
								Actor->bAnimFinished = Chan->bAnimFinished;
								Actor->bAnimLoop = Chan->bAnimLoop;
								Actor->bAnimNotify = Chan->bAnimNotify;
								Actor->bAnimBlendAdditive = Chan->bAnimBlendAdditive;
								Actor->AnimSequence = Chan->AnimSequence;
								Actor->AnimFrame = Chan->AnimFrame;
								Actor->AnimRate = Chan->AnimRate;
								Actor->AnimBlend = Chan->AnimBlend;
								Actor->TweenRate = Chan->TweenRate;
								Actor->AnimLast = Chan->AnimLast;
								Actor->AnimMinRate = Chan->AnimMinRate;
								Actor->OldAnimRate = Chan->OldAnimRate;
								Actor->SimAnim = Chan->SimAnim;
								Actor->eventAnimEnd();
							}
							else if (Actor->IsProbing(NAME_AnimEnd))
								Actor->eventAnimEndEx(Channel);

							if (!Act->IsValid() || Act->bDeleteMe)
								return;
						}
					}
				}
				else
				{
					// Finished tweening.
					break;
				}
			}
		}

		// Copy channel zero information to stock animation info
		if (!Channel)
		{
			Actor->bAnimFinished = Chan->bAnimFinished;
			Actor->bAnimLoop = Chan->bAnimLoop;
			Actor->bAnimNotify = Chan->bAnimNotify;
			Actor->bAnimBlendAdditive = Chan->bAnimBlendAdditive;
			Actor->AnimSequence = Chan->AnimSequence;
			Actor->AnimFrame = Chan->AnimFrame;
			Actor->AnimRate = Chan->AnimRate;
			Actor->AnimBlend = Chan->AnimBlend;
			Actor->TweenRate = Chan->TweenRate;
			Actor->AnimLast = Chan->AnimLast;
			Actor->AnimMinRate = Chan->AnimMinRate;
			Actor->OldAnimRate = Chan->OldAnimRate;
			Actor->SimAnim = Chan->SimAnim;
		}
	}

}

UTexture* UDukeMeshInstance::GetTexture(INT Count)
{
	return nullptr; // Mac->mSurfaces(Count).m_Texture;
}
void UDukeMeshInstance::GetStringValue(FOutputDevice& Ar, const TCHAR* Key, INT Index)
{
	if (!Mesh)
		return;

	// the only two values which can be ascertained without a valid Mac
	if (!appStricmp(Key, TEXT("IsDukeMesh")))
		Ar.Logf(TEXT("%d"), 1);
	else if (!appStricmp(Key, TEXT("ConfigName")))
		Ar.Logf(TEXT("%s"), *Mesh->ConfigName);

	// Sequences
	else if (!appStricmp(Key, TEXT("NumAnimSeqs")))
		Ar.Logf(TEXT("%d"), GetNumSequences());
	else if (!appStricmp(Key, TEXT("AnimSeqName")))
	{
		if ((Index >= 0) && (Index < GetNumSequences()))
		{
			HMeshSequence Seq = GetSequence(Index);
			if (Seq && GetSeqName(Seq) != NAME_None)
				Ar.Logf(TEXT("%s"), *GetSeqName(Seq));
		}
	}
	else if (!appStricmp(Key, TEXT("AnimSeqRate")))
	{
		if ((Index >= 0) && (Index < GetNumSequences()))
		{
			HMeshSequence Seq = GetSequence(Index);
			if (Seq && GetSeqName(Seq) != NAME_None)
				Ar.Logf(TEXT("%f"), GetSeqRate(Seq));
		}
	}
	else if (!appStricmp(Key, TEXT("AnimSeqFrames")))
	{
		if ((Index >= 0) && (Index < GetNumSequences()))
		{
			HMeshSequence Seq = GetSequence(Index);
			if (Seq && GetSeqName(Seq) != NAME_None)
				Ar.Logf(TEXT("%d"), GetSeqNumFrames(Seq));
		}
	}
}
void UDukeMeshInstance::SendStringCommand(const TCHAR* Cmd)
{
	// jmarshall - todo
		//Mac->Msgf((char*)appToAnsi(Cmd));
	// jmarshall end
}
FCoords UDukeMeshInstance::GetBasisCoords(AActor* actor, FCoords Coords)
{
	if (!actor)
		return(Coords);
	FLOAT DrawScale = actor->bParticles ? 1.f : actor->DrawScale;
	//DrawScale = 1.f;
	//UBOOL NotWeaponHeuristic = ((!Viewport) || (Owner->Owner != Viewport->Actor));	
	FVector HeightAdjust = actor->bMeshLowerByCollision ? FVector(0, 0, -actor->CollisionHeight) : FVector(0, 0, -actor->MeshLowerHeight);
	// Andy, this is a message from Brandon.  When you make this an actor method, make a Pawn version that adjusts
	// the height by 5 units.
	if (actor->IsA(APawn::StaticClass()))
		HeightAdjust.Z -= 5;
	Coords = Coords * (actor->Location + actor->PrePivot) * actor->Rotation * mDukeRotOrigin
		* HeightAdjust * FScale(mDukeScale * DrawScale, 0.f, SHEER_None);
	return(Coords);
}

static UBOOL mesh_GetFrameNoTransform = 0;

#define TWEEN_FIX			// JEP

INT UDukeMeshInstance::GetFrame(FVector* ResultVerts, BYTE* VertsEnabled, INT Size, FCoords Coords, FLOAT LodLevel)
{

	if (!Actor)
		return(0);

	STAT(clock(GStat.GetFrameCycles));

	return 0;
}


UBOOL UDukeMeshInstance::GetMountCoords(FName MountName, INT MountType, FCoords& OutCoords, AActor* ChildActor)
{

	if (!Actor)
		return(0);

	STAT(clock(GStat.GetMountCoordsCycles));


	return(0);

}

void UDukeMeshInstance::Draw(/* FSceneNode* */void* InFrame, /* FDynamicSprite* */void* InSprite,
	FCoords InCoords, DWORD InPolyFlags)
{
}

UBOOL UDukeMeshInstance::LineCheck(FCheckResult& Result, AActor* Owner, FVector End, FVector Start, FVector Extent, DWORD ExtraNodeFlags, UBOOL bMeshAccurate)
{
	if ((Extent != FVector(0, 0, 0))
		|| !Actor || (Actor != Owner))
	{
		// Use cylinder.
		return UPrimitive::LineCheck(Result, Owner, End, Start, Extent, ExtraNodeFlags, bMeshAccurate);
	}
	else
	{
		return UPrimitive::LineCheck(Result, Owner, End, Start, FVector(0, 0, 0), ExtraNodeFlags, bMeshAccurate);

		// jmarshall - we don't need this.
#if 0
		Result.MeshBoneName = NAME_None;
		Result.MeshTri = -1;
		Result.MeshBarys = FVector(0.33, 0.33, 0.34);
		Result.MeshTexture = NULL;

		UBOOL PrimResult = UPrimitive::LineCheck(Result, Owner, End, Start, FVector(0, 0, 0), ExtraNodeFlags, bMeshAccurate);
		if (PrimResult)
			return(PrimResult); // cylinder failed, no need to go further

		// get vertices and triangles
		FMemMark Mark(GMem);
		SMacTri* TempTris = New<SMacTri>(GMem, Mac->GetNumTris());
		VVec3* TempVerts = New<VVec3>(GMem, Mac->GetNumVertexes());
		INT NumTris = Mac->EvaluateTris(1.f, TempTris);
		mesh_GetFrameNoTransform = 1;
		INT NumVerts = GetFrame((FVector*)TempVerts, NULL, sizeof(VVec3), GMath.UnitCoords, 1.f);
		mesh_GetFrameNoTransform = 0;

		// transform the start and end into mesh space
		FCoords MTWCoords = GetBasisCoords(GMath.UnitCoords) / FScale(Mac->mDukeScale * Actor->DrawScale, 0.f, SHEER_None);
		MTWCoords /= FScale(Mac->mDukeScale * Actor->DrawScale, 0.f, SHEER_None);
		FCoords WTMCoords = MTWCoords.Transpose();
		FVector EndTrans = (End.TransformPointBy(WTMCoords) + Mac->mDukeOrigin).ToStd();
		FVector StartTrans = (Start.TransformPointBy(WTMCoords) + Mac->mDukeOrigin).ToStd();
		VVec3 RayStart = *((VVec3*)&StartTrans);
		VVec3 RayEnd = *((VVec3*)&EndTrans);

		// get a line to trace
		VLine3 Ray; Ray.TwoPoint(RayStart, RayEnd);
		FLOAT RayLen = RayStart & RayEnd;

		// trace it
		DWORD HitTri;
		FLOAT HitDist;
		VVec3 HitBarys;
		CCpjSklBone* HitBone;
		UBOOL TraceRes = Mac->TraceRay(NumTris, TempTris, NumVerts, TempVerts, Ray, &HitTri, &HitDist, &HitBarys, &HitBone);
		if (TraceRes)
		{
			Result.Time = HitDist / RayLen;
			Result.Location = Start + (End - Start) * Result.Time;
			Result.Actor = Actor;
			Result.Primitive = NULL;

			FVector TriV[3];
			for (INT j = 0; j < 3; j++)
			{
				FVector* V = (FVector*)&TempVerts[TempTris[HitTri].vertIndex[j]];
				TriV[j] = (V->ToUnr() - Mac->mDukeOrigin).TransformPointBy(MTWCoords);
			}
			Result.Normal = FVector((TriV[1] - TriV[0]) ^ (TriV[2] - TriV[0]));
			Result.Normal *= (1.f / appSqrt(Result.Normal.SizeSquared() + 0.001f));

			Result.MeshBoneName = HitBone ? FName(appFromAnsi(*HitBone->name)) : NAME_None;
			Result.MeshTri = HitTri;
			Result.MeshBarys = *((FVector*)&HitBarys);
			Result.MeshTexture = TempTris[HitTri].texture ? (UTexture*)(TempTris[HitTri].texture->imagePtr) : NULL;

			VVec2 UV = *TempTris[HitTri].texUV[0] * Result.MeshBarys.X +
				*TempTris[HitTri].texUV[1] * Result.MeshBarys.Y +
				*TempTris[HitTri].texUV[2] * Result.MeshBarys.Z;
			Result.PointUV.X = UV.x;
			Result.PointUV.Y = UV.y;
		}

		Mark.Pop();
		return(TraceRes == 0);
#endif
		// jmarshall end
	}
}

FBox UDukeMeshInstance::GetRenderBoundingBox(const AActor* Owner, UBOOL Exact)
{
	// replace for accurate bounding box with respect to current bone bounding boxes
	UMesh* Mesh = GetMesh();
	if (!Mesh)
		return(FBox());
	return(Mesh->GetRenderBoundingBox(Owner, Exact));
}

/*
	UMeshInstance
	Bone intrinsics specific to DukeMeshes
*/
void UMeshInstance::execMeshToWorldLocation(FFrame& Stack, RESULT_DECL)
{

	P_GET_VECTOR(InLocation);
	P_GET_UBOOL_OPTX(bFromStd, 0);
	P_FINISH;

	UDukeMeshInstance* dmi = Cast<UDukeMeshInstance>(this);
	if (!dmi || !dmi->Actor)
	{
		*(FVector*)Result = InLocation;
		return;
	}
	if (bFromStd)
		InLocation = InLocation.ToUnr();
	*(FVector*)Result = (InLocation - dmi->mDukeOrigin).TransformPointBy(GetBasisCoords(GMath.UnitCoords));

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execMeshToWorldLocation);

void UMeshInstance::execWorldToMeshLocation(FFrame& Stack, RESULT_DECL)
{

	P_GET_VECTOR(InLocation);
	P_GET_UBOOL_OPTX(bToStd, 0);
	P_FINISH;

	UDukeMeshInstance* dmi = Cast<UDukeMeshInstance>(this);
	if (!dmi || !dmi->Actor)
	{
		*(FVector*)Result = InLocation;
		return;
	}
	*(FVector*)Result = InLocation.TransformPointBy((GetBasisCoords(GMath.UnitCoords)).Transpose()) + dmi->mDukeOrigin;
	if (bToStd)
		*(FVector*)Result = (*(FVector*)Result).ToStd();

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execWorldToMeshLocation);

void UMeshInstance::execMeshToWorldRotation(FFrame& Stack, RESULT_DECL)
{

	P_GET_ROTATOR(InRotation);
	P_FINISH;

	UDukeMeshInstance* dmi = Cast<UDukeMeshInstance>(this);
	if (!dmi || !dmi->Actor)
	{
		*(FRotator*)Result = InRotation;
		return;
	}
	FCoords MeshCoords = GetBasisCoords(GMath.UnitCoords);
	MeshCoords = MeshCoords / InRotation;
	MeshCoords = MeshCoords.Transpose();
	*(FRotator*)Result = MeshCoords.OrthoRotation();

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execMeshToWorldRotation);

void UMeshInstance::execWorldToMeshRotation(FFrame& Stack, RESULT_DECL)
{

	P_GET_ROTATOR(InRotation);
	P_FINISH;

	UDukeMeshInstance* dmi = Cast<UDukeMeshInstance>(this);
	if (!dmi || !dmi->Actor)
	{
		*(FRotator*)Result = InRotation;
		return;
	}
	FCoords MeshCoords = GetBasisCoords(GMath.UnitCoords);
	MeshCoords = MeshCoords / InRotation;
	*(FRotator*)Result = MeshCoords.OrthoRotation();

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execWorldToMeshRotation);

void UMeshInstance::execBoneFindNamed(FFrame& Stack, RESULT_DECL)
{

	P_GET_NAME(BoneName);
	P_FINISH;

	UDukeMeshInstance* dmi = Cast<UDukeMeshInstance>(this);
	if (!dmi)
	{
		*(INT*)Result = 0;
		return;
	}

	*(INT*)Result = (INT)dmi->FindBone(BoneName);

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneFindNamed);

void UMeshInstance::execBoneGetName(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_FINISH;

	FDukeSkelBone* bone = (FDukeSkelBone*)BoneHandle;
	if (!bone)
	{
		*(FName*)Result = NAME_None;
		return;
	}
	*(FName*)Result = FName(*bone->name);

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetName);

void UMeshInstance::execBoneGetParent(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_FINISH;

	FDukeSkelBone* bone = (FDukeSkelBone*)BoneHandle;
	if (!bone)
	{
		*(INT*)Result = 0;
		return;
	}
	*(INT*)Result = (INT)(bone->parent);

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetParent);

void UMeshInstance::execBoneGetChildCount(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_FINISH;

// jmarshall - todo
	//CMacBone* bone = (CMacBone*)BoneHandle;
	//if (!bone)
	//{
	//	*(INT*)Result = 0;
	//	return;
	//}
	//INT count = 0;
	//for (CMacBone* b = bone->mFirstChild; b; b = b->mNextSibling)
	//	count++;
	//*(INT*)Result = count;

	* (INT*)Result = 0;
// jmarshall end
}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetChildCount);

void UMeshInstance::execBoneGetChild(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_GET_INT(ChildIndex);
	P_FINISH;
// jmarshall - todo
	//CMacBone* bone = (CMacBone*)BoneHandle;
	//if (!bone)
	//{
	//	*(INT*)Result = 0;
	//	return;
	//}
	//INT count = 0;
	//for (CMacBone* b = bone->mFirstChild; b && (count < ChildIndex); b = b->mNextSibling)
	//	count++;
	//*(INT*)Result = (INT)b;
	* (INT*)Result = 0;
// jmarshall end
}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetChild);

void UMeshInstance::execBoneGetTranslate(FFrame& Stack, RESULT_DECL)
{
	P_GET_INT(BoneHandle);
	P_GET_UBOOL_OPTX(bAbsolute, 0);
	P_GET_UBOOL_OPTX(bDefault, 0);
	P_GET_FLOAT_OPTX(fScale, 1.f);
	P_FINISH;

// jmarshall - todo
	//CMacBone* bone = (CMacBone*)BoneHandle;
	//if (!bone)
	//{
	//	*(FVector*)Result = FVector(0, 0, 0);
	//	return;
	//}
	//VCoords3 c;
	//if (!bDefault)
	//{
	//	c = bone->GetCoords(bAbsolute != 0);
	//}
	//else
	//{
	//	c = bone->mSklBone->baseCoords;
	//	if (bAbsolute)
	//	{
	//		for (CMacBone* b = bone->mParent; b; b = b->mParent)
	//			c <<= b->mSklBone->baseCoords;
	//	}
	//}
	//c *= fScale;
	//FVector v(*(FVector*)&c.t);
	//*(FVector*)Result = v.ToUnr();
	* (FVector*)Result = FVector(0, 0, 0);
// jmarshall end
}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetTranslate);

void UMeshInstance::execBoneGetRotate(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_GET_UBOOL_OPTX(bAbsolute, 0);
	P_GET_UBOOL_OPTX(bDefault, 0);
	P_FINISH;
// jmarshall - todo
	//CMacBone* bone = (CMacBone*)BoneHandle;
	//if (!bone)
	//{
	//	*(FRotator*)Result = FRotator(0, 0, 0);
	//	return;
	//}
	//VCoords3 c;
	//if (!bDefault)
	//{
	//	c = bone->GetCoords(bAbsolute != 0);
	//}
	//else
	//{
	//	c = bone->mSklBone->baseCoords;
	//	if (bAbsolute)
	//	{
	//		for (CMacBone* b = bone->mParent; b; b = b->mParent)
	//			c <<= b->mSklBone->baseCoords;
	//	}
	//}
	//FCoords fc(FVector(0, 0, 0), *((FVector*)&c.r.vX), *((FVector*)&c.r.vY), *((FVector*)&c.r.vZ));
	//fc = fc.ToUnr();
	//*(FRotator*)Result = fc.OrthoRotation();
	* (FRotator*)Result = FRotator(0, 0, 0);
// jmarshall end
}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetRotate);
void UMeshInstance::execBoneGetScale(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_GET_UBOOL_OPTX(bAbsolute, 0);
	P_GET_UBOOL_OPTX(bDefault, 0);
	P_FINISH;

	FDukeSkelBone* bone = (FDukeSkelBone*)BoneHandle;
	if (!bone)
	{
		*(FVector*)Result = FVector(1, 1, 1);
		return;
	}

	*(FVector*)Result = bone->GetScale(bDefault, bAbsolute);
}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneGetScale);

void UMeshInstance::execBoneSetTranslate(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_GET_VECTOR(Translate);
	P_GET_UBOOL_OPTX(bAbsolute, 0);
	P_FINISH;

	FDukeSkelBone* bone = (FDukeSkelBone*)BoneHandle;
	if (!bone)
	{
		*(DWORD*)Result = 0;
		return;
	}

	bone->SetTranslation(Translate, bAbsolute);

	*(DWORD*)Result = 1;

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneSetTranslate);

void UMeshInstance::execBoneSetRotate(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_GET_ROTATOR(Rotate);
	P_GET_UBOOL_OPTX(bAbsolute, 0);
	P_GET_UBOOL_OPTX(bRelCurrent, 0);
	P_FINISH;

	FDukeSkelBone* bone = (FDukeSkelBone*)BoneHandle;
	if (!bone)
	{
		*(DWORD*)Result = 0;
		return;
	}

	bone->SetRotation(Rotate, bAbsolute, bRelCurrent);

	*(DWORD*)Result = 1;

}
IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneSetRotate);

void UMeshInstance::execBoneSetScale(FFrame& Stack, RESULT_DECL)
{

	P_GET_INT(BoneHandle);
	P_GET_VECTOR(Scale);
	P_GET_UBOOL_OPTX(bAbsolute, 0);
	P_FINISH;

	FDukeSkelBone* bone = (FDukeSkelBone*)BoneHandle;
	if (!bone)
	{
		*(DWORD*)Result = 0;
		return;
	}
	
	bone->SetScale(Scale, bAbsolute);

	*(DWORD*)Result = 1;

}

IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execBoneSetScale);

void UMeshInstance::execGetBounds(FFrame& Stack, RESULT_DECL)
{
	P_GET_VECTOR_REF(Min);
	P_GET_VECTOR_REF(Max);
	P_FINISH;

	UDukeMeshInstance* dmi = Cast<UDukeMeshInstance>(this);
	if (!dmi)
	{
		return;
	}

	*Min = dmi->mDukeBounds[0];
	*Max = dmi->mDukeBounds[1];
}

IMPLEMENT_FUNCTION(UMeshInstance, INDEX_NONE, execGetBounds);