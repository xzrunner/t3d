#include "Gadgets.h"

#include <stdlib.h>

#include "BOB.h"
#include "BmpFile.h"
#include "BmpImg.h"

namespace bspeditor {

// define all the gadgets, their type, state, bitmap index, position, etc.
Gadget Gadgets::buttons[NUM_GADGETS] = 
       { 
       {GADGET_TYPE_TOGGLE,    GADGET_ON, GADGET_ID_SEGMENT_MODE_ID,    670, 50,          116,32, NULL,0}, 
       {GADGET_TYPE_TOGGLE,    GADGET_OFF, GADGET_ID_POLYLINE_MODE_ID,  670, 50+(50-12),  116,32, NULL,0}, 
       {GADGET_TYPE_TOGGLE,    GADGET_OFF, GADGET_ID_DELETE_MODE_ID,    670, 50+(100-24), 116,32, NULL,0}, 
       {GADGET_TYPE_MOMENTARY, GADGET_OFF, GADGET_ID_CLEAR_ALL_ID,      670, 348,         116,32, NULL,0}, 
       {GADGET_TYPE_TOGGLE,    GADGET_OFF, GADGET_ID_FLOOR_MODE_ID,     670, 556,         116,32, NULL,0}, 

       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_TEXTURE_DEC,  680, 316,         16,16, NULL,0}, 
       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_TEXTURE_INC,  700, 316,         16,16, NULL,0}, 

       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_HEIGHT_DEC,   680, 208,         16,16, NULL,0}, 
       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_HEIGHT_INC,   700, 208,         16,16, NULL,0}, 

       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_FLOOR_TEXTURE_DEC, 680, 522,         16,16, NULL,0}, 
       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_FLOOR_TEXTURE_INC, 700, 522,         16,16, NULL,0}, 
       };

Gadgets::Gadgets()
{
	background = new t3d::BOB;
	button_img = new t3d::BOB;
}

Gadgets::~Gadgets()
{
	delete button_img;
	delete background;
}

void Gadgets::Load()
{
	// this function simply loads all the gadget images into the global image 
	// array, additionally it sets up the pointers the images in the buttons
	// array

	// create the bob image to hold all the button images
	button_img->Create(0,0,128,48,10, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 32);

	// set visibility
	button_img->SetAttrBit(BOB_ATTR_VISIBLE);

	// load the bitmap to extract imagery
	t3d::BmpFile* bitmap = new t3d::BmpFile("../../assets/chap13/bspguicon01.bmp");

	// interate and extract both the off and on bitmaps for each animated button
	for (int index = 0; index < 10; index++)
	{
		// load the frame left to right, row by row
		button_img->LoadFrame(bitmap,index,index%2,index/2,BITMAP_EXTRACT_MODE_CELL);

	} // end for index

	// fix pointers in gadget array
	buttons[GADGET_ID_SEGMENT_MODE_ID].bitmap  = button_img;
	buttons[GADGET_ID_POLYLINE_MODE_ID].bitmap = button_img;
	buttons[GADGET_ID_DELETE_MODE_ID].bitmap   = button_img;
	buttons[GADGET_ID_CLEAR_ALL_ID].bitmap     = button_img;
	buttons[GADGET_ID_FLOOR_MODE_ID].bitmap    = button_img;

	// unload the bitmap
	delete bitmap;
}

void Gadgets::Draw(LPDIRECTDRAWSURFACE7 lpdds)
{
	// draws all the gadgets, note that buffer must be unlocked

	// interate and draw all the buttons 
	for (int index = 0; index < NUM_GADGETS; index++)
	{
		// get the current button
		Gadget* curr_button = &buttons[index];

		// what type of button
		switch(curr_button->type)
		{
		case GADGET_TYPE_TOGGLE:    // toggles from off/on, on/off
			{
				// set the frame of the button
				button_img->SetCurrFrame(curr_button->id + curr_button->state);

				// set position of the button
				button_img->SetPosition(curr_button->x0, curr_button->y0);

				// draw the button         
				button_img->Draw(lpdds);

			} break;

		case GADGET_TYPE_MOMENTARY: // lights up while you press it
			{
				// set the frame of the button
				button_img->SetCurrFrame(curr_button->id + curr_button->state);

				// set position of the button
				button_img->SetPosition(curr_button->x0, curr_button->y0);

				// draw the button               
				button_img->Draw(lpdds);

				// increment counter
				if (curr_button->state == GADGET_ON)
				{
					if (++curr_button->count > 2)
					{
						curr_button->state = GADGET_OFF;
						curr_button->count = 0;
					} // end if
				} // end if

			} break;

		case GADGET_TYPE_STATIC:    // doesn't do anything visual    
			{
				// do nothing

			} break;

		default:break;

		} // end switch

	} // end for index
}

int Gadgets::Process(int mx, int my, unsigned char mbuttons[])
{
	// this function processes the gadgets messages and changes their states
	// based on clicks from the mouse

	// iterate thru all the gadgets and determine if there has been a click in
	// any of the clickable areas 

	for (int index = 0; index < NUM_GADGETS; index++)
	{
		// get the current button
		Gadget* curr_button = &buttons[index];

		// test for a button press
		if (mbuttons[0] | mbuttons[1] | mbuttons[2])
		{
			// determine if there has been a collision
			if ( (mx > curr_button->x0) && (mx < curr_button->x0+curr_button->width) && 
				(my > curr_button->y0) && (my < curr_button->y0+curr_button->height) )
			{
				// what type of button
				switch(curr_button->type)
				{
				case GADGET_TYPE_TOGGLE:    // toggles from off/on, on/off
					{
						// set the frame of the button
						if (curr_button->state == GADGET_OFF)
							curr_button->state = GADGET_ON;

						// reset all other mutual exclusive toggles
						for (int index2 = 0; index2 < NUM_GADGETS; index2++)
							if (index2!=index && buttons[index2].type == GADGET_TYPE_TOGGLE)
								buttons[index2].state = GADGET_OFF;

						// return the id of the button pressed
						return(curr_button->id);

					} break;

				case GADGET_TYPE_MOMENTARY: // lights up while you press it
					{
						// set the frame of the button to onn
						curr_button->state = GADGET_ON;

						// return the id of the button pressed
						return(curr_button->id);

					} break;

				case GADGET_TYPE_STATIC:    // doesn't do anything visual    
					{
						// do nothing

						// return the id of the button pressed
						return(curr_button->id);

					} break;

				default:break;

				} // end switch

			} // end if collision detected

		} // end if button pressed
		else
		{
			// reset buttons, etc. do housekeeping

		} // end else   


	} // end for index

	// nothing was pressed
	return(-1);
}

}