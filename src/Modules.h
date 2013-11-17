#pragma once

namespace t3d {

class Log;
class Timer;
class Input;
class Sound;
class Music;
class Graphics;
class Image;

// conditionally compilation for engine stats
extern int debug_total_polys_per_frame;
extern int debug_polys_lit_per_frame;
extern int debug_polys_clipped_per_frame;
extern int debug_polys_rendered_per_frame;
extern int debug_backfaces_removed_per_frame;
extern int debug_objects_removed_per_frame;
extern int debug_total_transformations_per_frame;

extern int bhv_nodes_visited;

class Modules
{
public:
	static bool Init();
	static void Release();

	static Log& GetLog() { return *_log; }
	static Timer& GetTimer() { return *_timer; }
	static Input& GetInput() { return *_input; }
	static Sound& GetSound() { return *_sound; }
//	static Music& GetMusic() { return *_music; }
	static Graphics& GetGraphics() { return *_graphics; }
	static Image& GetImage() { return *_image; }

private:
	static Log* _log;
	static Timer* _timer;
	static Input* _input;
	static Sound* _sound;
//	static Music* _music;
	static Graphics* _graphics;
	static Image* _image;

}; // Modules

}