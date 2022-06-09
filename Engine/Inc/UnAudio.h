/*=============================================================================
	UnAudio.h: Unreal base audio.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
		* Wave modification code by Erik de Neve
=============================================================================*/

class USound;
class UMusic;
class UAudioSubsystem;
class DnHDSound;

/*-----------------------------------------------------------------------------
	UAudioSubsystem.
-----------------------------------------------------------------------------*/

//
// UAudioSubsystem is the abstract base class of
// the game's audio subsystem.
//
class ENGINE_API UAudioSubsystem : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(UAudioSubsystem,USubsystem,CLASS_Config)
	NO_DEFAULT_CONSTRUCTOR(UAudioSubsystem)

	// UAudioSubsystem interface.
	virtual UBOOL Init()=0;
	virtual void SetViewport( UViewport* Viewport )=0;
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog )=0;
	virtual void Update( FPointRegion Region, FCoords& Listener )=0;
	virtual void RegisterMusic( UMusic* Music )=0;
	virtual void RegisterSound( USound* Music )=0;
	virtual void UnregisterSound( USound* Sound )=0;
	virtual void UnregisterMusic( UMusic* Music )=0;
	virtual void StopSound( INT Index )=0;
	virtual void StopSoundBySlot( AActor* Actor, INT Index )=0;
#undef PlaySound
	virtual UBOOL PlaySound( AActor* const Actor, 
							 INT Id, 
							 USound* const Sound, 
							 FVector Location, 
							 FLOAT Volume, 
							 FLOAT Radius, 
							 FLOAT Pitch,
							 UBOOL IsMonitoredSound
	)=0;
	virtual UBOOL PlaySound(AActor* const Actor,
		INT Id,
		DnHDSound *Sound,
		FVector Location,
		FLOAT Volume,
		FLOAT Radius,
		FLOAT Pitch,
		UBOOL IsMonitoredSound
	) = 0;

	virtual bool IsActorPlayingAudio(const AActor* Actor) = 0;

	virtual void NoteDestroy( AActor* Actor )=0;
	virtual UBOOL GetLowQualitySetting()=0;
	virtual UViewport* GetViewport()=0;
	virtual void RenderAudioGeometry( FSceneNode* Frame )=0;
	virtual void PostRender( FSceneNode* Frame )=0;

	virtual void MusicPlay(TCHAR *Prefix,TCHAR *Filename,UBOOL Instant=0,FLOAT CrossfadeTime=0.0f,UBOOL Push=0)=0;
};

/*-----------------------------------------------------------------------------
	USound.
-----------------------------------------------------------------------------*/

//
// Sound data.
//
class ENGINE_API FSoundData : public TLazyArray<BYTE>
{
public:
	USound* Owner;
	void Load();
	FLOAT GetPeriod();
	FSoundData( USound* InOwner )
	: Owner( InOwner )
	{}
};

class DnHDSound {
public:
	DnHDSound(const wchar_t* name)
	{
		this->name = name;
	}

	const TCHAR* GetName() const
	{
		return name;
	}

	void* handle;

private:
	const wchar_t* name;
};

//
// A sound effect.
//
class ENGINE_API USound : public UObject
{
	DECLARE_CLASS(USound,UObject,CLASS_SafeReplace)

	// Variables.
	FSoundData	Data;
	FName		FileType;
	INT			OriginalSize;
	FLOAT       Duration;
	void*		Handle;
	static UAudioSubsystem* Audio;

	// Constructor.
	USound()
	: Data( this )
	{
		Duration = -1.f;
	}

	// Duration.
	FLOAT GetDuration();

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();
	void PostLoad();
};

// CDH...
// Sound with voice information
class ENGINE_API UVoiceSound : public USound
{
	DECLARE_CLASS(UVoiceSound,USound,CLASS_SafeReplace)

	// Variables.
	FName MouthFrameSequence;
	INT MouthFrameLookup[1024]; // lip sync mouth frames at 30 FPS (about 34 seconds worth)
};
// ...CDH

/*-----------------------------------------------------------------------------
	UMusic.
-----------------------------------------------------------------------------*/

//
// A song.
//
class ENGINE_API UMusic : public UObject
{
	DECLARE_CLASS(UMusic,UObject,CLASS_SafeReplace)

	// Variables.
	TLazyArray<BYTE>	Data;
	FName				FileType;
	INT					OriginalSize;
	void*				Handle;
	static UAudioSubsystem* Audio;

	// Constructor.
	UMusic()
	{}

	// UObject implementation.
	void Serialize( FArchive& Ar );
	void Destroy();
	void PostLoad();
};

/*-----------------------------------------------------------------------------
	FWaveModInfo. 
-----------------------------------------------------------------------------*/

//  Macros to convert 4 bytes to a Riff-style ID DWORD.
//  Todo: make these endian independent !!!

#define MAKEFOURCC(ch0, ch1, ch2, ch3)\
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |\
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define mmioFOURCC(ch0, ch1, ch2, ch3)\
    MAKEFOURCC(ch0, ch1, ch2, ch3)

// Main Riff-Wave header.
struct FRiffWaveHeader
{ 
	DWORD	rID;			// Contains 'RIFF'
	DWORD	ChunkLen;		// Remaining length of the entire riff chunk (= file).
	DWORD	wID;			// Form type. Contains 'WAVE' for .wav files.
};

// General chunk header format.
struct FRiffChunkOld
{
	DWORD	ChunkID;		  // General data chunk ID like 'data', or 'fmt ' 
	DWORD	ChunkLen;		  // Length of the rest of this chunk in bytes.
};

// ChunkID: 'fmt ' ("WaveFormatEx" structure ) 
struct FFormatChunk
{
    _WORD   wFormatTag;        // Format type: 1 = PCM
    _WORD   nChannels;         // Number of channels (i.e. mono, stereo...).
    DWORD   nSamplesPerSec;    // Sample rate. 44100 or 22050 or 11025  Hz.
    DWORD   nAvgBytesPerSec;   // For buffer estimation  = sample rate * BlockAlign.
    _WORD   nBlockAlign;       // Block size of data = Channels times BYTES per sample.
    _WORD   wBitsPerSample;    // Number of bits per sample of mono data.
    _WORD   cbSize;            // The count in bytes of the size of extra information (after cbSize).
};

// ChunkID: 'smpl'
struct FSampleChunk
{
	DWORD   dwManufacturer;
	DWORD   dwProduct;
	DWORD   dwSamplePeriod;
	DWORD   dwMIDIUnityNote;
	DWORD   dwMIDIPitchFraction;
	DWORD	dwSMPTEFormat;		
	DWORD   dwSMPTEOffset;		//
	DWORD   cSampleLoops;		// Number of tSampleLoop structures following this chunk
	DWORD   cbSamplerData;		// 
};
 
struct FSampleLoop				// Immediately following cbSamplerData in the SMPL chunk.
{
	DWORD	dwIdentifier;		//
	DWORD	dwType;				//
	DWORD	dwStart;			// Startpoint of the loop in samples
	DWORD	dwEnd;				// Endpoint of the loop in samples
	DWORD	dwFraction;			// Fractional sample adjustment
	DWORD	dwPlayCount;		// Play count
};

//
// Structure for in-memory interpretation and modification of WAVE sound structures.
//
class ENGINE_API FWaveModInfo
{
public:

	// Pointers to variables in the in-memory WAVE file.
	DWORD* pSamplesPerSec;
	DWORD* pAvgBytesPerSec;
	_WORD* pBlockAlign;
	_WORD* pBitsPerSample;
	_WORD* pChannels;

	DWORD  OldBitsPerSample;

	DWORD* pWaveDataSize;
	DWORD* pMasterSize;
	BYTE*  pSampleDataStart;
	BYTE*  pSampleDataEnd;
	BYTE*  pWaveDataEnd;
	DWORD  SampleDataSize;

	INT	   SampleLoopsNum;
	FSampleLoop*  pSampleLoop;

	DWORD  NewDataSize;
	UBOOL  NoiseGate;

	// Constructor.
	FWaveModInfo() : pSamplesPerSec(NULL),
					 pAvgBytesPerSec(NULL),
					 pBlockAlign(NULL),
					 pBitsPerSample(NULL),
					 pChannels(NULL),

					 OldBitsPerSample(0),

					 pWaveDataSize(NULL),
					 pMasterSize(NULL),
					 pSampleDataStart(NULL),
					 pSampleDataEnd(NULL),
					 pWaveDataEnd(NULL),
					 SampleDataSize(0),
					 SampleLoopsNum(0),
					 pSampleLoop(NULL),
					 NewDataSize(0),
					 NoiseGate(0)		//FALSE)
	{	}
	
	// 16-bit padding.
	DWORD Pad16Bit( DWORD InDW )
	{
		return ((InDW + 1)& ~1);
	}

	// Read headers and load all info pointers in WaveModInfo. 
	// Returns 0 if invalid data encountered.
	// UBOOL ReadWaveInfo( TArray<BYTE>& WavData );
	UBOOL ReadWaveInfo( TArray<BYTE>& WavData );
	
	// Handle RESIZING and updating of all variables needed for the new size:
	// notably the (possibly multiple) loop structures.
	UBOOL UpdateWaveData( TArray<BYTE>& WavData);

	// Wave size and/or bitdepth reduction.
	void Reduce16to8();
	void HalveData();
	void HalveReduce16to8(); 

	// Filters.
	void NoiseGateFilter(); 

private:
	UBOOL FindChunk( const FRiffChunkOld*& pRiffChunk, const BYTE cChunkToFind[4]);
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
