// Navmesh.uc (c) 2022 by icecoldduke
//

class NavMesh extends Actor
	native;

native function CreateNavMeshForLevel();

function PostBeginPlay()
{
	CreateNavMeshForLevel();
}

defaultproperties
{

}