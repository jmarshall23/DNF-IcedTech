//=============================================================================
// dnHRocket2trail.
//=============================================================================
class dnHRocket2trail expands dnMissileTrail;

defaultproperties
{
     SpawnPeriod=0.000000
     Lifetime=2.000000
     InitialVelocity=(Z=0.000000)
     InitialAcceleration=(Z=10.000000)
     MaxVelocityVariance=(X=0.000000,Y=0.000000,Z=10.000000)
     MaxAccelerationVariance=(Z=10.000000)
     RealtimeVelocityVariance=(Z=10.000000)
     UseZoneGravity=False
     Connected=True
     LineStartColor=(R=255,G=255,B=255)
     LineEndColor=(R=255,G=255,B=255)
     LineStartWidth=6.000000
     LineEndWidth=6.000000
     Textures(0)=Texture't_generic.Smoke.gensmoke1aRC'
     StartDrawScale=0.500000
     EndDrawScale=1.750000
     AlphaStart=0.123500
     AlphaMid=0.135000
     AlphaEnd=0.000000
     AlphaRampMid=0.135000
     bUseAlphaRamp=True
     RotationInitial=35000.000000
     RotationVariance=35000.000000
     RotationInitial3d=(Pitch=35000,Yaw=35000,Roll=35000)
     RotationVariance3d=(Pitch=35000,Yaw=35000,Roll=35000)
     TriggerType=SPT_Disable
     VisibilityRadius=8000.000000
     VisibilityHeight=8000.000000
     bHidden=True
     TimeWarp=0.750000
     CollisionRadius=0.000000
     CollisionHeight=0.000000
     Style=STY_Translucent
     bUnlit=True
}
