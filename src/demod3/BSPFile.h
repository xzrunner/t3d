/*
BSP .LEV file format version 1.0

d integer, f floating point value

Version: f    <-- the version, 1.0 in this case

NumSections: d  <-- number of sections, always 2 for now

Section: walls
NumWalls: n    <-- number of walls 

x0.f y0.f x1.f y1.f evel.f height.f text_id.d color.d attr.d <-- endpoints, elevation, and height of wall, texture id, color, attributes
.
.
.
x0.f y0.f x1.f y1.f elev.f height.f text_id.d color.d attr.d <-- endpoints, elevation, and height of wall, texture id, color, attributes

EndSection

Section: floors
NumFloorsX: d              < -- number of floor cells in X directions, or columns
NumFloorsY: d              < -- number of floor cells in Y direction, or rows

id0.d id1.d id2.d ..... id(NumFloorsX - 1).d    < -- a total of NumFloorsY rows, each idi is the texture id
.                                               < -- -1 means no floor tile there
.
.
.
id0.d id1.d id2.d ..... id(NumFloorsX - 1).d

EndSection

example*********


Version: 1.0
NumSections: 2

Section: walls
NumWalls: 4 
10 10 20 10 0 10 0
20 10 20 20 0 10 0
20 20 10 20 0 10 0
10 20 10 10 0 10 0
Ends

Section: floors
NumFloorsX: 6
NumFloorsY: 4
-1 0 0 -1 -1 -1
-1 0 0 -1 -1 -1
-1 -1 -1 -1 -1 -1
-1 -1 -1 -1 -1 -1
ends
*/

#pragma once

#include <ddraw.h>

#include "Vector.h"

namespace t3d { class BSPNode; class BmpImg; class RenderObject; class BOB; }

namespace bspeditor {

#define SCREEN_TO_WORLD_X    (-BSP_MAX_X/2)   // the translation factors to move the origin
#define SCREEN_TO_WORLD_Z    (-BSP_MAX_Y/2)   // to the center of the 2D universe

#define WORLD_SCALE_X              2      // scaling factors from screen space to world
#define WORLD_SCALE_Y              2      // space (note the inversion of the Z)
#define WORLD_SCALE_Z             -2

class BSPFile
{
public:
	BSPFile();
	~BSPFile();

	void Load(char *filename);

	void Save(char *filename, int flags);

	void ConvertLinesToWalls();

	void GenerateFloorsObj(t3d::RenderObject* obj,   // pointer to object
					       int rgbcolor,        // color of floor if no texture        
						   vec4* pos,		   // initial position
						   vec4* rot,		   // initial rotations
						   int poly_attr);      // the shading attributes we would like

public:
	char buffer[256];        // text buffer

	t3d::BSPNode *wall_list,		// global pointer to the linked list of walls
		         *bsp_root;		    // global pointer to root of BSP tree

}; // BSPFile

}