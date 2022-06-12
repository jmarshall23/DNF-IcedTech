#ifndef __DNMESHPRIVATE_H__
#define __DNMESHPRIVATE_H__
//****************************************************************************
//**
//**    DNMESHPRIVATE.H
//**    Header - DNF Mesh Objects - Private
//**
//****************************************************************************

// jmarshall
#include "DnMeshMD3.h"
// jmarshall end

//============================================================================
//    DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
//============================================================================
//    CLASSES / STRUCTURES
//============================================================================
class UDukeMeshInstance;

// jmarshall: New Cannibal Runtime Wrapper

#include "oldcannibal/VecMain.h"

//
// CCpjSklBone -- md5mesh bone.
//
class CCpjSklBone
{
public:
	FString name;
	int nameHash;
	CCpjSklBone* parentBone;
	VCoords3 baseCoords;
	float length;

	CCpjSklBone() { parentBone = NULL; nameHash = 0; length = 1.f; }
};

struct SMacTri
{
	int vertIndex[3];
};

/*
	CMacBone
	Actor bone state
*/
class CMacBone
{
protected:
	VCoords3 mRelCoords; // parent-relative bone coords
	VCoords3 mAbsCoords; // absolute worldspace bone coords
	bool mAbsValid; // whether absolute coords are currently valid

	void ValidateAbs(bool inMakeValid) // validate or invalidate the absolute coords
	{
		if (inMakeValid)
		{
			// validate a bone's absolute state, including its ancestors
			if (mAbsValid)
				return;
			if (mParent)
				mParent->ValidateAbs(true);
			mAbsCoords = mRelCoords;
			if (mParent)
				mAbsCoords <<= mParent->mAbsCoords;
			mAbsValid = 1;
		}
		else
		{
			// invalidate a bone's absolute state, including its children
			if (!mAbsValid)
				return;
			mAbsValid = 0;
			for (CMacBone* b = mFirstChild; b; b = b->mNextSibling)
				b->ValidateAbs(false);
		}
	}

public:
	CCpjSklBone* mSklBone; // bound skeletal bone	
	CMacBone* mParent; // parent actor bone
	CMacBone* mFirstChild; // first child actor bone
	CMacBone* mNextSibling; // next sibling actor bone

	CMacBone() { mSklBone = NULL; mParent = mFirstChild = mNextSibling = NULL; mAbsValid = 0; }

	VCoords3 GetCoords(bool inAbsolute)
	{
		if (inAbsolute)
		{
			ValidateAbs(true);
			return(mAbsCoords);
		}
		return(mRelCoords);
	}
	void SetCoords(const VCoords3& inCoords, bool inAbsolute)
	{
		if (inAbsolute)
		{
			VCoords3 elderCoords;
			// Get relative by backward transforming up the parent tree
			for (CMacBone* b = mParent; b; b = b->mParent)
				elderCoords <<= b->mRelCoords;
			mRelCoords = inCoords >> elderCoords;
			// Set absolute since we have it
			mAbsCoords = inCoords;
			// Only invalidate absolute from the children down
			if (mFirstChild)
				mFirstChild->ValidateAbs(false);
		}
		else
		{
			// Set relative since we have it
			mRelCoords = inCoords;
			// Invalidate absolute from here down
			ValidateAbs(false);
		}
	}
	void ResetCoords()
	{
		SetCoords(mSklBone->baseCoords, false);
	}
};

class CCpjSeqEvent
{
public:
	int eventType;
	float time;
	FString paramString;

	CCpjSeqEvent() { eventType = 0; time = 0.0; paramString = NULL; }
};


class OCpjSequence
{
public:
	FName name;

	float m_Rate;
	TArray<CCpjSklBone> m_Frames;
	TArray<CCpjSeqEvent> m_Events;

	OCpjSequence()
	{
		m_Rate = 10.0f;
	}
};

class OCpjSurface
{
public:
	UTexture* m_Texture;
	TArray<SMacTri> m_Tris;
};


class CCpjSklWeight
{
public:
	CCpjSklBone* bone;
    float factor;
	VVec3 offsetPos;

	CCpjSklWeight() { bone = NULL; factor = 0.0; offsetPos = VVec3(0,0,0); }
};

class CCpjSklVert
{
public:
	VVec2 uv;
	int startWeight;
	int numWeight;
};

class CCpjSklMount
{
public:
	FString name;
	CCpjSklBone* bone;
	VCoords3 baseCoords;

	CCpjSklMount() { bone = NULL; }
};

class OCpjSkeleton
{
	TArray<CCpjSklBone> m_Bones;
	TArray<CCpjSklWeight> m_Weights;
	TArray<CCpjSklVert> m_Verts;
	TArray<CCpjSklMount> m_Mounts;
};

class ODukeMacActor {
public:
	UDukeMeshInstance* mOwnerInstance;
	FVector mDukeOrigin;
	FRotator mDukeRotOrigin;
	FVector mDukeScale;
	FVector mDukeBounds[2];
	bool bBonesDirty;

	OCpjSkeleton* mSkeleton;
	TArray<OCpjSurface> mSurfaces;
	TArray<OCpjSequence> mSequences;

	ODukeMacActor()
	{
		bBonesDirty = false;
		mOwnerInstance = NULL;
		mDukeOrigin = FVector(0,0,0);
		mDukeRotOrigin = FRotator(0,0,0);
		mDukeScale = FVector(1,1,1);
		mDukeBounds[0] = FVector(-1,-1,-1);
		mDukeBounds[1] = FVector(1,1,1);
	}

	// OMacActor
	CMacBone* FindBone(const char* inName);
	OCpjSequence* FindSequence(const char* inName);
	void Destroy();

	int EvaluateTris(float inLodLevel, SMacTri* outTriList);
	int EvaluateVerts(float inLodLevel, float inVertAlpha, FVector* outVerts);
	bool EvaluateTriVerts(int inTriIndex, float inVertAlpha, FVector* outVerts);

	int GetNumVertexes();
	int GetNumTris();

	
private:

};
// jmarshall end

class ENGINE_API UDukeMeshInstance : public UMeshInstance
{
	DECLARE_CLASS(UDukeMeshInstance,UMeshInstance,CLASS_Transient)

	AActor* Actor;
	UDukeMesh* Mesh;
	ODukeMacActor* Mac;

	// UObject
	UDukeMeshInstance();
	void Destroy();

	// UPrimitive
	UBOOL LineCheck(FCheckResult& Result, AActor* Owner, FVector End, FVector Start, FVector Extent, DWORD ExtraNodeFlags, UBOOL bMeshAccurate=0);
	FBox GetRenderBoundingBox(const AActor* Owner, UBOOL Exact);

	// UMeshInstance
	UMesh* GetMesh();
	void SetMesh(UMesh* InMesh);

	AActor* GetActor();
	void SetActor(AActor* InActor);

	INT GetNumSequences();
	HMeshSequence GetSequence(INT SeqIndex);
	HMeshSequence FindSequence(FName SeqName);
	
	FName GetSeqName(HMeshSequence Seq);
	void SetSeqGroupName(FName SequenceName, FName GroupName);
	FName GetSeqGroupName(FName SequenceName);
	INT GetSeqNumFrames(HMeshSequence Seq);
	FLOAT GetSeqRate(HMeshSequence Seq);
	INT GetSeqNumEvents(HMeshSequence Seq);
	EMeshSeqEvent GetSeqEventType(HMeshSequence Seq, INT Index);
	FLOAT GetSeqEventTime(HMeshSequence Seq, INT Index);
	const TCHAR* GetSeqEventString(HMeshSequence Seq, INT Index);

	UBOOL PlaySequence(HMeshSequence Seq, BYTE Channel, UBOOL bLoop, FLOAT Rate, FLOAT MinRate, FLOAT TweenTime);
	void DriveSequences(FLOAT DeltaSeconds);

	UTexture* GetTexture(INT Count);
	void GetStringValue(FOutputDevice& Ar, const TCHAR* Key, INT Index);
	void SendStringCommand(const TCHAR* Cmd);
	FCoords GetBasisCoords(AActor *actor,FCoords Coords);
	inline FCoords GetBasisCoords(FCoords Coords){return GetBasisCoords(Actor,Coords);}
	INT GetFrame(FVector* Verts, BYTE* VertsEnabled, INT Size, FCoords Coords, FLOAT LodLevel);
	UBOOL GetMountCoords(FName MountName, INT MountType, FCoords& OutCoords, AActor* ChildActor);
    UBOOL GetBoneCoords(CMacBone *bone, FCoords& OutCoords);

	void Draw(/* FSceneNode* */void* InFrame, /* FDynamicSprite* */void* InSprite,
		FCoords InCoords, DWORD InPolyFlags);

	// UDukeMeshInstance
	void DestroyMacActor();
};

//============================================================================
//    GLOBAL DATA
//============================================================================
//============================================================================
//    GLOBAL FUNCTIONS
//============================================================================
//============================================================================
//    INLINE CLASS METHODS
//============================================================================
//============================================================================
//    TRAILING HEADERS
//============================================================================

//****************************************************************************
//**
//**    END HEADER DNMESHPRIVATE.H
//**
//****************************************************************************
#endif // __DNMESHPRIVATE_H__
