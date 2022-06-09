//****************************************************************************
//**
//**    CPJLOD.CPP
//**    Cannibal Models - LOD Descriptions
//**
//****************************************************************************
//============================================================================
//    HEADERS
//============================================================================
#include "Kernel.h"
#include "CpjMain.h"
#include "MacMain.h"
#include "CpjLod.h"
#include <time.h>

#define CPJVECTOR VVec3
#define CPJQUAT VQuat3
#pragma pack(push,1)
#include "CpjFmt.h"
#pragma pack(pop)


//============================================================================
//    DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================
//============================================================================
//    CLASSES / STRUCTURES
//============================================================================
class XLodException
{
private:
	CCorString mText;
public:
	XLodException(NChar* inFmt, ... ) { mText = STR_Va(inFmt); }
	const NChar* GetText() { return(*mText); }
};

class CLodGenMrgInfo
{
public:
	

	CLodGenMrgInfo()
	{

	}
	void BuildModel()
	{

	}
};

class CLodGenVert
{
public:
	struct STexVert
	{
		CCpjSrfTex* mTex;
		VVec2 mPos;
		NDword mUVIndex;
	};
	VVec3 mPos;
	TCorArray<STexVert> mTV;
};

class CLodGenVertInfo
{
public:
	NDword mOrigCount; // original vertex count
	NDword mMrgCount; // mrg vertex count, original plus duplicates
	TCorArray<CLodGenVert> mGenVerts; // original vertex positions
	TCorArray<NDword> mMrgToOrig; // original index for each mrg vertex
	TCorArray<NDword> mOrigToMrg; // first mrg index for each original vertex

	CLodGenVertInfo() { mOrigCount = mMrgCount = 0; }
};

class CLodGenTriInfo
{
public:
	NDword mCount;
	TCorArray<SMacTri> mMacTris;
	TCorArray<NDword> mOrigToMrg;
	TCorArray<NDword> mMrgToOrig;
};

class CLodGenerator
{
public:
	CLodGenMrgInfo mMrgInfo;
	
	OMacActor* mActor;
	
	NFloat mLodLevel;
	NBool mUseTexVerts;

	VVec3 mBBMin, mBBMax;
	VVec3 mBBox[8];
	NDword mMinActiveFaces;

	CLodGenTriInfo mTris;
	CLodGenVertInfo mVerts;

	TCorArray<VVec3> mMinGeom;

	CLodGenerator()
	{
		mActor = NULL;
	}
	~CLodGenerator()
	{
		if (mActor)
			mActor->Destroy();
	}
	NBool Init(OCpjLodData* inLod, OCpjGeometry* inGeo, OCpjSurface* inSrf, NFloat inLodLevel, NBool inUseTexVerts)
	{
		if (!inGeo || !inSrf)
			return(0);
		if (inGeo->m_Tris.GetCount() != inSrf->m_Tris.GetCount())
			return(0);
		
		mActor = OMacActor::New(NULL);
		mActor->SetGeometry(inGeo);
		mActor->SetSurface(0, inSrf);
		if (inLod)
			mActor->SetLodData(inLod);

		if (inLodLevel < 0.f) inLodLevel = 0.f;
		if (inLodLevel > 1.f) inLodLevel = 1.f;
		mLodLevel = inLodLevel;
		mUseTexVerts = inUseTexVerts;

		mMinActiveFaces = 0;

		return(1);
	}

	void PreGenerateVerts()
	{
		
	}

	void PreGenerateMinGeom()
	{
		
	}

	void Process()
	{

	}

	void PostGenerateVertRemaps()
	{
		
	}

	void PostGenerateFaceRemaps()
	{
		
	}

	void CreateLodData(OCpjLodData* inLod)
	{
		
	}

	NBool Generate(OCpjLodData* inLod, CCorString& outError)
	{
		return FALSE;
	}
};

//============================================================================
//    PRIVATE DATA
//============================================================================
//============================================================================
//    GLOBAL DATA
//============================================================================
//============================================================================
//    PRIVATE FUNCTIONS
//============================================================================
//============================================================================
//    GLOBAL FUNCTIONS
//============================================================================
//============================================================================
//    CLASS METHODS
//============================================================================
/*
	OCpjLodData
*/
OBJ_CLASS_IMPLEMENTATION(OCpjLodData, OCpjChunk, 0);

NBool OCpjLodData::Generate(OCpjGeometry* inGeo, OCpjSurface* inSrf)
{
	// flush the old data
	LoadChunk(NULL, 0);

	// run the generator
	CLodGenerator g;
	if (!g.Init(NULL, inGeo, inSrf, 1.01f, /*true*/false))
	{
		LOG_Logf("LOD Initialization failure");
		return(0);
	}

	CCorString genError;
	if (!g.Generate(this, genError))
	{
		LOG_Logf("LOD Generate: %s", *genError);
		return(0);
	}
/*
	LOG_Logf("NumTris: %d", g.mTris.mCount);
	LOG_Logf("NumOrigVerts: %d", g.mVerts.mOrigCount);
	LOG_Logf("NumMrgVerts: %d", g.mVerts.mMrgCount);

	LOG_Logf("Tri Mapping:");
	for (NDword i=0;i<g.mTris.mCount;i++)
		LOG_Logf("  %d -> %d -> %d", i, g.mTris.mOrigToMrg[i], g.mTris.mMrgToOrig[g.mTris.mOrigToMrg[i]]);
	LOG_Logf("Vert Mapping Orig To Mrg:");
	for (i=0;i<g.mVerts.mOrigCount;i++)
		LOG_Logf("  %d -> %d -> %d", i, g.mVerts.mOrigToMrg[i], g.mVerts.mMrgToOrig[g.mVerts.mOrigToMrg[i]]);
	LOG_Logf("Vert Mapping Mrg To Orig:");
	for (i=0;i<g.mVerts.mMrgCount;i++)
		LOG_Logf("  %d -> %d -> %d", i, g.mVerts.mMrgToOrig[i], g.mVerts.mOrigToMrg[g.mVerts.mMrgToOrig[i]]);
*/
	return(1);
}

NDword OCpjLodData::GetFourCC()
{
	return(KRN_FOURCC(CPJ_LOD_MAGIC));
}
NBool OCpjLodData::LoadChunk(void* inImagePtr, NDword inImageLen)
{
    NDword i, j;

	if (!inImagePtr)
	{
		// remove old array data
		m_Levels.Purge(); m_Levels.Shrink();
		return(1);
	}

	// verify header
	SLodFile* file = (SLodFile*)inImagePtr;
	if (file->header.magic != KRN_FOURCC(CPJ_LOD_MAGIC))
		return(0);
	if (file->header.version < 3)
	{
		// old LOD format, ignore contents
		m_Levels.Purge(); m_Levels.Shrink();
		if (file->header.ofsName)
			SetName((char*)inImagePtr + file->header.ofsName);
		return(1);
	}
	if (file->header.version != CPJ_LOD_VERSION)
		return(0);

	// set up image data pointers
	SLodLevel* fileLevels = (SLodLevel*)(&file->dataBlock[file->ofsLevels]);
	NWord* fileVertRelay = (NWord*)(&file->dataBlock[file->ofsVertRelay]);
	SLodTri* fileTriangles = (SLodTri*)(&file->dataBlock[file->ofsTriangles]);

	// remove old array data
	m_Levels.Purge(); m_Levels.Shrink(); m_Levels.Add(file->numLevels);

	if (file->header.ofsName)
		SetName((char*)inImagePtr + file->header.ofsName);

	// levels
	for (i=0;i<file->numLevels;i++)
	{
		SLodLevel* iL = &fileLevels[i];
		CCpjLodLevel* oL = &m_Levels[i];
		
		oL->detail = iL->detail;
		oL->vertRelay.Add(iL->numVertRelay);
		for (j=0;j<iL->numVertRelay;j++)
			oL->vertRelay[j] = fileVertRelay[iL->firstVertRelay+j];
		oL->triangles.Add(iL->numTriangles);
		for (j=0;j<iL->numTriangles;j++)
		{
			SLodTri* iT = &fileTriangles[iL->firstTriangle+j];
			CCpjLodTri* oT = &oL->triangles[j];
			oT->srfTriIndex = iT->srfTriIndex;
			oT->vertIndex[0] = iT->vertIndex[0];
			oT->vertIndex[1] = iT->vertIndex[1];
			oT->vertIndex[2] = iT->vertIndex[2];
			oT->uvIndex[0] = iT->uvIndex[0];
			oT->uvIndex[1] = iT->uvIndex[1];
			oT->uvIndex[2] = iT->uvIndex[2];
		}
	}

	return(1);
}

NBool OCpjLodData::SaveChunk(void* inImagePtr, NDword* outImageLen)
{
    NDword i, j;
	SLodFile header;
    NDword imageLen;

	// build header and calculate memory required for image
	imageLen = 0;
	header.header.ofsName = imageLen + offsetof(SLodFile, dataBlock);
	imageLen += strlen(GetName())+1;
	header.numLevels = m_Levels.GetCount();
	header.ofsLevels = imageLen;
	imageLen += header.numLevels*sizeof(SLodLevel);
	header.numTriangles = 0;
	header.ofsTriangles = imageLen;
	for (i=0;i<header.numLevels;i++)
		header.numTriangles += m_Levels[i].triangles.GetCount();
	imageLen += header.numTriangles*sizeof(SLodTri);
	header.numVertRelay = 0;
	header.ofsVertRelay = imageLen;
	for (i=0;i<header.numLevels;i++)
		header.numVertRelay += m_Levels[i].vertRelay.GetCount();
	imageLen += header.numVertRelay*sizeof(NWord);
	imageLen += offsetof(SLodFile, dataBlock);

	// return if length is all that's desired
	if (outImageLen)
		*outImageLen = imageLen;
	if (!inImagePtr)
		return(1);

	header.header.magic = KRN_FOURCC(CPJ_LOD_MAGIC);
	header.header.lenFile = imageLen - 8;
	header.header.version = CPJ_LOD_VERSION;
	header.header.timeStamp = time(NULL);

	SLodFile* file = (SLodFile*)inImagePtr;
	memcpy(file, &header, offsetof(SLodFile, dataBlock));

	// set up image data pointers
	SLodLevel* fileLevels = (SLodLevel*)(&file->dataBlock[file->ofsLevels]);
	NWord* fileVertRelay = (NWord*)(&file->dataBlock[file->ofsVertRelay]);
	SLodTri* fileTriangles = (SLodTri*)(&file->dataBlock[file->ofsTriangles]);

	strcpy((char*)inImagePtr + file->header.ofsName, GetName());

	// levels
	NDword curTriangle = 0;
	NDword curVertRelay = 0;
	for (i=0;i<file->numLevels;i++)
	{
		SLodLevel* iL = &fileLevels[i];
		CCpjLodLevel* oL = &m_Levels[i];
		
		iL->detail = oL->detail;
		iL->numVertRelay = (NWord)oL->vertRelay.GetCount();
		iL->firstVertRelay = (NWord)curVertRelay;
		curVertRelay += iL->numVertRelay;
		for (j=0;j<iL->numVertRelay;j++)
			fileVertRelay[iL->firstVertRelay+j] = oL->vertRelay[j];
		iL->numTriangles = (NWord)oL->triangles.GetCount();
		iL->firstTriangle = (NWord)curTriangle;
		curTriangle += iL->numTriangles;
		for (j=0;j<iL->numTriangles;j++)
		{
			SLodTri* iT = &fileTriangles[iL->firstTriangle+j];
			CCpjLodTri* oT = &oL->triangles[j];
			iT->srfTriIndex = oT->srfTriIndex;
			iT->vertIndex[0] = oT->vertIndex[0];
			iT->vertIndex[1] = oT->vertIndex[1];
			iT->vertIndex[2] = oT->vertIndex[2];
			iT->uvIndex[0] = oT->uvIndex[0];
			iT->uvIndex[1] = oT->uvIndex[1];
			iT->uvIndex[2] = oT->uvIndex[2];
		}
	}

	return(1);
}

//****************************************************************************
//**
//**    END MODULE CPJLOD.CPP
//**
//****************************************************************************