//=============================================================================
// PigCop.uc
//=============================================================================
class PigCop extends HumanNPC
	native;

#exec OBJ LOAD FILE = ..\Meshes\c_characters.dmx

var name NativeRequestedAnimation;
var string NativeRequestState;
var vector NativeRequestMoveLocation;

var vector enemyLastSeenLocation;
var bool enemyIsVisible;

var int ApproachState;

native function BeginAI();
native function bool StateSeePlayer(actor SeenPlayer);

native function TickAI(float DeltaTime);

native function StateApproachingEnemy();
native function bool StateShootEnemy();

/*
========================================
Animation List

A_IdleInactiveM16A
A_FightIdle - good for weaponless lapd idle.
A_Death_HitStomach - Good animation for death.
A_Roar - Good animation for waking up.
A_Run_ShotGun - Good animation for run and attack.
T_Gen2HandFire - Good animation for shooting.

A_Death_HitChest - Death animation for being hit from the front
A_Death_HitBack - Death animation for being hit from the back

========================================
*/

function PostBeginPlay()
{
    Super.PostBeginPlay();

    BeginAI();    
}

function Tick(float DeltaTime)
{
    TickAI(DeltaTime);

    if (Enemy != None)
    {
        if (GetStateName() != 'ShootEnemy')
        {
            if (enemyIsVisible && VSize(Enemy.Location - Location) < 512)
            {
                if (FRand() < 0.33)
                {
                    StopMoving();
                    GotoState('ShootEnemy');
                }

                if (GetStateName() == 'ApproachingEnemy' && VSize(Destination) > 0)
                {
                    if (VSize(Enemy.Location - Destination) > 34)
                    {
                        GotoState('ApproachingEnemy');
                    }
                }
            }
        }
    }

    Super.Tick(DeltaTime);
}

function bool IsShooting()
{
    return GetSequence(0) == 'A_Run_ShotGun';
}

function PlayToRunning()
{
    if (GetSequence(0) != 'A_Run_ShotGun')
    {
        PlayAllAnim('A_Run_ShotGun', , 0.16, true);
    }
}

//
// ShootEnemy
//
state ShootEnemy
{
    function BeginState()
    {
        PlayAllAnim('A_FightIdle', , 0.16, true);
    }

Begin:
    
    if (StateShootEnemy())
    {
        TurnToward(Enemy);
        PlayAllAnim('T_Gen2HandFire', , 0.1, false);
        FinishAnim(0);
        PlayAllAnim('A_FightIdle', , 0.16, true);
        Sleep(0.2 + FRand());
    }
    else
    {
        Sleep(1.0);
    }

    if (!enemyIsVisible)
    {
        GotoState('FindEnemy');
    }
    else if (VSize(Enemy.Location - Location) > 512)
    {
        GotoState('ApproachingEnemy');
    }

    Goto('Begin');
}

//
// FindEnemy
//
state FindEnemy
{
    function BeginState()
    {

    }

Begin:
    TurnTo(enemyLastSeenLocation);
    MoveTo(enemyLastSeenLocation + vect(0, 0, 64), GetRunSpeed() / 3);

    if (enemyIsVisible)
    {
        GotoState('ApproachingEnemy');
    }

    Sleep(1.0);
    Goto('Begin');
}

//
// ApproachingEnemy
//
state ApproachingEnemy
{
    function BeginState()
    {
        EnableHeadTracking(true);
        HeadTrackingActor = Enemy;

        ApproachState = 0;
        NativeRequestState = "ApproachingEnemy";
    }

    function HitWall(vector HitNormal, actor HitWall)
    {
        Focus = Destination;
        if (PickWallAdjust() && !HitWall.IsA('dnDecoration'))
            GotoState('ApproachingEnemy', 'AdjustFromWall');
        else if (!HitWall.IsA('dnDecoration'))
            MoveTimer = -1.0;
    }

Begin:
    StateApproachingEnemy();
    if (enemyIsVisible && VSize(Enemy.Location - Location) < 256)
    {
        if (FRand() < 0.33)
        {
            GotoState('ShootEnemy');
            Destination.x = 0;
            Destination.y = 0;
            Destination.z = 0;
        }
    }

    if (GetSequence(0) != NativeRequestedAnimation)
    {        
        PlayAllAnim(NativeRequestedAnimation, , 0.16, true);
    }

    if (VSize(NativeRequestMoveLocation) != 0)
    {
        Destination = NativeRequestMoveLocation;
        NativeRequestMoveLocation.x = 0;
        NativeRequestMoveLocation.y = 0;
        NativeRequestMoveLocation.z = 0;
        TurnTo(Destination);
        MoveTo(Destination + vect(0, 0, 64), GetRunSpeed() / 3);
    }
    Sleep(1.0);
    Goto('Begin');
AdjustFromWall:
    StrafeTo(Destination, Focus, GetRunSpeed());
    Goto('Begin');
}

//
// Idling State
//
auto state Idling
{
    function SeePlayer(actor SeenPlayer)
    {
        if (StateSeePlayer(SeenPlayer))
        {
            GotoState('Idling', 'Acquisition');
            Disable('SeePlayer');
            NativeRequestState = "Idling";
        }
    }

    function BeginState()
    {
    }

Acquisition:
    TurnToward(Enemy);
    PlayAllAnim('A_Roar', , 0.1, false);
    FinishAnim(0);
    PlayAllAnim('IdleA', , 0.1, true);
    GotoState('ApproachingEnemy');

Begin:
    //SetPhysics(PHYS_Flying);
    PlayAllAnim('IdleA',, 0.1, true);
}

defaultproperties
{
     EgoKillValue = 8
     Mesh = DukeMesh'c_characters.PigCop'
     WeaponInfo(0) = (WeaponClass = "dnGame.m16",PrimaryAmmoCount = 500,altAmmoCount = 50)
     WeaponInfo(1) = (WeaponClass = "")
     WeaponInfo(2) = (WeaponClass = "")
     Health = 50
     BaseEyeHeight = 27.000000
     EyeHeight = 27.000000
     GroundSpeed = 420.000000
     bIsHuman = True
     CollisionRadius = 17.000000
     CollisionHeight = 39.000000
     bAggressiveToPlayer = true
    NativeShootWaitTime = 0.0;
}
