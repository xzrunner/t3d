#include "Modules.h"

#include "Log.h"
#include "Timer.h"
#include "Input.h"
#include "Sound.h"
#include "Music.h"
#include "Graphics.h"
#include "Image.h"

namespace t3d {

Log* Modules::_log = NULL;
Timer* Modules::_timer = NULL;
Input* Modules::_input = NULL;
Sound* Modules::_sound = NULL;
//Music* Modules::_music = NULL;
Graphics* Modules::_graphics = NULL;
Image* Modules::_image = NULL;

bool Modules::Init()
{
	if (_log == NULL) _log = new Log();
	if (_timer == NULL) _timer = new Timer();
	if (_input == NULL) _input = new Input();
	if (_sound == NULL) _sound = new Sound();
//	if (_music == NULL) _music = new Music();
	if (_graphics == NULL) _graphics = new Graphics();
	if (_image == NULL) _image = new Image();

	return true;
}

void Modules::Release()
{
	delete _image, _image = NULL;
	delete _graphics, _graphics = NULL;
//	delete _music, _music = NULL;
	delete _sound, _sound = NULL;
	delete _input, _input = NULL;
	delete _timer, _timer = NULL;
	delete _log, _log = NULL;
}

}