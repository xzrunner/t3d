#pragma once

namespace t3d {

// defines for vertices, these are "hints" to the transform and
// lighting systems to help determine if a particular vertex has
// a valid normal that must be rotated, or a texture coordinate
// that must be clipped etc., this helps us minmize load during lighting
// and rendering since we can determine exactly what kind of vertex we
// are dealing with, something like a (direct3d) flexible vertex format in 
// as much as it can hold:
// point
// point + normal
// point + normal + texture coordinates
#define VERTEX_ATTR_NULL             0x0000 // this vertex is empty
#define VERTEX_ATTR_POINT            0x0001
#define VERTEX_ATTR_NORMAL           0x0002
#define VERTEX_ATTR_TEXTURE          0x0004

// used for simple model formats to override/control the lighting
#define VERTEX_FLAGS_OVERRIDE_MASK          0xf000 // this masks these bits to extract them
#define VERTEX_FLAGS_OVERRIDE_CONSTANT      0x1000
#define VERTEX_FLAGS_OVERRIDE_EMISSIVE      0x1000 //(alias)
#define VERTEX_FLAGS_OVERRIDE_PURE          0x1000
#define VERTEX_FLAGS_OVERRIDE_FLAT          0x2000
#define VERTEX_FLAGS_OVERRIDE_GOURAUD       0x4000
#define VERTEX_FLAGS_OVERRIDE_TEXTURE       0x8000

#define VERTEX_FLAGS_INVERT_TEXTURE_U       0x0080   // invert u texture coordinate 
#define VERTEX_FLAGS_INVERT_TEXTURE_V       0x0100   // invert v texture coordinate
#define VERTEX_FLAGS_INVERT_SWAP_UV         0x0800   // swap u and v texture coordinates

struct Vertex
{
	union
	{
		float M[12];            // array indexed storage

		// explicit names
		struct
		{
			float x,y,z,w;     // point
			float nx,ny,nz,nw; // normal (vector or point)
			float u0,v0;       // texture coordinates 

			float i;           // final vertex intensity after lighting
			int   attr;        // attributes/ extra texture coordinates
		};

		// high level types
		struct 
		{
			vec4 v;       // the vertex
			vec4 n;       // the normal
			vec2 t;       // texture coordinates
		};
	};
}; // Vertex

}