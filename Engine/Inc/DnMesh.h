#ifndef __DNMESH_H__
#define __DNMESH_H__
//****************************************************************************
//**
//**    DNMESH.H
//**    Header - DNF Mesh Objects
//**
//****************************************************************************
//============================================================================
//    HEADERS
//============================================================================
//============================================================================
//    DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
//============================================================================
//    CLASSES / STRUCTURES
//============================================================================

// jmarshall
#include "DnMeshMD3.h"
// jmarshall end

struct FDukeSkelBone
{
	FName name;
	FDukeSkelBone* parent;
	int parentNum;
	FVector xyz;
	FQuat rotation;

	FVector GetScale(bool bDefault, bool bAbsolute)
	{
		return FVector(1, 1, 1);
	}

	void SetTranslation(FVector translate, bool bAbsolute)
	{

	}

	void SetRotation(FRotator rotate, bool bAbsolute, bool bRelCurrent)
	{

	}

	void SetScale(FVector scale, bool bAbsolute)
	{

	}
};

struct FDukeSkelVert
{
	float u;
	float v;
	int startWeight;
	int numWeights;
};

struct FDukeSkelTri
{
	int tris[3];
};

struct FDukeSkelWeight
{
	FDukeSkelBone* joint;
	float weight;
	FVector offset;
};

struct FDukeSkelMesh
{
	UTexture* texture;
	TArray<FDukeSkelVert>   verts;
	TArray<FDukeSkelTri>    tris;
	TArray<FDukeSkelWeight> weights;
};

struct FDukeSkelEvent
{
	int frame;
	EMeshSeqEvent eventType;
	FString paramString;
};

struct FDukeSkelFrame
{

};

//
// FDukeSkelSequence
// These are the md5anims and .event files. 
//
struct FDukeSkelSequence
{
	FName name;
	TArray<FDukeSkelEvent> events;
	TArray<FDukeSkelFrame> frames;
};

/*
	UDukeMesh
*/
class ENGINE_API UDukeMesh : public UMesh
{
	DECLARE_CLASS(UDukeMesh,UMesh,0)

	FString ConfigName; // file name of project configuration
	TMap<FName, FName> SequenceGroupMap;

	// UObject
	UDukeMesh();
	void Serialize(FArchive& Ar);

	// UPrimitive
	FBox GetRenderBoundingBox(const AActor* Owner, UBOOL Exact);
	UBOOL LineCheck
	(
		FCheckResult&	Result,
		AActor*			Owner,
		FVector			End,
		FVector			Start,
		FVector			Size,
		DWORD           ExtraNodeFlags,
		UBOOL			bMeshAccurate=0
	);

	// UMesh
	UClass* GetInstanceClass();

	int NumSequences() { return sequences.Num(); }
	FDukeSkelSequence* GetSequence(int index) { return &sequences(index); }

	FDukeSkelSequence* FindSequence(FName& name)
	{
		for (int i = 0; i < sequences.Num(); i++)
		{
			if (sequences(i).name == name)
			{
				return &sequences(i);
			}
		}

		return nullptr;
	}

private:
	FString meshFileName;

	void LoadMD3(byte* buffer, int bufferLen);
	TArray<md3ImportSurface_t> md3Surfaces;

	void LoadMD5(const wchar_t *fileName);
	TArray<FDukeSkelBone> bones;
	TArray<FDukeSkelMesh> meshes;

	TArray< FDukeSkelSequence> sequences;
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
//**    END HEADER DNMESH.H
//**
//****************************************************************************
#endif // __DNMESH_H__
