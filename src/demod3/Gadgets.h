#pragma once

#include <ddraw.h>

namespace t3d { class BOB; }

namespace bspeditor {

// types of gadgets
#define GADGET_TYPE_TOGGLE             0  // toggles from off/on, on/off
#define GADGET_TYPE_MOMENTARY          1  // lights up while you press it
#define GADGET_TYPE_STATIC             2  // doesn't do anything visual

// gadget ids
#define GADGET_ID_SEGMENT_MODE_ID      0  // drawing a single segment
#define GADGET_ID_POLYLINE_MODE_ID     2  // drawing a polyline
#define GADGET_ID_DELETE_MODE_ID       4  // delete lines 
#define GADGET_ID_CLEAR_ALL_ID         6  // clear all lines
#define GADGET_ID_FLOOR_MODE_ID        8  // drawing floor tiles

#define GADGET_ID_WALL_TEXTURE_DEC     20 // previous texture
#define GADGET_ID_WALL_TEXTURE_INC     21 // next texture

#define GADGET_ID_WALL_HEIGHT_DEC      22 // decrease wall height
#define GADGET_ID_WALL_HEIGHT_INC      23 // increase wall height

#define GADGET_ID_FLOOR_TEXTURE_DEC    30 // previous floor texture
#define GADGET_ID_FLOOR_TEXTURE_INC    31 // next floor texture

#define GADGET_OFF                     0  // the gadget is off
#define GADGET_ON                      1  // the gadget is on

#define NUM_GADGETS                    11 // number of active gadget

struct Gadget
{
	int type;          // type of gadget
	int state;         // state of gadget
	int id;            // id number of gadget
	int x0, y0;        // screen position of gadget
	int width, height; // size of gadget
	t3d::BOB* bitmap;  // bitmaps of gadget
	int count;
};

class Gadgets
{
public:
	Gadgets();
	~Gadgets();
	
	void Load();

	void Draw(LPDIRECTDRAWSURFACE7 lpdds);

	int Process(int mx, int my, unsigned char mbuttons[]);

private:
	static Gadget buttons[NUM_GADGETS];

public:
	t3d::BOB* background;        // the background image
	t3d::BOB* button_img;        // holds the button images

}; // Gadgets

}