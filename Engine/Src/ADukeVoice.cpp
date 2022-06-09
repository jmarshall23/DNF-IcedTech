/*=============================================================================
	ADukeVoice.cpp

	IceColdDuke - Port of DukeVoice.uc to C++
=============================================================================*/
#include "EnginePrivate.h"
#include "dnClient.h"		/* Dukenet client header. */

/*-----------------------------------------------------------------------------
	ADukeNet object implementation.
-----------------------------------------------------------------------------*/
IMPLEMENT_CLASS(ADukeVoice);

void ADukeVoice::execDukeSay(FFrame& Stack, RESULT_DECL)
{
	P_GET_OBJECT(USound, Phrase);
	P_FINISH

	float Pitch;	

	if (Owner->DrawScale < 0.5)
		Pitch = 1.5;
	else
		Pitch = 1.0;

	if (Level->NetMode == NM_Standalone)
	{
		eventPlayOwnedSound(Phrase, SLOT_Talk, TransientSoundVolume, 0, TransientSoundRadius, Pitch, true, false);
		eventPlayOwnedSound(Phrase, SLOT_Ambient, TransientSoundVolume, 0, TransientSoundRadius, Pitch, true, false);
		eventPlayOwnedSound(Phrase, SLOT_Interface, TransientSoundVolume, 0, TransientSoundRadius, Pitch, true, false);
	}
	else
	{
		eventPlaySound(Phrase, SLOT_Interface, TransientSoundVolume, 0, TransientSoundRadius, Pitch, true);
	}
}