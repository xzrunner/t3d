#pragma once

#include <dsound.h>

namespace t3d {

#define MAX_SOUNDS     256 // max number of sounds in system at once 

// digital sound object state defines
#define SOUND_NULL     0 // " "
#define SOUND_LOADED   1
#define SOUND_PLAYING  2
#define SOUND_STOPPED  3

// directx 7.0 compatibility

#ifndef DSBCAPS_CTRLDEFAULT
#define DSBCAPS_CTRLDEFAULT (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME )
#endif

class Sound
{
public:
	Sound();

	int Init(HWND handle);
	int Shutdown();

	int LoadWAV(char *filename, int control_flags = DSBCAPS_CTRLDEFAULT);

	int ReplicateSound(int source_id);

	int Play(int id, int flags=0, int volume=0, int rate=0, int pan=0);

	int StopSound(int id);
	int StopAllSounds();

	int DeleteSound(int id);
	int DeleteAllSounds();

	int StatusSound(int id);

	int SetVolume(int id,int vol);
	int SetFreq(int id,int freq);
	int SetPan(int id,int pan);

	LPDIRECTSOUND GetDirectSound() const { return lpds; }

private:
	// this holds a single sound
	typedef struct pcm_sound_typ
	{
		LPDIRECTSOUNDBUFFER dsbuffer;   // the ds buffer containing the sound
		int state;                      // state of the sound
		int rate;                       // playback rate
		int size;                       // size of sound
		int id;                         // id number of the sound
	} pcm_sound, *pcm_sound_ptr;

private:
	LPDIRECTSOUND		lpds;			// directsound interface pointer
	DSBUFFERDESC		dsbd;           // directsound description
	DSCAPS				dscaps;         // directsound caps
	HRESULT				dsresult;       // general directsound result
	DSBCAPS				dsbcaps;        // directsound buffer caps
	LPDIRECTSOUNDBUFFER	lpdsbprimary;   // the primary mixing buffer
	pcm_sound			sound_fx[MAX_SOUNDS];    // the array of secondary sound buffers

	WAVEFORMATEX		pcmwf;          // generic waveformat structure

}; // Sound

}