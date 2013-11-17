#pragma once

#include <ddraw.h>

#include "defines.h"

namespace t3d {

// defines for BOBs
#define BOB_STATE_DEAD         0    // this is a dead bob
#define BOB_STATE_ALIVE        1    // this is a live bob
#define BOB_STATE_DYING        2    // this bob is dying
#define BOB_STATE_ANIM_DONE    1    // done animation state
#define MAX_BOB_FRAMES         64   // maximum number of bob frames
#define MAX_BOB_ANIMATIONS     16   // maximum number of animation sequeces

#define BOB_ATTR_SINGLE_FRAME   1   // bob has single frame
#define BOB_ATTR_MULTI_FRAME    2   // bob has multiple frames
#define BOB_ATTR_MULTI_ANIM     4   // bob has multiple animations
#define BOB_ATTR_ANIM_ONE_SHOT  8   // bob will perform the animation once
#define BOB_ATTR_VISIBLE        16  // bob is visible
#define BOB_ATTR_BOUNCE         32  // bob bounces off edges
#define BOB_ATTR_WRAPAROUND     64  // bob wraps around edges
#define BOB_ATTR_LOADED         128 // the bob has been loaded
#define BOB_ATTR_CLONE          256 // the bob is a clone

class BmpFile;

// the blitter object structure BOB
class BOB
{
public:
	BOB() { memset(this, 0, sizeof(BOB)); }

	int Create(int x, int y,          // initial posiiton
			   int width, int height, // size of bob
			   int num_frames,        // number of frames
			   int attr,              // attrs
			   int mem_flags,         // memory flags in DD format
			   unsigned int color_key_value, // default color key
			   int bpp);                // bits per pixel

	int LoadFrame(BmpFile* bitmap, // bitmap to scan image data from
				  int frame,       // frame to load
				  int cx,int cy,   // cell or absolute pos. to scan image from
				  int mode);       // if 0 then cx,cy is cell position, else 
								   // cx,cy are absolute coords

	int Draw(LPDIRECTDRAWSURFACE7 dest);
	int DrawScaled(int swidth, int sheight, LPDIRECTDRAWSURFACE7 dest);

	LPDIRECTDRAWSURFACE7 GetImage() {
		return images[curr_frame];
	}

	void Step() {
		if (++curr_frame >= num_frames)
			curr_frame = 0;
	}

	void SetCurrFrame(int frame) {
		curr_frame = frame;
	}

	void SetPosition(float x, float y) {
		this->x = x;
		this->y = y;
	}

	void SetAttrBit(int bit) { SET_BIT(attr, bit); }

private:
	int state;          // the state of the object (general)
	int anim_state;     // an animation state variable, up to you
	int attr;           // attributes pertaining to the object (general)
	float x,y;            // position bitmap will be displayed at
	float xv,yv;          // velocity of object
	int width, height;  // the width and height of the bob
	int width_fill;     // internal, used to force 8*x wide surfaces
	int bpp;            // bits per pixel
	int counter_1;      // general counters
	int counter_2;
	int max_count_1;    // general threshold values;
	int max_count_2;
	int varsI[16];      // stack of 16 integers
	float varsF[16];    // stack of 16 floats
	int curr_frame;     // current animation frame
	int num_frames;     // total number of animation frames
	int curr_animation; // index of current animation
	int anim_counter;   // used to time animation transitions
	int anim_index;     // animation element index
	int anim_count_max; // number of cycles before animation
	int *animations[MAX_BOB_ANIMATIONS]; // animation sequences

	LPDIRECTDRAWSURFACE7 images[MAX_BOB_FRAMES]; // the bitmap images DD surfaces

	friend class BitmapFont;

}; // BOB

}