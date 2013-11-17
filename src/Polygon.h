#pragma once

#include "Vector.h"
#include "Vertex.h"

namespace t3d {

// states of polygons and faces
#define POLY_STATE_NULL               0x0000
#define POLY_STATE_ACTIVE             0x0001
#define POLY_STATE_CLIPPED            0x0002
#define POLY_STATE_BACKFACE           0x0004
#define POLY_STATE_LIT                0x0008

// attributes of polygons and polygon faces
#define POLY_ATTR_2SIDED              0x0001
#define POLY_ATTR_TRANSPARENT         0x0002
#define POLY_ATTR_8BITCOLOR           0x0004
#define POLY_ATTR_RGB16               0x0008
#define POLY_ATTR_RGB24               0x0010

#define POLY_ATTR_SHADE_MODE_PURE       0x0020
#define POLY_ATTR_SHADE_MODE_CONSTANT   0x0020 // (alias)
#define POLY_ATTR_SHADE_MODE_EMISSIVE   0x0020 // (alias)
#define POLY_ATTR_SHADE_MODE_FLAT       0x0040
#define POLY_ATTR_SHADE_MODE_GOURAUD    0x0080
#define POLY_ATTR_SHADE_MODE_PHONG      0x0100
#define POLY_ATTR_SHADE_MODE_FASTPHONG  0x0100 // (alias)
#define POLY_ATTR_SHADE_MODE_TEXTURE    0x0200 
#define POLY_ATTR_MIPMAP				0x0400 // flags if polygon has a mipmap
#define POLY_ATTR_ENABLE_MATERIAL       0x0800 // use a real material for lighting
#define POLY_ATTR_DISABLE_MATERIAL      0x1000 // use basic color only for lighting (emulate version 1.0)

// states of polygons and faces
#define POLY_STATE_ACTIVE             0x0001
#define POLY_STATE_CLIPPED            0x0002
#define POLY_STATE_BACKFACE           0x0004

class BmpImg;

class Polygon
{
public:
	Polygon() {
		memset(this, 0, sizeof(Polygon)); 
		mati = -1;
	}

public:
	int state;				// state information
	int attr;				// physical attributes of polygon
	int color;				// color of polygon
	int lit_color[3];		// holds colors after lighting, 0 for flat shading
							// 0,1,2 for vertex colors after vertex lighting

	BmpImg* texture;	// pointer to the texture information for simple texture mapping

	int mati;			// material index (-1) for no material

	Vertex* vlist;		// the vertex list itself
	vec2* tlist;		// the texture list itself
	int vert[3];		// the indices into the vertex list
	int text[3];        // the indices into the texture coordinate list
	float nlength;      // length of normal
};

class PolygonF
{
public:
	PolygonF() {
 		memset(this, 0, sizeof(PolygonF)); 
 		mati = -1;
	};

public:
	int state;					// state information
	int attr;					// physical attributes of polygon
	int color;					// color of polygon
	int lit_color[3];			// holds colors after lighting, 0 for flat shading
								// 0,1,2 for vertex colors after vertex lighting

	BmpImg* texture;	// pointer to the texture information for simple texture mapping

	int mati;			// material index (-1) for no material

 	float nlength;		// length of the polygon normal if not normalized
 	vec4 normal;		// the general polygon normal
 
 	float avg_z;		// average z of vertices, used for simple sorting

	Vertex vlist[3];		// the vertices of this triangle
	Vertex tvlist[3];		// the vertices after transformation if needed

	PolygonF *next;		// pointer to next polygon in list??
	PolygonF *prev;		// pointer to previous polygon in list??

}; // PolygonF

// a self contained quad polygon used for the render list version 2 //////
// we need this to represent the walls since they are quads
class PolygonQF
{
public:
	int      state;           // state information
	int      attr;            // physical attributes of polygon
	int      color;           // color of polygon
	int      lit_color[4];    // holds colors after lighting, 0 for flat shading
							  // 0,1,2 for vertex colors after vertex lighting
	BmpImg* texture;		// pointer to the texture information for simple texture mapping

	int      mati;          // material index (-1) for no material

	float    nlength;       // length of the polygon normal if not normalized 
	vec4	 normal;        // the general polygon normal

	float    avg_z;         // average z of vertices, used for simple sorting

	Vertex vlist[4];		// the vertices of this quad
	Vertex tvlist[4];		// the vertices after transformation if needed 

	PolygonQF *next;		// pointer to next polygon in list??
	PolygonQF *prev;		// pointer to previous polygon in list??

}; // PolygonQF

}