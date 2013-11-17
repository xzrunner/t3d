#include "Polygon.h"
#include "Matrix.h"
#include "defines.h"

namespace t3d {

// general clipping flags for polygons
#define CLIP_POLY_X_PLANE           0x0001 // cull on the x clipping planes
#define CLIP_POLY_Y_PLANE           0x0002 // cull on the y clipping planes
#define CLIP_POLY_Z_PLANE           0x0004 // cull on the z clipping planes

// defines that control the rendering function state attributes
// note each class of control flags is contained within
// a 4-bit nibble where possible, this helps with future expansion

// no z buffering, polygons will be rendered as are in list
#define RENDER_ATTR_NOBUFFER                     0x00000001  

// use z buffering rasterization
#define RENDER_ATTR_ZBUFFER                      0x00000002  

// use a Z buffer, but with write thru, no compare
#define RENDER_ATTR_WRITETHRUZBUFFER			 0x00000008  

// use 1/z buffering rasterization
#define RENDER_ATTR_INVZBUFFER                   0x00000004  

// use mipmapping
#define RENDER_ATTR_MIPMAP                       0x00000010  

// enable alpha blending and override
#define RENDER_ATTR_ALPHA                        0x00000020  

// enable bilinear filtering, but only supported for
// constant shaded/affine textures
#define RENDER_ATTR_BILERP                       0x00000040  

// use affine texturing for all polys
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE   0x00000100  

// use perfect perspective texturing
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT  0x00000200  

// use linear piecewise perspective texturing
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR   0x00000400  

// use a hybrid of affine and linear piecewise based on distance
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1  0x00000800  

// not implemented yet
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID2  0x00001000  

struct RenderContext
{
	int     attr;                 // all the rendering attributes
	unsigned char* video_buffer;  // ptr to video buffer to render to
	int     lpitch;               // memory pitch in bytes of video buffer       

	unsigned char*zbuffer;        // ptr to z buffer or 1/z buffer
	int     zpitch;               // memory pitch in bytes of z or 1/z buffer
	int     alpha_override;       // alpha value to override ALL polys with

	int     mip_dist;             // maximum distance to divide up into 
								  // mip levels
								  // 0 - (NUM_ALPHA_LEVELS - 1)
	int     texture_dist,         // the distance to enable affine texturing
			texture_dist2;        // when using hybrid perspective/affine mode

	// future expansion
	int     ival1, ivalu2;        // extra integers
	float   fval1, fval2;         // extra floats
	void    *vptr;                // extra pointer
};

class Camera;
class RenderObject;

class RenderList
{
public:

	bool Insert(const Polygon& poly);
	bool Insert(const PolygonF& poly);
	bool Insert(const RenderObject& obj, bool insert_local = false);

	void Transform(const mat4& mt, int coord_select);

	void ModelToWorld(const vec4& world_pos, 
		int coord_select=TRANSFORM_LOCAL_TO_TRANS);

	void RemoveBackfaces(const Camera& cam);

	void WorldToCamera(const Camera& cam);

	void CameraToPerspective(const Camera& cam);

	void PerspectiveToScreen(const Camera& cam);

	void Reset();

	void DrawWire32(unsigned char* video_buffer, int lpitch);
	void DrawSolid32(unsigned char* video_buffer, int lpitch);
	void DrawSolidZB32(unsigned char* video_buffer, int lpitch,
		unsigned char* zbuffer, int zpitch);
	void DrawTextured32(unsigned char* video_buffer, int lpitch, 
		BmpImg* texture);
	void DrawContext(const RenderContext& rc);
	void DrawHybridTexturedSolidINVZB32(unsigned char* video_buffer, int lpitch,
		unsigned char* zbuffer, int zpitch, float dist1, float dist2);

	void LightWorld32(const Camera& cam);

	// z-sort algorithm (simple painters algorithm)
	void Sort(int sort_method = SORT_POLYLIST_AVGZ);

	void ClipPolys(const Camera& cam, int clip_flags);

	int GetNumPolys() const { return _num_polys; }

private:
	// render list defines
	static const int MAX_POLYS = 32768;

private:
	int _state; // _state of renderlist ???
	int _attr;  // attributes of renderlist ???

	// the render list is an array of pointers each pointing to 
	// a self contained "renderable" polygon face POLYF4DV1
	PolygonF* _poly_ptrs[MAX_POLYS];

	// additionally to cut down on allocatation, de-allocation
	// of polygons each frame, here's where the actual polygon
	// faces will be stored
	PolygonF _poly_data[MAX_POLYS];

	int _num_polys; // number of polys in render list

}; // RenderList

}