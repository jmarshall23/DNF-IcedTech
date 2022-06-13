//****************************************************************************
//**
//**    DNMESH.CPP
//**    DNF Mesh Objects
//**
//****************************************************************************
//============================================================================
//    HEADERS
//============================================================================

#include "EnginePrivate.h"

//============================================================================
//    PRIVATE DATA
//============================================================================
//============================================================================
//    GLOBAL DATA
//============================================================================
//============================================================================
//    PRIVATE FUNCTIONS
//============================================================================
/*-----------------------------------------------------------------------------
	CDH: Cannibal connection
-----------------------------------------------------------------------------*/

//============================================================================
//    GLOBAL FUNCTIONS
//============================================================================
//============================================================================
//    CLASS METHODS
//============================================================================

/*
	UDukeMesh
*/
IMPLEMENT_CLASS(UDukeMesh);

UDukeMesh::UDukeMesh()
{
}
void UDukeMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << ConfigName;

// jmarshall
	meshFileName = ConfigName.StripExtension();
	wchar_t tempFilePath[512];

	// Try to load in a MD3 first, if we find a MD3 mesh then this mesh is static.
	TArray<BYTE> md3FileBuffer;
	wsprintf(tempFilePath, TEXT("..\\Meshes\\%s.md3"), *meshFileName);
	if (appLoadFileToArray(md3FileBuffer, tempFilePath))
	{
		LoadMD3(&md3FileBuffer(0), md3FileBuffer.Num());
	}
	else
	{
		wsprintf(tempFilePath, TEXT("..\\Meshes\\%s"), *meshFileName);
		LoadMD5(tempFilePath);
	}
// jmarshall end
}

FBox UDukeMesh::GetRenderBoundingBox(const AActor* Owner, UBOOL Exact)
{
	
	FBox Bound = BoundingBox;

	UDukeMeshInstance* Inst = Cast<UDukeMeshInstance>(GetInstance((AActor*)Owner));
	if (!Inst)
		return(Bound);

	//if ( Owner->bIsRenderActor && ((ARenderActor*) Owner)->bCollisionForRenderBox )
	return GetCollisionBoundingBox( Owner );	
}

UClass* UDukeMesh::GetInstanceClass()
{
	return(UDukeMeshInstance::StaticClass());
}

UBOOL UDukeMesh::LineCheck(FCheckResult& Result, AActor* Owner, FVector End, FVector Start, FVector Extent, DWORD ExtraNodeFlags, UBOOL bMeshAccurate)
{
	return UPrimitive::LineCheck( Result, Owner, End, Start, Extent, ExtraNodeFlags, bMeshAccurate );
}

/*
	AMeshDecal
*/
void AMeshDecal::execBuildDecal( FFrame& Stack, RESULT_DECL )
{

	P_GET_OBJECT(AActor, InActor);
	P_GET_OBJECT(UTexture, InTexture);
	P_GET_INT(InTri);
	P_GET_VECTOR(InBaryCenter);
	P_GET_FLOAT(InRollRadians);
	P_GET_FLOAT(InDimU);
	P_GET_FLOAT(InDimV);
	P_FINISH;
#if 0 // todo decals.
	Texture = NULL;
	Actor = NULL;
	Mesh = NULL;
	Tris.Empty();
	Tris.Shrink();
	
	// an actor and texture are required
	if (!InTexture || !InActor || !InActor->Mesh)
	{
		*(INT*)Result = 0;
		return;
	}

	// grab the mesh instance
	UDukeMeshInstance* MeshInst = Cast<UDukeMeshInstance>(InActor->GetMeshInstance());
	if (!MeshInst)
	{
		*(INT*)Result = 0;
		return;
	}

	if ((InTri < 0) || (InTri >= (INT)MeshInst->Mac->mGeometry->m_Tris.GetCount()))
	{
		*(INT*)Result = 0;
		return;
	}

	// get vertices and triangles
	FMemMark Mark(GMem);
	SMacTri* TempTris = New<SMacTri>(GMem, MeshInst->Mac->mGeometry->m_Tris.GetCount());
	VVec3* TempVerts = New<VVec3>(GMem, MeshInst->Mac->mGeometry->m_Verts.GetCount());
	INT NumTris = MeshInst->Mac->EvaluateTris(1.f, TempTris);
	mesh_GetFrameNoTransform = 1;
	INT NumVerts = MeshInst->GetFrame((FVector*)TempVerts, NULL, sizeof(VVec3), GMath.UnitCoords, 1.f);
	mesh_GetFrameNoTransform = 0;

	// pull out vertices of base triangle
	VVec3 v[3];
	for (INT i=0;i<3;i++)
		v[i] = TempVerts[MeshInst->Mac->mGeometry->m_Tris[InTri].edgeRing[i]->tailVertex - &MeshInst->Mac->mGeometry->m_Verts[0]];

	// compute coords using this base
	VCoords3 baseCoords;
	baseCoords.t = v[0]*InBaryCenter.X + v[1]*InBaryCenter.Y + v[2]*InBaryCenter.Z;
	baseCoords.r.vZ = (v[2]-v[0]) ^ (v[1]-v[0]);
	baseCoords.r.vZ.Normalize();
	baseCoords.r.vY = v[0] - baseCoords.t;
	baseCoords.r.vY.Normalize();
	baseCoords.r.vX = baseCoords.r.vY ^ baseCoords.r.vZ;
	baseCoords.r.vX.Normalize();
	//VQuat3 q; q.AxisAngle(VVec3(0,0,1), InRollRadians);
	//baseCoords.f <<= q;
	VCoords3 deltaCoords = /*VCoords3() & */baseCoords;

	// compute which triangles fall into the rectangle of the decal and add them
	for (i=0;i<NumTris;i++)
	{
		SMacTri* Tri = &TempTris[i];
		for (DWORD j=0;j<3;j++)
		{
			v[j] = TempVerts[Tri->vertIndex[j]];
			v[j] >>= deltaCoords;
			///*
			v[j].x /= InDimU;
			v[j].y /= -InDimV;
			v[j].x += 0.5f;
			v[j].y += 0.5f;
			//*/
		}
		
		if ((v[0].x<0.f) && (v[1].x<0.f) && (v[2].x<0.f)) continue;
		if ((v[0].y<0.f) && (v[1].y<0.f) && (v[2].y<0.f)) continue;
		if ((v[0].x>1.f) && (v[1].x>1.f) && (v[2].x>1.f)) continue;
		if ((v[0].y>1.f) && (v[1].y>1.f) && (v[2].y>1.f)) continue;

		FMeshDecalTri* DecalTri = &Tris(Tris.Add());
		DecalTri->TriIndex = i;
		for (j=0;j<3;j++)
		{
			DecalTri->TexU[j] = v[j].x;
			DecalTri->TexV[j] = v[j].y;
		}
	}

	// return generated triangles
	if (Tris.Num())
	{
		Texture = InTexture;
		Actor = InActor;
		Mesh = InActor->Mesh;
	}
	
	*(INT*)Result = Tris.Num();
	
	Mark.Pop();
#endif
}
//IMPLEMENT_FUNCTION( AMeshDecal, INDEX_NONE, execBuildDecal );


//****************************************************************************
//**
//**    END MODULE DNMESH.CPP
//**
//****************************************************************************

