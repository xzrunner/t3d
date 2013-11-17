//#pragma once
//
//#include <dmusici.h>
//
//namespace t3d {
//
//#define DM_NUM_SEGMENTS 64 // number of midi segments that can be cached in memory
//
//// midi object state defines
//#define MIDI_NULL     0   // this midi object is not loaded
//#define MIDI_LOADED   1   // this midi object is loaded
//#define MIDI_PLAYING  2   // this midi object is loaded and playing
//#define MIDI_STOPPED  3   // this midi object is loaded, but stopped
//
//class Music
//{
//public:
//	Music();
//
//	int Init(HWND handle);
//	int Shutdown();
//
//	int LoadMIDI(char *filename);
//	int DeleteMIDI(int id);
//	int DeleteAllMIDI();
//	int StatusMIDI(int id);
//
//	int Play(int id);
//	int Stop(int id);
//
//private:
//	// directmusic MIDI segment
//	typedef struct DMUSIC_MIDI_TYP
//	{
//		IDirectMusicSegment        *dm_segment;  // the directmusic segment
//		IDirectMusicSegmentState   *dm_segstate; // the state of the segment
//		int                        id;           // the id of this segment               
//		int                        state;        // state of midi song
//
//	} DMUSIC_MIDI, *DMUSIC_MIDI_PTR;
//
//private:
//	// direct music globals
//	IDirectMusicPerformance    *dm_perf;		// the directmusic performance manager 
//	IDirectMusicLoader         *dm_loader;		// the directmusic loader
//
//	// this hold all the directmusic midi objects
//	DMUSIC_MIDI                dm_midi[DM_NUM_SEGMENTS];
//	int dm_active_id;		// currently active midi segment
//
//}; // Music
//
//}