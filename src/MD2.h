#pragma once

#include "Vector.h"

namespace t3d {

// DEFINES //////////////////////////////////////////////////////////////////

#define MD2_MAGIC_NUM       (('I') + ('D' << 8) + ('P' << 16) + ('2' << 24))
#define MD2_VERSION         8 

#define MD2_DEBUG           0  // 0 - minimal output when loading files
                               // 1 - the farm
// md2 animation defines
#define NUM_MD2_ANIMATIONS  20 // total number of MD2 animations
#define MD2_MAX_FRAMES     198 // total number of MD2 frames, but I have seen 199, and 200

// md2 animation states
#define MD2_ANIM_STATE_STANDING_IDLE      0  // model is standing and idling
#define MD2_ANIM_STATE_RUN                1  // model is running
#define MD2_ANIM_STATE_ATTACK             2  // model is firing weapon/attacking
#define MD2_ANIM_STATE_PAIN_1             3  // model is being hit version 1
#define MD2_ANIM_STATE_PAIN_2             4  // model is being hit version 2
#define MD2_ANIM_STATE_PAIN_3             5  // model is being hit version 3
#define MD2_ANIM_STATE_JUMP               6  // model is jumping
#define MD2_ANIM_STATE_FLIP               7  // model is using hand gestures :)
#define MD2_ANIM_STATE_SALUTE             8  // model is saluting
#define MD2_ANIM_STATE_TAUNT              9  // model is taunting
#define MD2_ANIM_STATE_WAVE               10 // model is waving at someone
#define MD2_ANIM_STATE_POINT              11 // model is pointing at someone
#define MD2_ANIM_STATE_CROUCH_STAND       12 // model is crouching and idling
#define MD2_ANIM_STATE_CROUCH_WALK        13 // model is walking while crouching
#define MD2_ANIM_STATE_CROUCH_ATTACK      14 // model is firing weapon/attacking with crouching
#define MD2_ANIM_STATE_CROUCH_PAIN        15 // model is being hit while crouching
#define MD2_ANIM_STATE_CROUCH_DEATH       16 // model is dying while crouching
#define MD2_ANIM_STATE_DEATH_BACK         17 // model is dying while falling backward
#define MD2_ANIM_STATE_DEATH_FORWARD      18 // model is dying while falling forward
#define MD2_ANIM_STATE_DEATH_SLOW         19 // model is dying slowly (any direction)

// modes to run animation
#define MD2_ANIM_LOOP                        0 // play animation over and over
#define MD2_ANIM_SINGLE_SHOT                 1 // single shot animation

extern char* MD2_ANIM_STRINGS[];

// note: the idle animations are sometimes used for other things like salutes, hand motions, etc.

// this if the header structure for a Quake II .MD2 file by id Software,
// I have slightly annotated the field names to make them more readable 
// and less cryptic :)
struct MD2Header
{
	int identifier;     // identifies the file type, should be "IDP2"
	int version;        // version number, should be 8
	int skin_width;     // width of texture map used for skinning
	int skin_height;    // height of texture map using for skinning
	int framesize;      // number of bytes in a single frame of animation
	int num_skins;      // total number of skins
						// listed by ASCII filename and are available 
						// for loading if files are found in full path
	int num_verts;      // number of vertices in each model frame, the 
						// number of vertices in each frame is always the
						// same
	int num_textcoords; // total number of texture coordinates in entire file 
						// may be larger than the number of vertices
	int num_polys;      // number of polygons per model, or per frame of 
						// animation if you will
	int num_openGLcmds; // number of openGL commands which can help with
						// rendering optimization
						// however, we won't be using them

	int num_frames;     // total number of animation frames

	// memory byte offsets to actual data for each item

	int offset_skins;      // offset in bytes from beginning of file to the 
						   // skin array that holds the file name for each skin, 
						   // each file name record is 64 bytes
	int offset_textcoords; // offset in bytes from the beginning of file to the  
						   // texture coordinate array

	int offset_polys;      // offset in bytes from beginning of file to the 
						   // polygon mesh

	int offset_frames;     // offset in bytes from beginning of file to the 
						   // vertex data for each frame

	int offset_openGLcmds; // offset in bytes from beginning of file to the 
						   // openGL commands

	int offset_end;        // offset in bytes from beginning of file to end of file
};

// this is a single point for the md2 model, contains an 8-bit scaled x,y,z 
// and an index for the normal to the point that is encoded as an
// index into a normal table supplied by id software
struct MD2Point
{
	unsigned char v[3];          // vertex x,y,z in compressed byte format
	unsigned char normal_index;  // index into normal table from id Software (unused)
};

// this is a single u,v texture
struct MD2Textcoord
{
	short u,v; 
};

// this is a single frame for the md2 format, it has a small header portion 
// which describes how to scale and translate the model vertices, but then
// has an array to the actual data points which is variable, so the definition
// uses a single unit array to allow the compiler to address up to n with
// array syntax
struct MD2Frame
{
	float scale[3];          // x,y,z scaling factors for the frame vertices
	float translate[3];      // x,y,z translation factors for the frame vertices
	char name[16];           // ASCII name of the model, "evil death lord" etc. :)

	MD2Point vlist[1];      // beginning of vertex storage array
};

// this is a data structure for a single md2 polygon (triangle)
// it's composed of 3 vertices and 3 texture coordinates, both
// indices
struct MD2Poly
{
	unsigned short vindex[3];   // vertex indices 
	unsigned short tindex[3];   // texture indices
};

class RenderObject;
class BmpImg;

// finally, we will create a container class to hold the md2 model and help
// with animation etc. later, but this model will be converted on the fly
// to one of our object models each frame, once the md2 model is loaded,
// is is converted to the container format and the original md2 model is 
// discarded..
class MD2Container
{
public:
	void Load(char *modelfile,		// the filename of the .MD2 model
              vec4* scale,			// initial scaling factors
              vec4* pos,			// initial position
              vec4* rot,			// initial rotations
              char *texturefile,	// the texture filename for the model
              int attr,				// the lighting/model attributes for the model
              int color,			// base color if no texturing
              int vertex_flags);	// control ordering etc.

	void PrepareObject(RenderObject* obj);	// destination object

	void ExtractFrame(RenderObject* obj);	// destination object

	void SetAnimation(int anim_state,					// which animation to play
			          int anim_mode = MD2_ANIM_LOOP);	// mode of animation single/loop

	void Animate();

public:
	int _state;          // state of the model
	int _attr;           // attributes of the model
	int _color;          // base color if no texture
	int _num_frames;     // number of frames in the model
	int _num_polys;      // number of polygons
	int _num_verts;      // number of vertices 
	int _num_textcoords; // number of texture coordinates

	BmpImg*  _skin;		 // pointer to texture skin for model

	MD2Poly* _polys;		// pointer to polygon list
	vec3* _vlist;		// pointer to vertex coordinate list
	vec2* _tlist;		// pointer to texture coordinate list

	vec4 _world_pos;	// position of object
	vec4 _vel;			// velocity of object

	int   _ivars[8];     // integer variables
	float _fvars[8];     // floating point variables
	int _counters[8];    // general counters      

	int _anim_state;     // state of animation
	int _anim_counter;   // general animation counter
	int _anim_mode;      // single shot, loop, etc.
	int _anim_speed;     // smaller number, faster animation
	int _anim_complete;  // flags if single shot animation is done
	float _curr_frame;   // current frame in animation 
};

struct MD2Animation
{
	int   start_frame;  // starting and ending frame
	int   end_frame;  
	float irate;        // interpolation rate
	int   anim_speed;   // animation rate 
};

}