#pragma once

#include <string.h>

#include "Color.h"
#include "BmpImg.h"

namespace t3d {

// defines for materials, follow our polygon attributes as much as possible
#define MAT_ATTR_2SIDED                 0x0001
#define MAT_ATTR_TRANSPARENT            0x0002
#define MAT_ATTR_8BITCOLOR              0x0004
#define MAT_ATTR_RGB16                  0x0008
#define MAT_ATTR_RGB24                  0x0010

#define MAT_ATTR_SHADE_MODE_CONSTANT    0x0020
#define MAT_ATTR_SHADE_MODE_EMMISIVE    0x0020 // alias
#define MAT_ATTR_SHADE_MODE_FLAT        0x0040
#define MAT_ATTR_SHADE_MODE_GOURAUD     0x0080
#define MAT_ATTR_SHADE_MODE_FASTPHONG   0x0100
#define MAT_ATTR_SHADE_MODE_TEXTURE     0x0200

class Material
{
public:
	Material() : ptr(0), texture(0) {
		memset(this, 0, sizeof(Material));
	}

public:
	int state;           // state of material
	int id;              // id of this material, index into material array
	char name[64];       // name of material
	int  attr;           // attributes, the modes for shading, constant, flat, 
						 // gouraud, fast phong, environment, textured etc.
						 // and other special flags...

	Color color;             // color of material
	float ka, kd, ks, power; // ambient, diffuse, specular, 
							 // coefficients, note they are 
							 // separate and scalars since many 
							 // modelers use this format
							 // along with specular power

	Color ra, rd, rs;        // the reflectivities/colors pre-
							 // multiplied, to more match our 
							 // definitions, each is basically
							 // computed by multiplying the 
							 // color by the k's, eg:
							 // rd = color*kd etc.

	char texture_file[80];   // file location of texture
	BmpImg* texture;		 // actual texture map (if any)

	int   iaux1, iaux2;      // auxiliary vars for future expansion
	float faux1, faux2;
	void *ptr;

}; // Material

class MaterialsMgr
{
public:
	MaterialsMgr() : num_materials(0) {}

	Material& operator [] (int index) {
		return index < MAX_MATERIALS ? materials[index] : materials[MAX_MATERIALS-1];
	}
	int Size() const { return num_materials; }
	void SetSize(int size) { num_materials = size; }

public:
	static const int MAX_MATERIALS = 256;

private:
	Material materials[MAX_MATERIALS];	// materials in system
	int num_materials;					// current number of materials

}; // MaterialsMgr

}