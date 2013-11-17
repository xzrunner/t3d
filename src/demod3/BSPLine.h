#pragma once

#include "Vector.h"

namespace bspeditor {

struct BSPLine
{
	int id;            // id of line
	int color;         // color of line (polygon)
	int attr;          // line (polygon) attributes (shading modes etc.)
	int texture_id;    // texture index 
	vec2 p0, p1;       // endpoints of line

	int elev, height;  // height of wall
};

}