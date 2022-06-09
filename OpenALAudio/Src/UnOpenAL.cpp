/*
=================================================

UnOpenAL.cpp

Duke Nukem Forever OpenAL implementation.

Rev 1: Justin Marshall(icecoldduke) - generic playback for external ogg file support.

=================================================
*/

#include <Engine.h>
#include <al/al.h>
#include <al/alc.h>
#include <vorbis/vorbisfile.h>

#define __PLACEMENT_NEW_INLINE
#include <string>
#include <vector>

#define DN_MAX_AUDIO_CHANNELS 32
#define DN_MAX_MUSIC_CHANNELS 2

bool UnStringReplace(std::wstring& str, const std::wstring& from, const std::wstring& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::wstring::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

class FOpenALSound
{
public:
	FOpenALSound(const wchar_t* Filename)
	{
		short* pcmout = 0;
		soundHandle = -1;
		OggVorbis_File vf;

		FILE* fp = _wfopen(Filename, TEXT("rb"));
		if (!fp)
		{
			debugf(NAME_Init, TEXT("Failed to open sound file"));
			return;
		}

		if (ov_open_callbacks(fp, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
			debugf(NAME_Init, TEXT("Failed to read sound file!"));
			return;
		}

		vorbis_info* vi;
		vi = ov_info(&vf, -1);
		ALenum format = vi->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

		size_t data_len = ov_pcm_total(&vf, -1) * vi->channels * 2;
		pcmout = (short*)malloc(data_len);

		for (size_t size = 0, offset = 0, sel = 0;
			(size = ov_read(&vf, (char*)pcmout + offset, 4096, 0, 2, 1, (int*)&sel)) != 0;
			offset += size)
		{
			if (size < 0)
			{
				// ERROR HERE!
				return;
			}
		}
		alGenBuffers(1, &soundHandle);
		alBufferData(soundHandle, format, pcmout, data_len, vi->rate);

		ov_clear(&vf);

		path = Filename;
	}

	ALuint GetSoundHandle() const
	{
		return soundHandle;
	}

	bool IsValid() const
	{
		return soundHandle != -1;
	}

	std::wstring GetName()
	{
		return path;
	}
private:
	ALuint soundHandle;
	std::wstring path;
};

class FOpenALAudioChannel
{
public:
	bool IsPlaying()
	{
		int state = 0;
		alGetSourcei(handle, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}

	void PlaySound(const FOpenALSound *sound, const AActor *actor = nullptr)
	{
		alSourcei(handle, AL_BUFFER, sound->GetSoundHandle());		
		currentPlayingSound = sound;
		this->actor = actor;

		if (actor != nullptr)
		{
			FVector Location,
				Velocity;

			// See file header for coordinate system explanation.
			// JUSTIN: !!I WANTTED THIS IS UPDATE BUT ACTOR GETS FREED SOMETIMETS WHILE WE ARE PLAYING SOUNDS!!
			Location.X = actor->Location.X;
			Location.Y = actor->Location.Z; // Z/Y swapped on purpose, see file header
			Location.Z = actor->Location.Y; // Z/Y swapped on purpose, see file header

			alSourcefv(handle, AL_POSITION, (ALfloat*)&Location);
			alSourcei(handle, AL_SOURCE_RELATIVE, AL_TRUE);
		}
		else
		{
			alSourcei(handle, AL_SOURCE_RELATIVE, AL_FALSE);
		}

		alSourcePlay(handle);
	}

	void Update()
	{
		if (actor == nullptr)
			return;

		
	}

	void Init()
	{
		actor = nullptr;
		alGenSources(1, &handle);
	}

	void SetLooping(bool looping)
	{
		alSourcei(handle, AL_LOOPING, looping);
	}

	void Stop()
	{
		actor = nullptr;
		alSourceStop(handle);
	}

	const AActor* GetOwnerActor()
	{
		if (!IsPlaying())
		{
			return nullptr;
		}

		return actor;
	}

	const FOpenALSound* GetCurrentPlayingSound()
	{
		if (!IsPlaying())
		{
			return nullptr;
		}
		return currentPlayingSound;
	}

private:
	ALuint handle;
	const AActor* actor;
	const FOpenALSound* currentPlayingSound;
};

class DLL_EXPORT UOpenALAudioSubsystem : UAudioSubsystem
{
	DECLARE_CLASS(UOpenALAudioSubsystem, UAudioSubsystem, CLASS_Config)

private:
	UViewport* Viewport;
	ALCdevice* device;
	ALCcontext* context;

	FOpenALAudioChannel soundChannels[DN_MAX_AUDIO_CHANNELS];
	FOpenALAudioChannel musicChannel[2];

	std::vector<FOpenALSound*> queuedMusic;
	std::vector<FOpenALSound *> loadedSounds;
public:
	UBOOL Init()
	{
		device = alcOpenDevice(nullptr);
		if (!device) {
			debugf(NAME_Init, TEXT("OpenAL open device failed"));
			return false;
		}

		context = alcCreateContext(device, NULL);
		if (!context) {
			debugf(NAME_Init, TEXT("alcCreateContext failed"));
			return false;
		}

		if (!alcMakeContextCurrent(context)) {
			debugf(NAME_Init, TEXT("alcMakeContextCurrent failed"));
			return false;
		}

		debugf(NAME_Init, TEXT("OpenAL initialized"));
		
		for (int i = 0; i < DN_MAX_AUDIO_CHANNELS; i++)
			soundChannels[i].Init();

		for (int i = 0; i < DN_MAX_MUSIC_CHANNELS; i++)
		{
			musicChannel[i].Init();
			musicChannel[i].SetLooping(true);
		}

		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
		
		queuedMusic.clear();

		return true;
	}

	void StopAllMusic()
	{
		for (int i = 0; i < DN_MAX_MUSIC_CHANNELS; i++)
		{
			musicChannel[i].Stop();
		}

		queuedMusic.clear();
	}

	void PlayNewMusicChannel()
	{
		if (queuedMusic.size() == 0)
			return;

		for (int d = 0; d < queuedMusic.size(); d++)
		{
			bool alreadyPlaying = false;

			// Check to see if this is already playing.
			for (int i = 0; i < DN_MAX_MUSIC_CHANNELS; i++)
			{
				// If this is already playing dont play it again!
				if (musicChannel[i].GetCurrentPlayingSound() == queuedMusic[d])
				{
					musicChannel[i].Stop();
					musicChannel[i].PlaySound(queuedMusic[d]);
					alreadyPlaying = true;
					break;
				}
			}

			if (!alreadyPlaying)
			{
				for (int i = 0; i < DN_MAX_MUSIC_CHANNELS; i++)
				{
					if (!musicChannel[i].IsPlaying())
					{
						musicChannel[i].PlaySound(queuedMusic[d]);
						break;
					}
				}
			}			
		}
		

		queuedMusic.clear();
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
		PlayNewMusicChannel();

		for (int i = 0; i < DN_MAX_AUDIO_CHANNELS; i++)
			soundChannels[i].Update();

		// Set Player position and orientation.
		FVector Orientation[2];

		// See file header for coordinate system explanation.
		Orientation[0].X = Listener.ZAxis.X;
		Orientation[0].Y = Listener.ZAxis.Z; // Z/Y swapped on purpose, see file header	
		Orientation[0].Z = Listener.ZAxis.Y; // Z/Y swapped on purpose, see file header

		// See file header for coordinate system explanation.
		Orientation[1].X = Listener.XAxis.X;
		Orientation[1].Y = Listener.XAxis.Z; // Z/Y swapped on purpose, see file header
		Orientation[1].Z = Listener.XAxis.Y; // Z/Y swapped on purpose, see file header

		// Make the listener still and the sounds move relatively -- this allows 
		// us to scale the doppler effect on a per-sound basis.
		FVector Velocity = FVector(0, 0, 0),
		Location = Listener.Origin;

		// See file header for coordinate system explanation.
		Location.X = Listener.Origin.X;
		Location.Y = Listener.Origin.Z; // Z/Y swapped on purpose, see file header
		Location.Z = Listener.Origin.Y; // Z/Y swapped on purpose, see file header
		//Location *= AUDIO_DISTANCE_FACTOR;

		alListenerfv(AL_POSITION, (ALfloat*)&Location);
		alListenerfv(AL_ORIENTATION, (ALfloat*)&Orientation[0]);
		alListenerfv(AL_VELOCITY, (ALfloat*)&Velocity);
	}

	void RegisterMusic(UMusic* Music)
	{
	}

	FOpenALSound* LoadNewSound(const wchar_t* Filename)
	{
		for (int i = 0; i < loadedSounds.size(); i++)
		{
			if (loadedSounds[i]->GetName() == Filename)
			{
				return loadedSounds[i];
			}
		}

		FOpenALSound* handle = new FOpenALSound(Filename);
		if (!handle->IsValid())
		{
			delete handle;
			return nullptr;
		}

		loadedSounds.push_back(handle);
		return handle;
	}

	void RegisterSound(USound* Sound)
	{
		if (!Sound->Handle)
		{
			// Temporarily set the handle to avoid reentrance.
			Sound->Handle = (void*)-1;

			// Load the data.
			//Sound->Data.Load();
			//debugf(NAME_DevSound, TEXT("Register sound: %s (%i)"), Sound->GetName(), Sound->Data.Num());
			//check(Sound->Data.Num() > 0);

			std::wstring newFileName;

			newFileName = TEXT("../newsounds/");
			newFileName += Sound->GetName();
			newFileName += TEXT(".ogg");
			
			Sound->Handle = LoadNewSound(newFileName.c_str());

			// Scrap the source data we no longer need.
			//Sound->Data.Unload();
		}
	}

	void RegisterSound(DnHDSound * Sound)
	{
		if (!Sound->handle)
		{
			// Load the data.
			//Sound->Data.Load();
			//debugf(NAME_DevSound, TEXT("Register sound: %s (%i)"), Sound->GetName(), Sound->Data.Num());
			//check(Sound->Data.Num() > 0);

			std::wstring newFileName;

			newFileName = TEXT("../newsounds/");
			newFileName += Sound->GetName();
			newFileName += TEXT(".ogg");

			Sound->handle = LoadNewSound(newFileName.c_str());

			// Scrap the source data we no longer need.
			//Sound->Data.Unload();
		}
	}

	void UnregisterSound(USound* Sound)
	{
		// Do we really need to unregister sounds?
	}

	void UnregisterMusic(UMusic* Music)
	{
		StopAllMusic();
	}

	void StopSound(INT Index)
	{
	}

	void StopSoundBySlot(AActor* Actor, INT Index)
	{
	}

	bool IsActorPlayingAudio(const AActor* Actor)
	{
		if (Actor == nullptr)
			return false;

		for (int i = 0; i < DN_MAX_AUDIO_CHANNELS; i++)
		{
			if (soundChannels[i].GetOwnerActor() == Actor)
			{
				return true;
			}
		}

		return false;
	}

	UBOOL PlaySound(AActor* const Actor, INT Id, USound* const Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch, UBOOL IsMonitoredSound)
	{
		RegisterSound(Sound);

		if (Sound->Handle == nullptr)
			return false;

		for (int i = 0; i < DN_MAX_AUDIO_CHANNELS; i++)
		{
			if (!soundChannels[i].IsPlaying())
			{
				soundChannels[i].PlaySound((FOpenALSound*)Sound->Handle, Actor);
				return TRUE;
			}
		}

		return FALSE;
	}

	UBOOL PlaySound(AActor* const Actor, INT Id, DnHDSound* const Sound, FVector Location, FLOAT Volume, FLOAT Radius, FLOAT Pitch, UBOOL IsMonitoredSound)
	{
		RegisterSound(Sound);

		if (Sound->handle == nullptr)
			return false;

		for (int i = 0; i < DN_MAX_AUDIO_CHANNELS; i++)
		{
			if (!soundChannels[i].IsPlaying())
			{
				soundChannels[i].PlaySound((FOpenALSound*)Sound->handle, Actor);
				return TRUE;
			}
		}

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
		std::wstring newFileName;

		// An empty filename is a signal to halt all playing streams.
		if ((!Filename || !*Filename) && (!Prefix || !*Prefix))
		{
			StopAllMusic();
			queuedMusic.clear();
			return;
		}

		newFileName += TEXT("../music/");
		if (!*Filename)
		{
			newFileName += Prefix;
			newFileName += TEXT(".ogg");
		}
		else
		{			
			newFileName += Filename;

			// Fix the filename.
			UnStringReplace(newFileName, TEXT(".mp3"), TEXT(".ogg"));
		}

	 	FOpenALSound *musicSound = LoadNewSound(newFileName.c_str());
		if (musicSound == nullptr)
		{
			StopAllMusic();
			return;
		}

		// Don't queue the same track up multiple times.
		for (int i = 0; i < queuedMusic.size(); i++)
		{
			if (queuedMusic[i] == musicSound)
				return;
		}

		queuedMusic.push_back(musicSound);
	}
};

/*-----------------------------------------------------------------------------
	Class and package registration.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(UOpenALAudioSubsystem);
IMPLEMENT_PACKAGE(OpenALAudio);
