/*=============================================================================
	UnMeshRn.cpp: Unreal mesh rendering.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney		
=============================================================================*/

#include "EnginePrivate.h"

/*------------------------------------------------------------------------------
	Globals.
------------------------------------------------------------------------------*/
UBOOL               HasSpecialCoords;
FCoords             SpecialCoords;
static FLOAT        UScale, VScale;
static UTexture*    Textures[16];
static FTextureInfo TextureInfo[16];
static FTextureInfo EnvironmentInfo;
static FPlane      GUnlitColor;

EXECVAR(UBOOL, DisableMeshes, false);

/*------------------------------------------------------------------------------
	Environment mapping.
------------------------------------------------------------------------------*/
 
static __forceinline void EnviroMap( FSceneNode* Frame, FTransTexture& P )
{
	FVector T = P.Point.UnsafeNormal().MirrorByVector( P.Normal ).TransformVectorBy( Frame->Uncoords );
	P.U = (T.X+1.f) * 0.5f * UScale;
	P.V = (T.Y+1.f) * 0.5f * VScale;
}

/*--------------------------------------------------------------------------
	Clippers.
--------------------------------------------------------------------------*/

static FLOAT Dot[32];
static inline INT Clip( FSceneNode* Frame, FTransTexture** Dest, FTransTexture** Src, INT SrcNum )
{
	INT DestNum=0;
	for( INT i=0,j=SrcNum-1; i<SrcNum; j=i++ )
	{
		if( Dot[j]>=0.f )
		{
			Dest[DestNum++] = Src[j];
		}
		if( Dot[j]*Dot[i]<0.f )
		{
			FTransTexture* T = Dest[DestNum] = New<FTransTexture>(GMem);
			*T = FTransTexture( *Src[j] + (*Src[i]-*Src[j]) * (Dot[j]/(Dot[j]-Dot[i])) );
			T->Project( Frame );
			DestNum++;
		}
	}
	return DestNum; 
}

/*------------------------------------------------------------------------------
	Subsurface rendering.
------------------------------------------------------------------------------*/

// Triangle subdivision table.
/*
static const int CutTable[8][4][3] =
{
	{{0,1,2},{9,9,9},{9,9,9},{9,9,9}},
	{{0,3,2},{2,3,1},{9,9,9},{9,9,9}},
	{{0,1,4},{4,2,0},{9,9,9},{9,9,9}},
	{{0,3,2},{2,3,4},{4,3,1},{9,9,9}},
	{{0,1,5},{5,1,2},{9,9,9},{9,9,9}},
	{{0,3,5},{5,3,1},{1,2,5},{9,9,9}},
	{{0,1,4},{4,2,5},{5,0,4},{9,9,9}},
	{{0,3,5},{3,1,4},{5,4,2},{3,4,5}}
};
*/
static FVector RenderSubsurface_MinExtent=FVector(FLT_MAX,FLT_MAX,FLT_MAX);
static FVector RenderSubsurface_MaxExtent=FVector(FLT_MIN,FLT_MIN,FLT_MIN);

static void __fastcall RenderSubsurface
(
	bool            Batch,
	FSceneNode*		Frame,
	FTextureInfo&	Texture,
	UTexture       *Tex,
	FSpanBuffer*	Span,
	FTransTexture**	Pts,
	DWORD			PolyFlags,
	DWORD			PolyFlagsEx,
	INT				SubCount,
	UBOOL			ModifyExtent=0
)
{
#define EXT_RETURN \
	{ \
		if (PolyFlags & PF_Unlit) \
		{ \
			for (INT i=0;i<3;i++) \
				Pts[i]->Light = BackupPts[i]; \
		} \
		return; \
	}

	check(Frame&&(Frame->Viewport)&&(Frame->Viewport->RenDev));

	if(!Frame->Viewport->RenDev->QueuePolygonDoes())
	{
		// If outcoded, skip it.
		if( Pts[0]->Flags & Pts[1]->Flags & Pts[2]->Flags )
			return;
		// Backface reject it.
		if( (PolyFlags & PF_TwoSided) && FTriple(Pts[0]->Point,Pts[1]->Point,Pts[2]->Point) <= 0.0 )
		{
			if( !(PolyFlags & PF_TwoSided) )
				return;
			Exchange( Pts[2], Pts[0] );
		}
	}
	/* NJS: Reject small polys: */
#if 0
	for( INT i=0; i<2; i++ )
	{
		if((fabs(Pts[i]->ScreenY-Pts[i+1]->ScreenY)>0.0001f)
		 &&(fabs(Pts[i]->ScreenX-Pts[i+1]->ScreenX)>0.0001f))
			break;
	}

	if(i==2) return;
#endif

	FPlane BackupPts[3];

	// Handle effects.
	if( PolyFlags & (PF_Environment | PF_Unlit) )
	{
		// Environment mapping.
		if( PolyFlags & PF_Environment )
			for( INT i=0; i<3; i++ )
				EnviroMap( Frame, *Pts[i] );

		// Handle unlit.
		if( PolyFlags & PF_Unlit )
			for( INT j=0; j<3; j++ )
			{
				BackupPts[j] = Pts[j]->Light;
				Pts[j]->Light = GUnlitColor;
			}
	}

	// Clip it.
	INT NumPts=3;
	if(!Frame->Viewport->RenDev->QueuePolygonDoes())
	{

		BYTE AllCodes = Pts[0]->Flags | Pts[1]->Flags | Pts[2]->Flags;

		if( AllCodes )
		{
			if( AllCodes & FVF_OutXMin )
			{
				static FTransTexture* LocalPts[8];
				for( INT i=0; i<NumPts; i++ )
					Dot[i] = Frame->PrjXM * Pts[i]->Point.Z + Pts[i]->Point.X;
				NumPts = Clip( Frame, LocalPts, Pts, NumPts );
				if( NumPts==0 ) EXT_RETURN
				Pts = LocalPts;
			}
			if( AllCodes & FVF_OutXMax )
			{
				static FTransTexture* LocalPts[8];
				for( INT i=0; i<NumPts; i++ )
					Dot[i] = Frame->PrjXP * Pts[i]->Point.Z - Pts[i]->Point.X;
				NumPts = Clip( Frame, LocalPts, Pts, NumPts );
				if( NumPts==0 ) EXT_RETURN
				Pts = LocalPts;
			}
			if( AllCodes & FVF_OutYMin )
			{
				static FTransTexture* LocalPts[8];
				for( INT i=0; i<NumPts; i++ )
					Dot[i] = Frame->PrjYM * Pts[i]->Point.Z + Pts[i]->Point.Y;
				NumPts = Clip( Frame, LocalPts, Pts, NumPts );
				if( NumPts==0 ) EXT_RETURN
				Pts = LocalPts;
			}
			if( AllCodes & FVF_OutYMax )
			{
				static FTransTexture* LocalPts[8];
				for( INT i=0; i<NumPts; i++ )
					Dot[i] = Frame->PrjYP * Pts[i]->Point.Z - Pts[i]->Point.Y;
				NumPts = Clip( Frame, LocalPts, Pts, NumPts );
				if( NumPts==0 ) EXT_RETURN
				Pts = LocalPts;
			}
		}
		
		if( Frame->NearClip.W != 0.f )
		{
			UBOOL Clipped=0;
			for( INT i=0; i<NumPts; i++ )
			{
				Dot[i] = Frame->NearClip.PlaneDot(Pts[i]->Point);
				Clipped |= (Dot[i]<0.f);
			}
			if( Clipped )
			{
				static FTransTexture* LocalPts[8];
				NumPts = Clip( Frame, LocalPts, Pts, NumPts );
				if( NumPts==0 ) EXT_RETURN
				Pts = LocalPts;
			}
		}
		
		if(ModifyExtent&&!(PolyFlags&PF_Invisible))
		{
			for( INT i=0; i<NumPts; i++ )
			{
				ClipFloatFromZero(Pts[i]->ScreenX, Frame->FX);
					 if(RenderSubsurface_MinExtent.X>Pts[i]->ScreenX) RenderSubsurface_MinExtent.X=Pts[i]->ScreenX;
				else if(RenderSubsurface_MaxExtent.X<Pts[i]->ScreenX) RenderSubsurface_MaxExtent.X=Pts[i]->ScreenX;

				ClipFloatFromZero(Pts[i]->ScreenY, Frame->FY);
					 if(RenderSubsurface_MinExtent.Y>Pts[i]->ScreenY) RenderSubsurface_MinExtent.Y=Pts[i]->ScreenY;
				else if(RenderSubsurface_MaxExtent.Y<Pts[i]->ScreenY) RenderSubsurface_MaxExtent.Y=Pts[i]->ScreenY;
			}

		} else
		{
			for( INT i=0; i<NumPts; i++ )
			{
				ClipFloatFromZero(Pts[i]->ScreenX, Frame->FX);
				ClipFloatFromZero(Pts[i]->ScreenY, Frame->FY);
			}
		}
	}
// jmarshall
	//if(Batch)
	//	// Render it.
	//	Frame->Viewport->RenDev->QueuePolygon(&Texture, Pts, NumPts, PolyFlags, PolyFlagsEx|(Texture.Texture?Texture.Texture->PolyFlagsEx:0), Span );
	//else
		Frame->Viewport->RenDev->DrawGouraudPolygon(Frame,Texture,Pts,NumPts,PolyFlags,Span,PolyFlagsEx|(Texture.Texture?Texture.Texture->PolyFlagsEx:0));
// jmarshall end
	EXT_RETURN

#undef EXT_RETURN

}

/*------------------------------------------------------------------------------
	High level mesh rendering.
------------------------------------------------------------------------------*/

//
// Structure used by DrawMesh for sorting triangles.
//
INT __forceinline Compare( const FTransform* A, const FTransform* B )
{
	return appRound(B->Point.Z - A->Point.Z);
}

struct FMeshTriSortDuke
{
	SMacTri* Tri;
	INT Key;
};

EXECVAR_HELP(UBOOL, mesh_showbones, 0, "Display bone bounding boxes");
EXECVAR_HELP(FLOAT, mesh_wpndrawscale, 0.2, "Weapon draw scale");
EXECVAR_HELP(UBOOL, mesh_nospecular, 0, "Disable pseudo-specular glazing flag on mesh polys");
EXECVAR_HELP(UBOOL, mesh_notransparent, 0, "Disable transparent flag on mesh polys");
EXECVAR_HELP(UBOOL, mesh_nomodulated, 0, "Disable modulated flag on mesh polys");
EXECVAR_HELP(UBOOL, mesh_nomasking, 0, "Disable masking flag on mesh polys");
EXECVAR(FLOAT, mesh_specdist, 0.1f);
EXECVAR(FLOAT, mesh_wpnspecdist, 0.01f);
EXECVAR(FLOAT, mesh_lodforcelevel, 0);
EXECVAR_HELP(UBOOL, mesh_lodactive, 1, "Enables or disables mesh LOD");
EXECFUNC(GetSpecular)
{
	GDnExec->Printf(TEXT("%i"),!mesh_nospecular);
}
EXECFUNC(GetLodActive)
{
	GDnExec->Printf(TEXT("%i"),mesh_lodactive);
}

EXECFUNC(Lod)
{
	mesh_lodactive ^= 1;
	GDnExec->Printf(TEXT("Mesh level of detail reduction %s"),mesh_lodactive?TEXT("ON"):TEXT("OFF"));
}

#pragma warning(disable: 4505) // unreferenced local function

/*
	Color conversion convenience functions.
	Components in both RGB and HSV are in the [0,1] range.

	Cut&pasted directly from VecMain.h, and made static
	since the inlining of the VEC_RGBToHSV and VEC_HSVToRGB
	functions is causing havoc with the global
	optimizer in the draw function for some reason. - CDH
*/
static VVec3 __fastcall VEC_RGBToHSV_2(const VVec3& inRGB)
{
	float r = inRGB.x, g = inRGB.y, b = inRGB.z, v, x, f;
	int i;
	x = M_MIN3(r, g, b);
	v = M_MAX3(r, g, b);
	if (v == x)
		return(VVec3(0, 0, v));
	f = (r == x) ? g - b : ((g == x) ? b - r : r - g);
	i = (r == x) ? 3 : ((g == x) ? 5 : 1);
	return(VVec3((i-f/(v-x))/6.f, (v-x)/v, v));
}

static VVec3 __fastcall VEC_HSVToRGB_2(const VVec3& inHSV)
{
	float h = inHSV.x*6.f, s = inHSV.y, v = inHSV.z, m, n, f;
	int i;
	if (s == 0.f)
		return(VVec3(v,v,v));
	i = (int)h;
	f = h - i;
	if (!(i & 1))
		f = 1 - f;
	m = v * (1 - s);
	n = v * (1 - s*f);
	switch(i)
	{
		case 0: case 6: return(VVec3(v,n,m));
		case 1:			return(VVec3(n,v,m));
		case 2:			return(VVec3(m,v,n));
		case 3:			return(VVec3(m,n,v));
		case 4:			return(VVec3(n,m,v));
		case 5:			return(VVec3(v,m,n));
		default:		return(VVec3(0,0,0));
	};
}

// CDH: This function used to be below as part of DrawMesh but MSVC++'s ridiculous optimizer kept fubaring because that function is too large.
//      It is used to calculate vertex lighting when in HeatVision.  The name "StupidOptimizer" is intentional, as it is the most meaningful
//		description of this function possible, since the function would not exist if the optimizer weren't so insipid.
static void __fastcall StupidOptimizer(FSceneNode* Frame, UMeshInstance* MeshInst, ARenderActor* Owner, FTransTexture* Samples, INT NumVerts, const FCoords& Coords)
{
	FVector Center;
	UDukeMeshInstance* DukeMesh = Cast<UDukeMeshInstance>(MeshInst);
	CMacBone* ChestBone = NULL;
	if (DukeMesh)
		ChestBone = DukeMesh->Mac->FindBone( "Abdomen" );
	if (ChestBone)
	{
		VCoords3 c(ChestBone->GetCoords(true));
		FCoords fc(FVector(0,0,0), *((FVector*)&c.r.vX), *((FVector*)&c.r.vY), *((FVector*)&c.r.vZ));
		fc = fc.ToUnr();
		FCoords MeshCoords = DukeMesh->GetBasisCoords(GMath.UnitCoords);
		FCoords BoneCoords = GMath.UnitCoords;
		BoneCoords.XAxis = fc.XAxis.TransformVectorBy(MeshCoords); BoneCoords.XAxis.Normalize();
		BoneCoords.YAxis = fc.YAxis.TransformVectorBy(MeshCoords); BoneCoords.YAxis.Normalize();
		BoneCoords.ZAxis = fc.ZAxis.TransformVectorBy(MeshCoords); BoneCoords.ZAxis.Normalize();
		BoneCoords.Origin = *((FVector*)&c.t); BoneCoords.Origin = BoneCoords.Origin.ToUnr();
		BoneCoords.Origin = (BoneCoords.Origin - DukeMesh->Mac->mDukeOrigin).TransformPointBy(MeshCoords);
		FCoords OutCoords = BoneCoords.Transpose();
		Center = FVector(0,0,0).TransformPointBy(OutCoords);
	} else 
	{
		FBox Bound = MeshInst->GetRenderBoundingBox(Owner, false);
		Center = (Bound.Min + Bound.Max) * 0.5f;
	}
	Center = Center.TransformPointBy(Coords);
	FCoords InvFrameCoords = Frame->Coords.Transpose();

	FLOAT Radius2 = (FLOAT)Owner->HeatRadius;
	FLOAT Falloff2 = Radius2 + Radius2*(1.f-((FLOAT)Owner->HeatFalloff/256.f));
	for (INT i=0;i<NumVerts;i++)
	{
		FTransSample* Vert = &Samples[i];
		FLOAT VHeat = 1.f;
		FLOAT Dist2 = (Vert->Point - Center).Size();
		if (Dist2 > Radius2)
		{
			if (Dist2 > Falloff2)
				VHeat = 0.f;
			else
				VHeat = 1.f - ((Dist2 - Radius2) / (Falloff2 - Radius2));
		}

		FVector N = Vert->Normal.TransformVectorBy(InvFrameCoords);
		VVec3 v(Vert->Light.X, Vert->Light.Y, Vert->Light.Z);
		v = VEC_RGBToHSV_2((VVec3)v);
		//v.x = (FLOAT)fmod(0.15f - ((M_Fabs(N.Z) * 0.15f) + (1.f-VHeat)*((FLOAT)Owner->HeatFalloff/255.f)) + 1.f, 1.f);
		//v.x = VHeat;
		FLOAT temp1 = (M_Fabs(N.Z) * 0.15f);
		FLOAT temp2 = (1.f-VHeat)*(184.f/255.f);
		FLOAT temp3 = temp1 + temp2;
		v.x = (FLOAT)fmod(0.85f + temp3, 1.f);

		v.x = Min(v.x, 1.f);
		v.y = 1.f;
		v.z = 1.f;// - v.x;
		v = VEC_HSVToRGB_2(v);
		Vert->Light = FVector(v.x, v.y, v.z);
	}
}

// Draw a mesh map.
void __fastcall URender::DrawMesh
(
	FSceneNode*		Frame,
	AActor*			Owner,
	AActor*			LightSink,
	FSpanBuffer*	SpanBuffer,
	AZoneInfo*		Zone,
	const FCoords&	Coords,
	FVolActorLink*	LeafLights,
	FActorLink*		Volumetrics,
	DWORD			ExtraFlags, 
	DWORD			PolyFlagsEx
)
{
	if(DisableMeshes) 
		return;

	if(!Owner->Mesh||!Owner->Mesh->IsA(UDukeMesh::StaticClass()))
	{
		debugf(_T("Attempted to render an obsolete or unsupported mesh format: %s"),Owner->GetName());
		return;
	}

	
}

// Draw a mesh map.
void __fastcall URender::DrawMeshFast
(
	FSceneNode*		Frame,
	AActor*			Owner,
	AZoneInfo*		Zone,
	const FCoords&	Coords,
	DWORD			ExtraFlags,
	DWORD			PolyFlagsEx
)
{
	
}
