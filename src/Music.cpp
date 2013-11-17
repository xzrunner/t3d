//#include "Music.h"
//
//#include <direct.h>
//
//#include "Modules.h"
//#include "Sound.h"
//
//namespace t3d {
//
//// Convert from multibyte format to Unicode using the following macro:
//#define MULTI_TO_WIDE( x,y )  MultiByteToWideChar( CP_ACP,MB_PRECOMPOSED, y,-1,x,_MAX_PATH);
//
//// initializes a direct draw struct
//#define DD_INIT_STRUCT(ddstruct) { memset(&ddstruct,0,sizeof(ddstruct)); ddstruct.dwSize=sizeof(ddstruct); }
//
//Music::Music()
//	: dm_perf(NULL)
//	, dm_loader(NULL)
//	, dm_active_id(-1)
//{
//}
//
//int Music::Init(HWND handle)
//{
// 	// this function initializes directmusic, it also checks if directsound has
// 	// been initialized, if so it connect the wave output to directsound, otherwise
// 	// it creates it's own directsound object, hence you must start directsound up
// 	// first if you want to use both directsound and directmusic
// 
// 	int index; // looping var
// 
// 	// set up directmusic
// 	// initialize COM
// 	if (FAILED(CoInitialize(NULL)))
// 	{    
// 		// Terminate the application.
// 		return(0);
// 	}
// 
// 	// create the performance
// 	if (FAILED(CoCreateInstance(CLSID_DirectMusicPerformance,
// 		NULL,
// 		CLSCTX_INPROC,
// 		IID_IDirectMusicPerformance,
// 		(void**)&dm_perf)))    
// 	{   
// 		return(0);
// 	}
// 
// 	// initialize the performance, check if directsound is on-line if so, use the
// 	// directsound object, otherwise create a new one
// 	LPDIRECTSOUND lpds = Modules::GetSound().GetDirectSound();
// 	if (FAILED(dm_perf->Init(NULL, lpds, handle)))
// 	{
// 		return(0);// Failure -- performance not initialized
// 	} // end if 
// 
// 	// add the port to the performance
// 	if (FAILED(dm_perf->AddPort(NULL)))
// 	{    
// 		return(0);// Failure -- port not initialized
// 	} // end if
// 
// 	// create the loader to load object(s) such as midi file
// 	if (FAILED(CoCreateInstance(
// 		CLSID_DirectMusicLoader,
// 		NULL,
// 		CLSCTX_INPROC, 
// 		IID_IDirectMusicLoader,
// 		(void**)&dm_loader)))
// 	{
// 		// error
// 		return(0);
// 	}
// 
// 	// reset all the midi segment objects
// 	for (index = 0; index < DM_NUM_SEGMENTS; index++)
// 	{
// 		// reset the object
// 		dm_midi[index].dm_segment  = NULL;  
// 		dm_midi[index].dm_segstate = NULL;  
// 		dm_midi[index].state       = MIDI_NULL;
// 		dm_midi[index].id          = index;
// 	}
// 
// 	// reset the active id
// 	dm_active_id = -1;
//
//	return(1);
//}
//
//int Music::Shutdown()
//{
//	int index;
//
//	// If there is any music playing, stop it. This is 
//	// not really necessary, because the music will stop when
//	// the instruments are unloaded or the performance is    
//	// closed down.
//	if (dm_perf)
//		dm_perf->Stop(NULL, NULL, 0, 0 ); 
//
//	// delete all the midis if they already haven't been
//	DeleteAllMIDI();
//
//	// CloseDown and Release the performance object.    
//	if (dm_perf)
//	{
//		dm_perf->CloseDown();
//		dm_perf->Release();     
//	}
//
//	// Release the loader object.
//	if (dm_loader)
//		dm_loader->Release();     
//
//	// Release COM
//	CoUninitialize(); 
//
//	return(1);
//}
//
//int Music::LoadMIDI(char *filename)
//{
// 	// this function loads a midi segment
// 
// 	DMUS_OBJECTDESC ObjDesc; 
// 	HRESULT hr;
// 	IDirectMusicSegment* pSegment = NULL;
// 
// 	int index; // loop var
// 
// 	// look for open slot for midi segment
// 	int id = -1;
// 
// 	for (index = 0; index < DM_NUM_SEGMENTS; index++)
// 	{
// 		// is this one open
// 		if (dm_midi[index].state == MIDI_NULL)
// 		{
// 			// validate id, but don't validate object until loaded
// 			id = index;
// 			break;
// 		}
// 	}
// 
// 	// found good id?
// 	if (id==-1)
// 		return(-1);
// 
// 	// get current working directory
// 	char szDir[_MAX_PATH];
// 	WCHAR wszDir[_MAX_PATH]; 
// 
// 	if(_getcwd( szDir, _MAX_PATH ) == NULL)
// 	{
// 		return(-1);;
// 	}
// 
// 	MULTI_TO_WIDE(wszDir, szDir);
// 
// 	// tell the loader were to look for files
// 	hr = dm_loader->SetSearchDirectory(GUID_DirectMusicAllTypes,wszDir, FALSE);
// 
// 	if (FAILED(hr)) 
// 	{
// 		return (-1);
// 	}
// 
// 	// convert filename to wide string
// 	WCHAR wfilename[_MAX_PATH]; 
// 	MULTI_TO_WIDE(wfilename, filename);
// 
// 	// setup object description
// 	DD_INIT_STRUCT(ObjDesc);
// 	ObjDesc.guidClass = CLSID_DirectMusicSegment;
// 	wcscpy(ObjDesc.wszFileName, wfilename );
// 	ObjDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
// 
// 	// load the object and query it for the IDirectMusicSegment interface
// 	// This is done in a single call to IDirectMusicLoader::GetObject
// 	// note that loading the object also initializes the tracks and does 
// 	// everything else necessary to get the MIDI data ready for playback.
// 
// 	hr = dm_loader->GetObject(&ObjDesc,IID_IDirectMusicSegment, (void**) &pSegment);
// 
// 	if (FAILED(hr))
// 		return(-1);
// 
// 	// ensure that the segment plays as a standard MIDI file
// 	// you now need to set a parameter on the band track
// 	// Use the IDirectMusicSegment::SetParam method and let 
// 	// DirectMusic find the trackby passing -1 (or 0xFFFFFFFF) in the dwGroupBits method parameter.
// 
// 	hr = pSegment->SetParam(GUID_StandardMIDIFile,-1, 0, 0, (void*)dm_perf);
// 
// 	if (FAILED(hr))
// 		return(-1);
// 
// 	// This step is necessary because DirectMusic handles program changes and 
// 	// bank selects differently for standard MIDI files than it does for MIDI 
// 	// content authored specifically for DirectMusic. 
// 	// The GUID_StandardMIDIFile parameter must be set before the instruments are downloaded. 
// 
// 	// The next step is to download the instruments. 
// 	// This is necessary even for playing a simple MIDI file 
// 	// because the default software synthesizer needs the DLS data 
// 	// for the General MIDI instrument set
// 	// If you skip this step, the MIDI file will play silently.
// 	// Again, you call SetParam on the segment, this time specifying the GUID_Download parameter:
// 
// 	hr = pSegment->SetParam(GUID_Download, -1, 0, 0, (void*)dm_perf);
// 
// 	if (FAILED(hr))
// 		return(-1);
// 
// 	// at this point we have MIDI loaded and a valid object
// 
// 	dm_midi[id].dm_segment  = pSegment;
// 	dm_midi[id].dm_segstate = NULL;
// 	dm_midi[id].state       = MIDI_LOADED;
// 
// 	return(id);
//}
//
//
//int Music::DeleteMIDI(int id)
//{
// 	// this function deletes one MIDI segment
// 
// 	// Unload instruments this will cause silence.
// 	// CloseDown unloads all instruments, so this call is also not 
// 	// strictly necessary.
// 	if (dm_midi[id].dm_segment)
// 	{
// 		dm_midi[id].dm_segment->SetParam(GUID_Unload, -1, 0, 0, (void*)dm_perf); 
// 
// 		// Release the segment and set to null
// 		dm_midi[id].dm_segment->Release(); 
// 		dm_midi[id].dm_segment  = NULL;
// 		dm_midi[id].dm_segstate = NULL;
// 		dm_midi[id].state       = MIDI_NULL;
// 	}
//
//	return(1);
//}
//
//int Music::DeleteAllMIDI()
//{
// 	// delete all the MIDI 
// 	int index; // loop var
// 
// 	// free up all the segments
// 	for (index = 0; index < DM_NUM_SEGMENTS; index++)
// 	{
// 		// Unload instruments this will cause silence.
// 		// CloseDown unloads all instruments, so this call is also not 
// 		// strictly necessary.
// 		if (dm_midi[index].dm_segment)
// 		{
// 			dm_midi[index].dm_segment->SetParam(GUID_Unload, -1, 0, 0, (void*)dm_perf); 
// 
// 			// Release the segment and set to null
// 			dm_midi[index].dm_segment->Release(); 
// 			dm_midi[index].dm_segment  = NULL;
// 			dm_midi[index].dm_segstate = NULL;
// 			dm_midi[index].state       = MIDI_NULL;
// 		}
// 	}
//
//	return(1);
//}
//
//int Music::StatusMIDI(int id)
//{
//	// this checks the status of a midi segment
//
//	if (dm_midi[id].dm_segment && dm_midi[id].state !=MIDI_NULL )
//	{
//		// get the status and translate to our defines
//		if (dm_perf->IsPlaying(dm_midi[id].dm_segment,NULL) == S_OK) 
//			dm_midi[id].state = MIDI_PLAYING;
//		else
//			dm_midi[id].state = MIDI_STOPPED;
//
//		return(dm_midi[id].state);
//	}
//	else
//		return(0);
//}
//
//int Music::Play(int id)
//{
//	// play sound based on id
//
//	if (dm_midi[id].dm_segment && dm_midi[id].state!=MIDI_NULL)
//	{
//		// if there is an active midi then stop it
//		if (dm_active_id!=-1)
//			Stop(dm_active_id);
//
//		// play segment and force tracking of state variable
//		dm_perf->PlaySegment(dm_midi[id].dm_segment, 0, 0, &dm_midi[id].dm_segstate);
//		dm_midi[id].state = MIDI_PLAYING;
//
//		// set the active midi segment
//		dm_active_id = id;
//		return(1);
//	}
//	else
//		return(0);
//}
//
//int Music::Stop(int id)
//{
//	// stop a midi segment
//	if (dm_midi[id].dm_segment && dm_midi[id].state!=MIDI_NULL)
//	{
//		// play segment and force tracking of state variable
//		dm_perf->Stop(dm_midi[id].dm_segment, NULL, 0, 0);
//		dm_midi[id].state = MIDI_STOPPED;
//
//		// reset active id
//		dm_active_id = -1;
//
//		return(1);
//	}
//	else
//		return(0);
//}
//
//}