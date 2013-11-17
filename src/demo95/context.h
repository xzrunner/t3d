#pragma once

#include "Graphics.h"
#include "Vector.h"

namespace raider3d {

#define WINDOW_WIDTH      800  // size of window
#define WINDOW_HEIGHT     600

// player defines
#define CROSS_START_X         0
#define CROSS_START_Y         0
#define CROSS_WIDTH           56
#define CROSS_HEIGHT          56

// font display stuff
#define FONT_HPITCH           12  // space between chars
#define FONT_VPITCH           14  // space between lines

// 3D engine constants for overlay star field
#define NEAR_Z            10    // the near clipping plane
#define FAR_Z             2000  // the far clipping plane    
#define VIEW_DISTANCE      400  // viewing distance from viewpoint 
                                // this gives a field of view of 90 degrees
                                // when projected on a window of 800 wide

// difficulty stuff
#define DIFF_RATE             (float)(.001f)  // rate to advance  difficulty per frame
#define DIFF_PMAX             75

// interface text positions
#define TPOS_SCORE_X          346  // players score
#define TPOS_SCORE_Y          4

#define TPOS_HITS_X           250  // total kills
#define TPOS_HITS_Y           36

#define TPOS_ESCAPED_X        224  // aliens that have escaped
#define TPOS_ESCAPED_Y        58
 
#define TPOS_GLEVEL_X         430 // diff level of game
#define TPOS_GLEVEL_Y         58

#define TPOS_SPEED_X          404 // diff level of game
#define TPOS_SPEED_Y          36

#define TPOS_GINFO_X          400  // general information
#define TPOS_GINFO_Y          240

#define TPOS_ENERGY_X         158 // weapon energy level
#define TPOS_ENERGY_Y         6

extern float cross_x, // cross hairs
			 cross_y; 

extern int cross_x_screen,   // used for cross hair
		   cross_y_screen, 
		   target_x_screen,   // used for targeter
		   target_y_screen;

// diffiuculty
extern float diff_level;	// difficulty level used in velocity and probability calcs

extern int escaped;			// tracks number of missed ships
extern int hits;			// tracks number of hits
extern int score;
extern int weapon_energy;	// weapon energy
extern bool weapon_active;

// create system colors
static const signed int rgb_green = _RGB32BIT(255,0,255,0);
static const signed int rgb_white = _RGB32BIT(255,255,255,255);
static const signed int rgb_blue  = _RGB32BIT(255,0,0,255);
static const signed int rgb_red   = _RGB32BIT(255,255,0,0);

// player and game globals
static const int player_z_vel = 4; // virtual speed of viewpoint/ship

// these are the positions of the energy binding on the main lower control panel
static vec2 energy_bindings[6] = { vec2(342, 527), vec2(459, 527), 
								   vec2(343, 534), vec2(458, 534), 
								   vec2(343, 540), vec2(458, 540) };
// these hold the positions of the weapon burst which use lighting too
// the starting points are known, but the end points are computed on the fly
// based on the cross hair
static vec2 weapon_bursts[4] = { vec2(78, 500), vec2(0,0),     // left energy weapon
								 vec2(720, 500), vec2(0,0) };  // right energy weapon
}