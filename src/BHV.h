#pragma once

#include "Vector.h"

namespace t3d {

// defines for BHV system
#define MAX_OBJECTS_PER_BHV_CELL    64   // maximum objects that will fit in one BHV cell
                                         // note: this is DIFFERENT than a node
#define MIN_OBJECTS_PER_BHV_CELL    2    // recursion stops if a cell has less that this 
                                         // number of objects

#define MAX_BHV_CELL_DIVISIONS      8    // maximum number of cells in one axis that
                                         // space can be divided into 

#define MAX_OBJECTS_PER_BHV_NODE    512  // maximum objects a node can handle

#define MAX_BHV_PER_NODE            32   // maximum children per node

struct ObjContainer
{
	int state;          // state of container object
	int attr;           // attributes of container object
	vec4 pos;			// position of container object
	vec4 vel;			// velocity of object container
	vec4 rot;			// rotational rate of object container
	int auxi[8];        // aux array of 8 ints
	int auxf[8];        // aux array of 8 floats
	void *aux_ptr;      // aux pointer
	void *obj;          // pointer to master object
};

struct BHVCell
{
	int num_objects;                                     // number of objects in this cell
	ObjContainer* obj_list[MAX_OBJECTS_PER_BHV_CELL];    // storage for object pointers
};

class Camera;

class BHVNode
{
public:
	BHVNode() {
		memset(this, 0, sizeof(BHVNode));
	}

	void Build(ObjContainer* bhv_objects,       // ptr to all objects in intial scene
		       int num_objects,                 // number of objects in initial scene
		       int level,                       // recursion level
		       int num_divisions,               // number of division per level
		       int universe_radius);

	void FrustrumCull(const Camera& cam,         // camera to cull relative to
		              int cull_flags);			 // clipping planes to consider

	void Reset();

private:
	int state;         // state of this node
	int attr;          // attributes of this node
	vec4 pos;       // center of node
	vec4 radius;   // x,y,z radius of node
	int num_objects;   // number of objects contained in node
	ObjContainer *objects[MAX_OBJECTS_PER_BHV_NODE]; // objects

	int num_children;  // number of children nodes

	BHVNode* links[MAX_BHV_PER_NODE]; // links to children

}; // BHVNode

}