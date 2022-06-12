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
	UTexture* tex;
	VVec3 xyz;
	VVec2 uv;
};

#define 	ANIM_TX   BIT( 0 )
#define 	ANIM_TY   BIT( 1 )
#define 	ANIM_TZ   BIT( 2 )
#define 	ANIM_QX   BIT( 3 )
#define 	ANIM_QY   BIT( 4 )
#define 	ANIM_QZ   BIT( 5 )

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

/*
============
COM_SkipPath
============
*/
char* SkipPath(char* pathname)
{
	char* last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname == '/')
			last = pathname + 1;
		pathname++;
	}
	return last;
}

void UDukeMeshInstance::EvalBoneRelativeTransforms(OCpjSequence* sequence, CCpjSeqFrame* frm, VCoords3* frmDeltaCoords, NBool *bonesUsed)
{
	CMacBone* bone;
	NDword index;

	for (int i = 0; i < Mac->mActorBones.GetCount(); i++)
	{
		bonesUsed[i] = 0;
	}

	// map bone information from sequence
	static CMacBone* boneMap[256];
	for (i = 0; i < sequence->m_BoneInfo.GetCount(); i++)
		boneMap[i] = Mac->FindBone(*sequence->m_BoneInfo[i].name);

	for (i = 0; i < frm->translates.GetCount(); i++)
	{
		//bone = inActor->FindBone(*mSequence->m_BoneInfo[frm[ifrm]->translates[i].boneIndex].name);
		bone = boneMap[frm->translates[i].boneIndex];
		if (!bone)
			continue;
		index = bone - &Mac->mActorBones[0];
		bonesUsed[index] = 1;
		frmDeltaCoords[index].t = frm->translates[i].translate;
		/*
					NFloat desiredLength = mSequence->m_BoneInfo[frm[ifrm]->translates[i].boneIndex].srcLength;
					NFloat actualLength = bone->mSklBone->length;
					NFloat lengthScale = actualLength / desiredLength;
					frmDeltaCoords[ifrm][index].t /= lengthScale;
		*/
	}
	for (i = 0; i < frm->scales.GetCount(); i++)
	{
		//bone = inActor->FindBone(*mSequence->m_BoneInfo[frm[ifrm]->scales[i].boneIndex].name);
		bone = boneMap[frm->scales[i].boneIndex];
		if (!bone)
			continue;
		index = bone - &Mac->mActorBones[0];
		bonesUsed[index] = 1;
		frmDeltaCoords[index].s = frm->scales[i].scale;
	}
	for (i = 0; i < frm->rotates.GetCount(); i++)
	{
		//bone = inActor->FindBone(*mSequence->m_BoneInfo[frm[ifrm]->rotates[i].boneIndex].name);
		bone = boneMap[frm->rotates[i].boneIndex];
		if (!bone)
			continue;
		index = bone - &Mac->mActorBones[0];
		bonesUsed[index] = 1;
#if 0
		VQuat3 q;
		q.AxisAngle(VVec3(0, 0, 1), (float)frm[ifrm]->rotates[i].roll * M_PI / 32768.f); frmDeltaCoords[ifrm][index].r >>= q;
		q.AxisAngle(VVec3(1, 0, 0), (float)frm[ifrm]->rotates[i].pitch * M_PI / 32768.f); frmDeltaCoords[ifrm][index].r >>= q;
		q.AxisAngle(VVec3(0, 1, 0), (float)frm[ifrm]->rotates[i].yaw * M_PI / 32768.f); frmDeltaCoords[ifrm][index].r >>= q;
#else
#ifdef CPJ_SEQ_NOQUATOPT			
		VEulers3 eulers;
		eulers.r = (float)frm[ifrm]->rotates[i].roll * M_PI / 32768.f;
		eulers.p = (float)frm[ifrm]->rotates[i].pitch * M_PI / 32768.f;
		eulers.y = (float)frm[ifrm]->rotates[i].yaw * M_PI / 32768.f;
		frmDeltaCoords[ifrm][index].r >>= (~eulers);
#else			
		frmDeltaCoords[index].r >>= frm->rotates[i].UnrealQuat;
#endif // CPJ_SEQ_NOQUATOPT
#endif // if 0
		/*
					NFloat desiredLength = mSequence->m_BoneInfo[frm[ifrm]->rotates[i].boneIndex].srcLength;
					NFloat actualLength = bone->mSklBone->length;
					NFloat lengthScale = actualLength / desiredLength;
					q = VQuat3();
					if (lengthScale < 1.f)
						q.Slerp(VQuat3(VAxes3()), VQuat3(frmDeltaCoords[ifrm][index].r), 1.f-lengthScale, lengthScale, false);
					else
						q.Slerp(VQuat3(VAxes3()), VQuat3(frmDeltaCoords[ifrm][index].r), 0.f, lengthScale, false);
					frmDeltaCoords[ifrm][index].r = q;
		*/
	}

	for (i = 0; i < sequence->m_BoneInfo.GetCount(); i++)
	{
		if (!bonesUsed[i])
			continue;

		// Lets put this into the right space.
		frmDeltaCoords[i].t = VVec3(frmDeltaCoords[i].t.x, frmDeltaCoords[i].t.z, -frmDeltaCoords[i].t.y);
	}
}

void UDukeMeshInstance::GetAnimBitsForTransform(OCpjSequence* sequence, FDukeExportJoint* joint, VCoords3& jointTransform)
{
	VQuat3 UnrealQuat(jointTransform.r);

	if(jointTransform.t.x != 0.0f)
		joint->animBits |= ANIM_TX;

	if (jointTransform.t.y != 0.0f)
		joint->animBits |= ANIM_TY;

	if (jointTransform.t.z != 0.0f)
		joint->animBits |= ANIM_TZ;

	if (UnrealQuat.v.x != 0.0f)
		joint->animBits |= ANIM_QX;

	if (UnrealQuat.v.y != 0.0f)
		joint->animBits |= ANIM_QY;

	if (UnrealQuat.v.z != 0.0f)
		joint->animBits |= ANIM_QZ;
}

void UDukeMeshInstance::WriteAnimatedJointTransform(OCpjSequence* sequence, FILE *f, FDukeExportJoint* joint, VCoords3& jointTransform)
{
	VQuat3 UnrealQuat(jointTransform.r);

	FVector trans(jointTransform.t.x, jointTransform.t.y, jointTransform.t.z);

	fprintf(f, "\t");
	if (joint->animBits) {
		if (joint->animBits & ANIM_TX) {
			fprintf(f, " %f", trans.X);
		}
		if (joint->animBits & ANIM_TY) {
			fprintf(f, " %f", trans.Y);
		}
		if (joint->animBits & ANIM_TZ) {
			fprintf(f, " %f", trans.Z);
		}
		if (joint->animBits & ANIM_QX) {
			fprintf(f, " %f", UnrealQuat.v.x);
		}
		if (joint->animBits & ANIM_QY) {
			fprintf(f, " %f", UnrealQuat.v.y);
		}
		if (joint->animBits & ANIM_QZ) {
			fprintf(f, " %f", UnrealQuat.v.z);
		}
	}
	fprintf(f,"\n");
}

void UDukeMeshInstance::ExportEvents(const char* tempFileName, OCpjSequence* sequence)
{
	if (sequence->m_Events.GetCount() == 0)
		return;

	char eventFileName[512];
	sprintf(eventFileName, "%s_%s.events", tempFileName, sequence->GetName());

	FILE* f = fopen(eventFileName, "wb");
	fprintf(f, "frame,eventName,paramString\n");

	for (int i = 0; i < sequence->m_Events.GetCount(); i++)
	{
		CCpjSeqEvent* ev = &sequence->m_Events[i];

		int frame = ev->time * sequence->m_Frames.GetCount();

		fprintf(f, "%d,", frame);

		switch (ev->eventType)
		{
			case SEQEV_INVALID:
				fprintf(f, "SEQEV_INVALID,");
				break;

			case SEQEV_MARKER:
				fprintf(f, "SEQEV_MARKER,");
				break;
			case SEQEV_TRIGGER:
				fprintf(f, "SEQEV_TRIGGER,");
				break;
			case SEQEV_ACTORCMD:
				fprintf(f, "SEQEV_ACTORCMD,");
				break;
			case SEQEV_TRIFLAGS:
				fprintf(f, "SEQEV_TRIFLAGS,");
				break;
			default:
				assert(!"Invalid SEQ flag!");
		}

		fprintf(f, "%s\n", ev->paramString.Str());
	}


	fclose(f);
}

void UDukeMeshInstance::ExportSequence(const char* tempFileName, TArray< FDukeExportJoint>& joints, OCpjSequence* sequence)
{
	char animFileName[512];
	int numAnimatedComponents = 0;

	sequence->CacheIn();

	if (sequence->m_Frames.GetCount() == 0)
		return;

	for (int i = 0; i < joints.Num(); i++)
	{
		joints(i).animBits = 0;

		for (int d = 0; d < sequence->m_Frames.GetCount(); d++)
		{
			VCoords3 boneCoords[256];
			NBool bonesUsed[256];

			EvalBoneRelativeTransforms(sequence, &sequence->m_Frames[d], &boneCoords[0], &bonesUsed[0]);
			if (!bonesUsed[i])
			{
				continue;
			}

			GetAnimBitsForTransform(sequence, &joints(i), boneCoords[i]);
		}

		joints(i).firstComponent = numAnimatedComponents;
		for (int j = 0; j < 6; j++) {
			if (joints(i).animBits & BIT(j)) {
				numAnimatedComponents++;
			}
		}
	}

	ExportEvents(tempFileName, sequence);

	sprintf(animFileName, "%s_%s.md5anim", tempFileName, sequence->GetName());

	FILE* f = fopen(animFileName, "wb");

	fprintf(f, "MD5Version 10\n");
	fprintf(f, "commandline \"This file has been generated from the DNF MD5 exporter by Justin Marshall\"\n\n");

	fprintf(f, "numFrames %d\n", sequence->m_Frames.GetCount());
	fprintf(f, "numJoints %d\n", joints.Num());
	fprintf(f, "frameRate 24\n");
	fprintf(f, "numAnimatedComponents %d\n", numAnimatedComponents); // All components enabled.

	fprintf(f, "\nhierarchy {\n");
	for (int i = 0; i < joints.Num(); i++)
	{
		FDukeExportJoint& joint = joints(i);

		if (joint.parent != -1) {
			fprintf(f, "\t\"%s\"\t%d %d %d\n", joint.boneName, joint.parent, joint.animBits, joint.firstComponent);
		}
		else {
			fprintf(f, "\t\"%s\"\t-1 %d %d\n", joint.boneName, joint.animBits, joint.firstComponent);
		}
	}
	fprintf(f, "}\n");

	// write the frame bounds
	fprintf(f, "\nbounds {\n");
	for (int i = 0; i < sequence->m_Frames.GetCount(); i++) {
		fprintf(f, "\t( -200 -200 -200 ) ( 200 200 200 )\n");
	}
	fprintf(f, "}\n");

	fprintf(f, "\nbaseframe {\n");
	for (int i = 0; i < joints.Num(); i++)
	{
		fprintf(f, "\t( %f %f %f ) ( %f %f %f )\n", joints(i).local_xyz.X, joints(i).local_xyz.Y, joints(i).local_xyz.Z, joints(i).orient.X, joints(i).orient.Y, joints(i).orient.Z);
	}
	fprintf(f, "}\n");

	for (int i = 0; i < sequence->m_Frames.GetCount(); i++)
	{
		fprintf(f, "\nframe %d {\n", i);

		CCpjSeqFrame* frame = &sequence->m_Frames[i];

		VCoords3 boneCoords[256];
		NBool bonesUsed[256];

		EvalBoneRelativeTransforms(sequence, &sequence->m_Frames[i], &boneCoords[0], &bonesUsed[0]);

		for (int d = 0; d < joints.Num(); d++)
		{
			if (!bonesUsed[d])
			{
				continue;
			}

			WriteAnimatedJointTransform(sequence, f, &joints(d), boneCoords[d]);
		}

		fprintf(f, "}\n");
	}

	fclose(f);
}

void UDukeMeshInstance::ExportSequences(const char* fileName)
{
	TArray< FDukeExportJoint> joints;

	char tempFileName[512];

	GatherExportJoints(joints);

	StripExtension(fileName, tempFileName);

	for (int i = 0; i < Mac->mSequences.GetCount(); i++)
	{
		ExportSequence(tempFileName, joints, Mac->mSequences[i]);
	}
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
			tri.tris[0] = (mesh->tris.Num() * 3) + 2;
			tri.tris[1] = (mesh->tris.Num() * 3) + 1;
			tri.tris[2] = (mesh->tris.Num() * 3) + 0;
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

		if (bone->mParent != nullptr)
		{
			exportJoint.parent = bone->mParent - &Mac->mActorBones[0];
		}

		VCoords3 v = bone->GetCoords(false);

		exportJoint.xyz.X = v.t.x;
		exportJoint.xyz.Y = v.t.z;
		exportJoint.xyz.Z = v.t.y;

		exportJoint.local_xyz.X = v.t.x;
		exportJoint.local_xyz.Y = v.t.z;
		exportJoint.local_xyz.Z = v.t.y;
		

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

void UDukeMeshInstance::ExportMounts(const char* fileName)
{
	if (Mac->mSkeleton->m_Mounts.GetCount() == 0)
		return;

	char tempFileName[512];
	StripExtension(fileName, tempFileName);

	char eventFileName[512];
	sprintf(eventFileName, "%s.mounts", tempFileName);

	FILE* f = fopen(eventFileName, "wb");
	fprintf(f, "mount_name,bound_name,x,y,z,yaw,pitch,roll\n");

	for (int i = 0; i < Mac->mSkeleton->m_Mounts.GetCount(); i++)
	{
		VEulers3 euler(Mac->mSkeleton->m_Mounts[i].baseCoords.r);

		fprintf(f, "%s,%s,%f,%f,%f,%f,%f,%f\n", Mac->mSkeleton->m_Mounts[i].name.Str(), Mac->mSkeleton->m_Mounts[i].bone->name.Str(), Mac->mSkeleton->m_Mounts[i].baseCoords.t.x, Mac->mSkeleton->m_Mounts[i].baseCoords.t.y, Mac->mSkeleton->m_Mounts[i].baseCoords.t.z, euler.y, euler.p, euler.r);
	}

	fclose(f);
}

void UDukeMeshInstance::ExportToMD5Mesh(const char* fileName)
{
	TArray< FDukeExportJoint> joints;
	TArray< FDukeExportMesh> meshes;

	static VVec3 verts[6635];
	int numVerts = Mac->EvaluateVerts(1, 1.0, &verts[0]);

	GatherExportJoints(joints);

	GatherExportMeshes(fileName, joints, meshes);

	ExportMounts(fileName);

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

void UDukeMeshInstance::ExportToMD3(const char* fileName)
{
	if (!Mesh)
	{
		return;
	}

	static VVec3 verts[6635];

	int numVerts = Mac->EvaluateVerts(1, 1.0, &verts[0]);

	static SMacTri TempTris[4096];
	INT NumListTris = Mac->EvaluateTris(1, TempTris);

	TArray<md3ExportSurface_t> meshes;

	// First pass find all of the texture groups.
	for (int i = 0; i < NumListTris; i++)
	{
		md3ExportSurface_t* mesh = nullptr;

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
			md3ExportSurface_t newMesh;
			newMesh.texture = (UTexture*)TempTris[i].texture->imagePtr;

			char temp[512];
			sprintf(temp, "%s", fileName);;
			StripExtension(temp, temp);
			sprintf(newMesh.name, "%s_%d", temp, meshes.Num());

			ExportTexture(newMesh.texture, newMesh.name);

			sprintf(newMesh.name, "%s_%d", SkipPath(temp), meshes.Num());

			meshes.AddItem(newMesh);

			mesh = &meshes(meshes.Num() - 1);
		}

		for (int d = 0; d < 3; d++)
		{
			int index = TempTris[i].vertIndex[d];

			FVector xyz;
			FVector uv;
			
			memcpy(&xyz, &verts[index], sizeof(VVec3));
			memcpy(&uv, TempTris[i].texUV[d], sizeof(VVec2));

			uv.Y = 1.0 - uv.Y;

			mesh->vertexes.AddItem(xyz);
			mesh->uv.AddItem(uv);
		}
	}

	WriteToMD3(fileName, meshes);
}


void UDukeMeshInstance::WriteToMD3(const char* fileName, const TArray<md3ExportSurface_t>& surfaces)
{
	md3Header_t header = { };

	header.ident = MD3_IDENT;
	header.version = MD3_VERSION;
	header.numFrames = 1;
	header.numSurfaces = surfaces.Num();
	header.numSkins = surfaces.Num();
	header.numTags = 0;
	strcpy(header.name, "DukeMesh");

	FILE* f = fopen(fileName, "wb");

	fwrite(&header, 1, sizeof(md3Header_t), f);

	header.ofsFrames = ftell(f);
	for (int i = 0; i < header.numFrames; i++)
	{
		md3Frame_t frame = { };
		fwrite(&frame, 1, sizeof(md3Frame_t), f);
	}

	header.ofsSurfaces = ftell(f);
	for (int i = 0; i < header.numSurfaces; i++)
	{
		md3Surface_t surf = { };

		sprintf(surf.name, "surf_%d", i);
		surf.numFrames = header.numFrames;
		surf.numShaders = 1;
		surf.numTriangles = surfaces(i).vertexes.Num();
		surf.numVerts = surfaces(i).vertexes.Num();

		int surfWriteLocation = ftell(f);
		fwrite(&surf, 1, sizeof(md3Surface_t), f);

		surf.ofsXyzNormals = ftell(f) - surfWriteLocation;

		for (int d = 0; d < surfaces(i).vertexes.Num(); d++)
		{
			md3XyzNormal_t xyz;

			xyz.xyz[0] = surfaces(i).vertexes(d).X / MD3_XYZ_SCALE;
			xyz.xyz[1] = surfaces(i).vertexes(d).Y / MD3_XYZ_SCALE;
			xyz.xyz[2] = surfaces(i).vertexes(d).Z / MD3_XYZ_SCALE;

			xyz.normal = 0;

			fwrite(&xyz, 1, sizeof(md3XyzNormal_t), f);
		}

		surf.ofsTriangles = ftell(f) - surfWriteLocation;

		int index = 0;
		for (int d = 0; d < surfaces(i).vertexes.Num(); d++)
		{
			md3Triangle_t tri;

			tri.indexes[0] = index + 2;
			tri.indexes[1] = index + 1;
			tri.indexes[2] = index + 0;

			index += 3;

			fwrite(&tri, 1, sizeof(md3Triangle_t), f);
		}

		surf.ofsSt = ftell(f) - surfWriteLocation;
		for (int d = 0; d < surfaces(i).uv.Num(); d++)
		{
			md3St_t st;

			st.st[0] = surfaces(i).uv(d).X;
			st.st[1] = surfaces(i).uv(d).Y;
			fwrite(&st, 1, sizeof(md3St_t), f);
		}
		surf.ofsShaders = ftell(f) - surfWriteLocation;
		md3Shader_t shader;
		strcpy(shader.name, surfaces(i).name);
		fwrite(&shader, 1, sizeof(md3Shader_t), f);

		surf.ofsEnd = ftell(f) - surfWriteLocation;
		fseek(f, surfWriteLocation, SEEK_SET);
		fwrite(&surf, 1, sizeof(md3Surface_t), f);

		fseek(f, surfWriteLocation + surf.ofsEnd, SEEK_SET);
	}

	fseek(f, 0, SEEK_SET);
	fwrite(&header, 1, sizeof(md3Header_t), f);

	fclose(f);
}