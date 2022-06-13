// DnMeshMD3.cpp - MD3 Loading code for Duke Nukem Forever by Justin Marshall
//

#include "EnginePrivate.h"

void UDukeMesh::LoadMD3(byte* buffer, int bufferLen)
{
	md3Header_t*		md3;
	int					version;
	int					size;

	md3 = (md3Header_t*)buffer;

	version = md3->version;
	if (version != MD3_VERSION) {
		appErrorf(TEXT("UDukeMeshInstance::LoadMD3: wrong version (%i should be %i)"), version, MD3_VERSION);		
		return;
	}

	// swap all the surfaces
	md3Surface_t *surf = (md3Surface_t*)((byte*)md3 + md3->ofsSurfaces);
	for (int i = 0; i < md3->numSurfaces; i++) {
		md3ImportSurface_t importSurface;

		// change to surface identifier
		surf->ident = 0;	//SF_MD3;

		// register the shaders
		md3Shader_t* shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);

		strcpy(importSurface.name, surf->name);

		//importSurface.texture = 
		//
		//
		//for (j = 0; j < surf->numShaders; j++, shader++) {
		//	const idMaterial* sh;
		//
		//	sh = declManager->FindMaterial(shader->name);
		//	shader->shader = sh;
		//}

		// swap all the triangles
		md3Triangle_t *tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (int j = 0; j < surf->numTriangles; j++, tri++) {
			importSurface.indexes.AddItem(tri->indexes[0]);
			importSurface.indexes.AddItem(tri->indexes[1]);
			importSurface.indexes.AddItem(tri->indexes[2]);
		}

		md3St_t *st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for (int j = 0; j < surf->numVerts; j++, st++) {
			FVector uv;
			uv.X = st->st[0];
			uv.Y = st->st[1];
			importSurface.uv.AddItem(uv);
		}

		md3XyzNormal_t *xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++)
		{
			importSurface.vertexes.AddItem(FVector(xyz->xyz[0] * MD3_XYZ_SCALE, xyz->xyz[1] * MD3_XYZ_SCALE, xyz->xyz[2] * MD3_XYZ_SCALE));
		}

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}
}