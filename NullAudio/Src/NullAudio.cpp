#include <Engine.h>

class DLL_EXPORT UNullAudioSubsystem : UAudioSubsystem
{
	DECLARE_CLASS(UNullAudioSubsystem, UAudioSubsystem, CLASS_Config)

private:
	UViewport* Viewport;

public:
	UBOOL Init()
	{
		return TRUE;
	}

	void SetViewport(UViewport* Viewport)
	{
		this->Viewport = Viewport;
	}

	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar)
	{
		return FALSE;
	}

	void Update(FPointRegion Region, FCoords& Listener)
	{
	}

	void RegisterMusic(UMusic* Music)
	{
	}

	void RegisterSound(USound* Sound)
	{
	}

	void UnregisterSound(USound* Sound)
	{
	}

	void UnregisterMusic(UMusic* Music)
	{
	}

	void StopSound(INT Index)
	{
	}

	void StopSoundBySlot(AActor* Actor, INT Index)
	{
	}

	UBOOL PlaySound(AActor* const Actor, INT Id, USound* const Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch, UBOOL IsMonitoredSound)
	{
		return FALSE;
	}

	void NoteDestroy(AActor* Actor)
	{
	}

	UBOOL GetLowQualitySetting()
	{
		return FALSE;
	}

	UViewport* GetViewport()
	{
		return Viewport;
	}

	void RenderAudioGeometry(FSceneNode* Frame)
	{
	}

	void PostRender(FSceneNode* Frame)
	{
	}

	void MusicPlay(TCHAR* Prefix, TCHAR* Filename, UBOOL Instant, FLOAT CrossfadeTime, UBOOL Push)
	{
	}
};

/*-----------------------------------------------------------------------------
	Class and package registration.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UNullAudioSubsystem);
IMPLEMENT_PACKAGE(NullAudio);
