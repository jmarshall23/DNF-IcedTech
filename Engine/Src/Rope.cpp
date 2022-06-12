#include "EnginePrivate.h"
#include "Rope.h"

//#include "..\..\Cannibal\CannibalUnr.h"

#define m_baseBoneCoords ((VCoords3 *)(m_baseBoneCoords))

static VVec3 xAxis( 1, 0, 0 );
static VVec3 yAxis( 0, 1, 0 );
static VVec3 zAxis( 0, 0, 1 );

IMPLEMENT_CLASS(ABoneRope);
IMPLEMENT_CLASS(RopePrimitive);

#define ROPE_CYLINDERS(p) ((Cylinder*)p->m_ropeCylinders)

#pragma warning(disable:4714)

// JEP ...
//====================================================================
//	UpdateRopeRenderBox
//====================================================================
static void UpdateRopeRenderBox(ABoneRope *Rope)
{
	
}
// ... JEP

//====================================================================
//RotateOCS - Rotate an OCS about an axis by the specified angle
//====================================================================
void RotateOCS
    (
    VCoords3&   ocs, 
    VVec3       axis,
    float       angle
    )
{
    ocs >>= VAxes3( axis, angle );
}

//====================================================================
//RotateBone - Rotates the specified bone around the passed in axis and angle
//====================================================================
void RotateBone
    (
    CMacBone *bone,
    VVec3    axis,
    float    angle
    )

{
    VCoords3 base;
    VAxes3   rotAxes;

    rotAxes = VAxes3( axis, angle );

    base   = bone->GetCoords( false );
    base.r <<= rotAxes;
    bone->SetCoords( base, false );
}


//====================================================================
//RotateBone - Converts a world OCS into the OCS of the bone
//====================================================================
void World2Bone
    (
    CMacBone    *bone,
    VCoords3&   inWorldOCS
    )
{
	VCoords3 tempCoord;

	for (CMacBone *b = bone->mParent; b; b = b->mParent )
    {
        tempCoord <<= b->GetCoords(false);
    }
	inWorldOCS >>= tempCoord;
}

#if 0
//====================================================================
//Bone2World
//====================================================================
static void Bone2World
    (
    CMacBone *bone,
    VCoords3& myBaseWorld
    )
{
	myBaseWorld = bone->GetCoords(false);

	for (CMacBone* b = bone->mParent; b; b = b->mParent )
    {
		myBaseWorld <<= b->GetCoords(false);
    }
}
#endif

//====================================================================
//ABoneRope::ABoneRope
//====================================================================
ABoneRope::ABoneRope() 
{
	m_ropeCylinders=0;
}

//====================================================================
//ABoneRope::UpdateCylinders - Update the location and orientation of the
//bounding cylinders on the rope which are used for collision
//====================================================================
void ABoneRope::UpdateCylinders
    (
    void
    )

{
   
}

void ABoneRope::Destroy()
{
	debugf( TEXT(" ABoneRope::Destroy() : %X" ), m_ropeCylinders );

	if ( m_ropeCylinders )
	{
		appFree( (void *)m_ropeCylinders );
		//GMalloc->HeapCheck();
		//free( (void *)m_ropeCylinders );
		m_ropeCylinders = 0;
	}

	Super::Destroy();
}

//====================================================================
//ABoneRope::InitializeRope - Initializes the Rope and it's bounding
//cylinders.  This can be called multiple times to change the cylinders (i.e. for netplay)
//====================================================================
void ABoneRope::InitializeRope
    (
    void
    )

{
   
}

//====================================================================
//ABoneRope::execInitializeRope - Initializes the Rope and it's bounding
//cylinders
//====================================================================
void ABoneRope::execInitializeRope
    (
    FFrame& Stack,
    RESULT_DECL
    )

{
    P_FINISH;
    InitializeRope();
}

//====================================================================
//ABoneRope::GetBoneFromHandle
//====================================================================
void *ABoneRope::GetBoneFromHandle( int handle )
{
    return NULL;
}

//====================================================================
//ABoneRope::GetHandleFromBone
//====================================================================
INT ABoneRope::GetHandleFromBone( void *bone )
{
    return 0;
}

//====================================================================
//ABoneRope::execCheckCollision - Check to see if the ray collides with
//the rope.  Returns a pointer to the bone on the rope that was hit.
//====================================================================
void ABoneRope::execCheckCollision
    (
    FFrame& Stack,
    RESULT_DECL
    )

{
    VVec3          xAxis( 1, 0, 0 );
    VVec3          yAxis( 0, 1, 0 );
    VVec3          zAxis( 0, 0, 1 );

    P_GET_VECTOR(point);
    P_GET_VECTOR(dir);
    P_GET_FLOAT(max_distance);
    P_FINISH;

    *(INT*)Result = 0;

}

//======================================================================
//ABoneRope::GetRiderPosition
//Returns a value of from 0 to 1.0 based on the distance that the rider is
//away from the base of the rope.  
//======================================================================
FLOAT ABoneRope::GetRiderPosition
    (
    void
    )

{
    if ( !m_Rider )
        return 0;

    FVector delta = m_Rider->Location - Location;
    return ( delta.Size() / m_ropeLength );
}

//====================================================================
//ABoneRope::CalculateDirectionVector - Calculates a vector based on the
//angular displacement of the rope
//====================================================================
FVector ABoneRope::CalculateDirectionVector
   (
   void
   )

   {
   FVector dir;

   // convert the angular displacement into a vector
   dir.X = sin( m_angularDisplacement.X );
   dir.Y = sin( m_angularDisplacement.Y );
   dir.Z = -1;
   dir.Normalize();

   return dir;
   }

void CheapBroadcastMessage(AActor* inActor, TCHAR* inFmt, ... )
{ 
	static TCHAR buf[256];
	GET_VARARGS( buf, ARRAY_COUNT(buf), *(const TCHAR**)&inFmt );
	inActor->Level->eventBroadcastMessage(FString(buf),0,NAME_None);
}

//====================================================================
//ABoneRope::execOnRope - Called when a player gets on the rope
//====================================================================
void ABoneRope::execOnRope( FFrame& Stack, RESULT_DECL )
{
    P_FINISH;
    
    // Give the rope a bit of a push in the direction that the player is moving, so 
    // it feels like the rope is moving a bit
    if ( m_Rider )
    {
        m_angularVelocity.X += m_Rider->Velocity.X * m_angularInputVelocityScale / m_ropeLength;
        m_angularVelocity.Y += m_Rider->Velocity.Y * m_angularInputVelocityScale / m_ropeLength;
    }
    m_swingStateAway = true;
}

//====================================================================
//ABoneRope::execDamageRope - Damage to a rope - passes in a location and a dir
//====================================================================
void ABoneRope::execDamageRope( FFrame& Stack, RESULT_DECL )
{
    P_GET_VECTOR( hitLocation );
    P_GET_VECTOR( hitDirection );
    P_FINISH;

    return;

    /*
    if ( m_lastHitTime + 0.1 > Level->TimeSeconds )
        return;

    m_lastHitTime = Level->TimeSeconds;

    // Find the bone that is closest to this location
    UDukeMeshInstance* MeshInst = Cast<UDukeMeshInstance>( GetMeshInstance() );

    if ( !MeshInst )
        return;
   
    if ( m_Rider )
        return;

    CMacBone    *bone          = MeshInst->Mac->mActorBones.GetData();
    NDword      BoneCount      = MeshInst->Mac->mActorBones.GetCount();
    CMacBone    *topBone       = bone;
    CMacBone    *closestBone   =NULL;
    FLOAT       closestDistance=9999999;
    
    for ( NDword i=0; i<BoneCount; i++,bone++ )
    {   
        FCoords boneLoc;
        MeshInst->GetBoneCoords( bone, boneLoc );
        boneLoc = boneLoc.Transpose();

        FLOAT distance = (hitLocation - boneLoc.Origin).Size();

        if ( distance < closestDistance )
        {
            closestDistance = distance;
            closestBone     = bone;
        }
    }

    if ( closestBone )
    {
        hitDirection.Normalize();
        hitDirection *= 0.7;

        FRotator rot = hitDirection.Rotation();
        
        VCoords3 &myBase = closestBone->GetCoords( false );

        VAxes3 rotAxes = VAxes3( xAxis, hitDirection.X );
        myBase.r <<= rotAxes;
        rotAxes = VAxes3( zAxis, hitDirection.Y );
        myBase.r <<= rotAxes;

        closestBone->SetCoords( myBase, false );
        
        bone = closestBone;

        // Starting with this bone and moving upwards, put a fraction of the impulse onto it.
        while ( bone != topBone )
        {
            bone--;
            hitDirection *= 0.75;
            VCoords3 &myBase = bone->GetCoords( false );

            VAxes3 rotAxes = VAxes3( xAxis, hitDirection.X );
            myBase.r <<= rotAxes;
            rotAxes = VAxes3( zAxis, hitDirection.Y );
            myBase.r <<= rotAxes;

            bone->SetCoords( myBase, false );
        } 

        m_riderBoneHandle = -1; // So rope doesn't straighten out automatically
    }
    */
}

//====================================================================
//ABoneRope::GetPlayerPositionFactor
//Figure out where the player is and return a factor.
//0 is top of the rope, and 1 is bottom.
//====================================================================
void ABoneRope::execGetPlayerPositionFactor( FFrame& Stack, RESULT_DECL )
{
    P_FINISH;

    *(FLOAT*)Result = 0;
}

void ABoneRope::execDoBoneRope( FFrame& Stack, RESULT_DECL )
{
    P_GET_FLOAT(deltaTime);
    P_GET_UBOOL(action);
    P_FINISH;
    
    DoBoneRope( deltaTime, action );
}

//====================================================================
//ABoneRope::execDoBoneRope - The main thinking function of the rope. 
//This will take care of all the swinging and effects of gravity on the rope.
//====================================================================
void ABoneRope::DoBoneRope( FLOAT deltaTime, UBOOL action )
{
	
}

//====================================================================
//ABoneRope::RiderHitSolid - Rider hit a solid object.  Take all velocity
//away from the rope and let gravity return it to center.  Also don't do this
//more than once a second.
//====================================================================
void ABoneRope::RiderHitSolid
    (
    void
    )

{
    if ( m_lastHitTime + 1 > Level->TimeSeconds )
        return;

    m_lastHitTime = Level->TimeSeconds;

    // Zero the velocity and set to the old displacement so we don't get a jerk
    if ( m_Rider )
    {
        m_Rider->Velocity *= -1;
    }

    m_angularVelocity *= 0;
    m_angularDisplacement = m_oldAngularDisplacement;

    // Do the rope to update it to the new velocity    
    DoBoneRope( 1, false ); 
}

//====================================================================
//ABoneRope::CreatePrimitive
//====================================================================
UPrimitive *ABoneRope::CreatePrimitive
    (
    void
    )

{
    RopePrimitive   *rp;
	FVector			TLoc;

    InitializeRope();

    //m_Location2 = Location;
    //m_Location2.Z *= m_ropeScale - 1;
	// JEP: This code was mucking up m_Location2, when script code was setting it up instead
	TLoc = Location;
    TLoc *= m_ropeScale - 1;

    if ( !m_ropePrimitive )
    {
        UClass *Cls = RopePrimitive::StaticClass();
        rp = (RopePrimitive*)StaticConstructObject( Cls, 
                                                    GetTransientPackage(),
                                                    NAME_None,
                                                    RF_Transient,
                                                    Cls->GetDefaultObject()
                                                  );
        m_ropePrimitive = (INT)rp;
        rp->m_RopeInstance = this;
		rp->AddToRoot();  // Unreal don't delete me!
    }
    else
    {
        rp = (RopePrimitive*)m_ropePrimitive;
    }

    // Create the bounding box for the primitive
    FVector extent,pos;
    extent = FVector( CollisionRadius, CollisionRadius, m_ropeLength/2 );
    
    // Calculate center of the rope
    //pos = m_Location2 - FVector(0,0,1) * m_ropeLength * 0.5;		// JEP: Commented out
    pos = TLoc - FVector(0,0,1) * m_ropeLength * 0.5;				// JEP
    rp->BoundingBox = FBox( pos - extent, pos + extent );	
    
    return rp;
}

//====================================================================
//ABoneRope::execRecreatePrimitive
//====================================================================
void ABoneRope::execRecreatePrimitive( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

    CreatePrimitive();
}

//====================================================================
//ABoneRope::execAddRope
//====================================================================
void ABoneRope::execAddRope( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	m_nextRope = GetLevel()->GetLevelInfo()->RopeList;
	GetLevel()->GetLevelInfo()->RopeList = this;
}

//====================================================================
//ABoneRope::execRemoveRope
//====================================================================
void ABoneRope::execRemoveRope( FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	ABoneRope *next = GetLevel()->GetLevelInfo()->RopeList;
	
    if ( next == this )
    {
		GetLevel()->GetLevelInfo()->RopeList = next->m_nextRope;
    }
	else
	{
		while ( next )
		{
			if ( next->m_nextRope == this )
			{
				next->m_nextRope = m_nextRope;
				break;
			}
			next = next->m_nextRope;
		}
	}
}

//====================================================================
//ABoneRope::GetPrimitive - Returns a primitive for system collision
//====================================================================
UPrimitive *ABoneRope::GetPrimitive() const
{
    if ( m_ropePrimitive )
    {
        return (RopePrimitive *)m_ropePrimitive;
    }

    // primitive doesn't exist, so create a primitve
    RopePrimitive *rp = (RopePrimitive*)((ABoneRope*)this)->CreatePrimitive();

    if ( rp )
        return rp;

    return(GetLevel()->Engine->Cylinder);
}


//====================================================================
//RopePrimitive::PointCheck
//====================================================================
UBOOL RopePrimitive::PointCheck
    (
    FCheckResult& Result,
    AActor* Owner,
    FVector Location,
    FVector Extent,
    DWORD ExtraNodeFlags
    )
{
    return UPrimitive::PointCheck( Result,Owner,Location,Extent,ExtraNodeFlags );
}

//====================================================================
//RopePrimitive::LineCheck - This has been hacked to only use the 
//bMeshAccurate flag (when a gun is being fired).  We don't care about
//any of the other traces
//====================================================================
UBOOL RopePrimitive::LineCheck
    (
    FCheckResult& Result,
    AActor* Owner,
    FVector End,
    FVector Start,
    FVector Extent,
    DWORD ExtraNodeFlags,
    UBOOL bMeshAccurate
    )
{   
    return true;
}

//====================================================================
//RopePrimitive::GetRenderBoundingBox
//====================================================================
FBox RopePrimitive::GetRenderBoundingBox
    (
    const AActor* Owner,
    UBOOL Exact
    )

{	
#if 1
	// JEP ... New way
    // Get the mesh instance from the Rope
	UDukeMeshInstance* MeshInst = Cast<UDukeMeshInstance>( m_RopeInstance->GetMeshInstance() );

	if (!MeshInst)		// If we don't have an instance, then return the generic version (we should always have one though in practice)
		return UPrimitive::GetRenderBoundingBox( Owner, Exact );

	//UpdateRopeRenderBox(m_RopeInstance);		

	// Return the meshes bounding box
	return MeshInst->GetRenderBoundingBox(Owner, Exact);
	// ... JEP
#else
	// Old way
	return UPrimitive::GetRenderBoundingBox( Owner, Exact );
#endif
}

//====================================================================
//RopePrimitive::GetCollisionBoundingBox
//====================================================================
FBox RopePrimitive::GetCollisionBoundingBox( const AActor* Owner ) const
{	
    return UPrimitive::GetCollisionBoundingBox( Owner );
}
 
void ABoneRope::Spawned()
{}
