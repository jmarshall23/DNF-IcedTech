//=============================================================================
// dnCharacterFX_BurrowUP_Rocks.
//=============================================================================
class dnCharacterFX_BurrowUP_Rocks expands dnCharacterFX_BurrowSpawnUP;

defaultproperties
{
     DestroyWhenEmptyAfterSpawn=True
     CurrentSpawnNumber=10
     AdditionalSpawn(0)=(SpawnClass=None)
     AdditionalSpawn(1)=(SpawnClass=None)
     SpawnNumber=2
     PrimeCount=2
     MaximumParticles=9
     Lifetime=0.750000
     LifetimeVariance=0.000000
     InitialVelocity=(Z=256.000000)
     MaxVelocityVariance=(X=256.000000,Y=256.000000,Z=0.000000)
     LocalFriction=0.000000
     BounceElasticity=1.000000
     Bounce=False
     ParticlesCollideWithWorld=False
     Textures(0)=Texture't_generic.dirtcloud.dirtcloud1aRC'
     Textures(1)=Texture't_generic.dirtcloud.dirtcloud1cRC'
     DrawScaleVariance=0.000000
     EndDrawScale=3.000000
     RotationVariance=65535.000000
     RotationVelocityMaxVariance=4.000000
     RotationVariance3d=(Pitch=0,Yaw=0,Roll=0)
     TriggerOnSpawn=True
     TriggerType=SPT_Enable
     PulseSeconds=0.000000
     AlphaEnd=0.000000
     LodMode=LOD_Full
     LifeSpan=0.500000
     CollisionRadius=0.000000
     CollisionHeight=0.000000
     DrawType=DT_Sprite
     Style=STY_Translucent
     Mesh=None
}
