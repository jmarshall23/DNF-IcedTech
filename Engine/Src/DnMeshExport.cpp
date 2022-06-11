// DnMeshExport.cpp - Justin Marshall(IceColdDuke)
//

//
// And so begins my war on the broken skeleton library called Cannibal.
// Mwhuaahh
//

#include "EnginePrivate.h"
#include <assert.h>

struct DnDukeVertex
{
	VVec3 xyz;
	VVec2 uv;
};

/*
============
COM_StripExtension
============
*/
void StripExtension(const char* in, char* out) {
	while (*in && *in != '.') {
		*out++ = *in++;
	}
	*out = 0;
}

void UDukeMeshInstance::WriteTGA(const char* filename, FRainbowPtr& data, const DWORD* palette, int width, int height, bool flipVertical) {
	byte* buffer;
	int		i, d;
	int		bufferSize = width * height * 4 + 18;
	int     imgStart = 18;

	buffer = (byte*)malloc(bufferSize);
	memset(buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 32;	// pixel size
	if (!flipVertical) {
		buffer[17] = (1 << 5);	// flip bit, for normal top to bottom raster order
	}

	for (i = imgStart, d = imgStart; i < bufferSize; i += 4, d += 4) {
		byte pixel[4];

		memcpy(&pixel[0], &palette[*data.PtrBYTE++], sizeof(DWORD));

		buffer[i] = pixel[0];		// red
		buffer[i + 1] = pixel[1];		// green
		buffer[i + 2] = pixel[2];		// blue
		buffer[i + 3] = pixel[3];		// alpha
	}

	char tempFileName[512];
	sprintf(tempFileName, "%s.tga", filename);

	FILE* f = fopen(tempFileName, "wb");
	fwrite(buffer, 1, bufferSize, f);
	fflush(f);
	fclose(f);

	free(buffer);
}

void UDukeMeshInstance::ExportSequences(const char* fileName)
{
	TArray< FDukeExportJoint> joints;

	GatherExportJoints(joints);

	for (int i = 0; i < Mac->mSequences.GetCount(); i++)
	{

	}
}

void UDukeMeshInstance::GatherExportMeshes(const char* fileName, const TArray< FDukeExportJoint>& joints, TArray< FDukeExportMesh>& meshes)
{
	static SMacTri TempTris[4096];
	INT NumListTris = Mac->EvaluateTris(1, TempTris);

	// First pass find all of the texture groups.
	for (int i = 0; i < NumListTris; i++)
	{
		FDukeExportMesh* mesh = nullptr;

		if (TempTris[i].texture == nullptr)
			continue;

		for (int d = 0; d < meshes.Num(); d++)
		{
			if (TempTris[i].texture->imagePtr == meshes(d).texture)
			{
				mesh = &meshes(d);
			}
		}

		if (mesh == nullptr)
		{
			FDukeExportMesh newMesh;
			newMesh.texture = (UTexture*)TempTris[i].texture->imagePtr;

			char temp[512];
			sprintf(temp, "%s", fileName);;
			StripExtension(temp, temp);
			sprintf(newMesh.textureName, "%s_%d", temp, meshes.Num());

			ExportTexture(newMesh.texture, newMesh.textureName);

			meshes.AddItem(newMesh);
		}
	}

	for (int i = 0; i < NumListTris; i++)
	{
		FDukeExportMesh* mesh = nullptr;

		if (TempTris[i].texture == nullptr)
			continue;

		for (int d = 0; d < meshes.Num(); d++)
		{
			if (TempTris[i].texture->imagePtr == meshes(d).texture)
			{
				mesh = &meshes(d);
			}
		}

		assert(mesh != nullptr);

		for (int f = 0; f < 3; f++)
		{
			int index = TempTris[i].vertIndex[f];

			CCpjSklVert* vert = &Mac->mSkeleton->m_Verts[index];
			FDukeExportVert v;

			v.numWeights = 0;
			v.weightIndex = mesh->weights.Num();
			v.u = TempTris[i].texUV[f]->x;
			v.v = TempTris[i].texUV[f]->y;

			for (int d = 0; d < vert->weights.GetCount(); d++)
			{
				FDukeExportWeight w;

				w.jointIndex = -1;

				for (int c = 0; c < Mac->mSkeleton->m_Bones.GetCount(); c++)
				{
					if (vert->weights[d].bone == Mac->mActorBones[c].mSklBone)
					{
						w.jointIndex = c;
						break;
					}
				}

				assert(w.jointIndex != -1);

				w.weightValue = vert->weights[d].factor;
				w.x = vert->weights[d].offsetPos.x * joints(w.jointIndex).scale.X;
				w.y = vert->weights[d].offsetPos.y * joints(w.jointIndex).scale.Z;
				w.z = vert->weights[d].offsetPos.z * joints(w.jointIndex).scale.Y;

				float z = w.z;
				w.z = w.y;
				w.y = z;

				mesh->weights.AddItem(w);
				v.numWeights++;
			}

			mesh->verts.AddItem(v);

			FDukeExportTri tri;
			tri.tris[0] = (mesh->tris.Num() * 3) + 0;
			tri.tris[1] = (mesh->tris.Num() * 3) + 1;
			tri.tris[2] = (mesh->tris.Num() * 3) + 2;
			mesh->tris.AddItem(tri);
		}
	}
}

void UDukeMeshInstance::GatherExportJoints(TArray< FDukeExportJoint>& joints)
{
	for (int i = 0; i < Mac->mActorBones.GetCount(); i++)
	{
		CMacBone* bone = &Mac->mActorBones[i];
		FDukeExportJoint exportJoint;

		sprintf(exportJoint.boneName, bone->mSklBone->name.Str(), bone->mSklBone->name.Len());
		exportJoint.parent = -1;

		if (bone->mSklBone->parentBone != nullptr)
		{
			for (int d = 0; d < Mac->mActorBones.GetCount(); d++)
			{
				if (bone->mSklBone->parentBone == Mac->mActorBones[d].mSklBone)
				{
					exportJoint.parent = d;
					break;
				}
			}

			assert(exportJoint.parent != -1);
		}

		VCoords3 v = bone->GetCoords(false);

		exportJoint.xyz.X = v.t.x;
		exportJoint.xyz.Y = v.t.z;
		exportJoint.xyz.Z = v.t.y;

		VQuat3 q = VQuat3(v.r);
		exportJoint.orient.X = 0;
		exportJoint.orient.Y = 0;
		exportJoint.orient.Z = 0;

		exportJoint.scale.X = v.s.x;
		exportJoint.scale.Y = v.s.y;
		exportJoint.scale.Z = v.s.z;

		joints.AddItem(exportJoint);
	}

	for (int i = 0; i < joints.Num(); i++)
	{
		FDukeExportJoint& exportJoint = joints(i);
		if (exportJoint.parent != -1)
		{
			FDukeExportJoint& parentJoint = joints(exportJoint.parent);

			exportJoint.xyz.X += parentJoint.xyz.X;
			exportJoint.xyz.Y += parentJoint.xyz.Y;
			exportJoint.xyz.Z += parentJoint.xyz.Z;
		}
	}
}

void UDukeMeshInstance::ExportToMD5Mesh(const char* fileName)
{
	TArray< FDukeExportJoint> joints;
	TArray< FDukeExportMesh> meshes;

	static VVec3 verts[6635];
	int numVerts = Mac->EvaluateVerts(1, 1.0, &verts[0]);

	GatherExportJoints(joints);

	GatherExportMeshes(fileName, joints, meshes);

	FILE* f = fopen(fileName, "wb");

	fprintf(f, "MD5Version 10\n");
	fprintf(f, "commandline \"This file has been generated from the DNF MD5 exporter by Justin Marshall\"\n\n");

	fprintf(f, "numJoints %d\n", joints.Num());
	fprintf(f, "numMeshes %d\n", meshes.Num());

	// Write out all the joints.
	fprintf(f, "joints {\n");
	for (int i = 0; i < joints.Num(); i++)
	{
		fprintf(f, "\t\"%s\" %d ( %f %f %f ) ( %f %f %f )\n", joints(i).boneName, joints(i).parent, joints(i).xyz.X, joints(i).xyz.Y, joints(i).xyz.Z, joints(i).orient.X, joints(i).orient.Y, joints(i).orient.Z);
	}
	fprintf(f, "}\n");

	// Write out all of the meshes. 
	for (int i = 0; i < meshes.Num(); i++)
	{
		fprintf(f, "mesh {\n");
		fprintf(f, "\tshader \"%s.tga\"\n", meshes(i).textureName);
		fprintf(f, "\tnumverts %d\n\n", meshes(i).verts.Num());
		for (int d = 0; d < meshes(i).verts.Num(); d++)
		{
			fprintf(f, "\tvert %d ( %f %f ) %d %d\n", d, meshes(i).verts(d).u, meshes(i).verts(d).v, meshes(i).verts(d).weightIndex, meshes(i).verts(d).numWeights);
		}

		fprintf(f, "\n\tnumtris %d\n\n", meshes(i).tris.Num());
		for (int d = 0; d < meshes(i).tris.Num(); d++)
		{
			fprintf(f, "\ttri %d %d %d %d\n", d, meshes(i).tris(d).tris[0], meshes(i).tris(d).tris[1], meshes(i).tris(d).tris[2]);
		}

		fprintf(f, "\n\tnumweights %d\n\n", meshes(i).weights.Num());
		for (int d = 0; d < meshes(i).weights.Num(); d++)
		{
			fprintf(f, "\tweight %d %d %f ( %f %f %f )\n", d, meshes(i).weights(d).jointIndex, meshes(i).weights(d).weightValue, meshes(i).weights(d).x, meshes(i).weights(d).y, meshes(i).weights(d).z);
		}

		fprintf(f, "}");
	}

	fclose(f);
}

void UDukeMeshInstance::ExportTexture(UTexture* texture, const char* fileName)
{
	FTextureInfo info;
	texture->Lock(info, appSeconds(), 0, NULL);

	FRainbowPtr SourceBitmap = info.Mips[0]->DataPtr;

#define PACK_DATA(r, g, b, a) ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))

	DWORD AlphaPalette[256];
	// Compute the alpha palette:
	for (INT i = 0; i < NUM_PAL_COLORS; i++)
		AlphaPalette[i] = PACK_DATA(info.Palette[i].R, info.Palette[i].G, info.Palette[i].B, info.Palette[i].A);

	WriteTGA(fileName, SourceBitmap, AlphaPalette, info.Mips[0]->USize, info.Mips[0]->VSize, false);

	texture->Unlock(info);
}

void UDukeMeshInstance::ExportToOBJ(const char* fileName)
{
	if (!Mesh)
	{
		return;
	}

	static VVec3 verts[6635];
	static SMacTri TempTris[4096];
	static DnDukeVertex dnVerts[6645];

	int numRealVerts = 0;

	INT NumListTris = Mac->EvaluateTris(1, TempTris);
	int numVerts = Mac->EvaluateVerts(1, 1.0, &verts[0]);

	for (int i = 0; i < NumListTris; i++)
	{
		for (int d = 0; d < 3; d++)
		{
			int index = TempTris[i].vertIndex[d];

			dnVerts[numRealVerts].xyz = verts[index];
			dnVerts[numRealVerts].uv = *TempTris[i].texUV[d];

			dnVerts[numRealVerts].uv.y = 1.0 - dnVerts[numRealVerts].uv.y;

			numRealVerts++;
		}
	}

	ExportTexture(GetTexture(0), fileName);

	FILE* f = fopen(fileName, "wb");

	fprintf(f, "# This file has been generated from the DNF obj exporter by Justin Marshall\n");

	// Write out all the vertexes.
	for (int i = 0; i < numRealVerts; i++)
	{
		fprintf(f, "v %f %f %f\n", dnVerts[i].xyz.x, dnVerts[i].xyz.y, dnVerts[i].xyz.z);
	}

	// Write out all the UV's.
	for (int i = 0; i < numRealVerts; i++)
	{
		fprintf(f, "vt %f %f\n", dnVerts[i].uv.x, dnVerts[i].uv.y);
	}

	for (int i = 0; i < numRealVerts; i += 3)
	{
		int idx1 = 1 + i + 0;
		int idx2 = 1 + i + 1;
		int idx3 = 1 + i + 2;

		fprintf(f, "f %d/%d %d/%d %d/%d\n", idx1, idx1, idx2, idx2, idx3, idx3);
	}

	fclose(f);
}
