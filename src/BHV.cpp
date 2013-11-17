#include "BHV.h"

#include "Modules.h"
#include "Log.h"
#include "RenderObject.h"
#include "Camera.h"

namespace t3d {

void BHVNode::Build(ObjContainer* bhv_objects, int _num_objects, int level, 
					int num_divisions, int universe_radius)
{
	// this function builds the BHV tree using a divide and conquer
	// divisioning algorithm to cluster the objects together

	Modules::GetLog().WriteError("\nEntering BHV function...");

	// are we building root?
	if (level == 0)
	{
		Modules::GetLog().WriteError("\nlevel = 0");

		// position at (0,0,0)
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
		pos.w = 1;

		// set radius of node to maximum
		radius.x = universe_radius;
		radius.y = universe_radius;
		radius.z = universe_radius;
		radius.w = 1;

		Modules::GetLog().WriteError("\nnode pos[%f, %f, %f], r[%f, %f, %f]", 
			pos.x,pos.y,pos.z,
			radius.x,radius.y,radius.z);

		// build root, simply add all objects to root
		for (int index = 0; index < _num_objects; index++)
		{
			// make sure object is not culled 
			if (!(bhv_objects[index].state & OBJECT_STATE_CULLED))
			{
				objects[num_objects++] = (ObjContainer*)&bhv_objects[index];
			} // end if

		} // end for index

		// at this point all the objects have been inserted into the root node
		// and num_objects is set to the number of objects
		// enable the node
		state = 1;
		attr  = 0;

		// and set all links to NULL
		for (int ilink = 0; ilink < MAX_BHV_PER_NODE; ilink++)
			links[ilink] = NULL;

		// set the number of objects
		num_objects = _num_objects;

		Modules::GetLog().WriteError("\nInserted %d objects into root node", num_objects);
		Modules::GetLog().WriteError("\nMaking recursive call with root node...");

		// done, so now allow recursion to build the rest of the tree
		Build(bhv_objects, _num_objects, 1, num_divisions, universe_radius);

	} // end if
	else
	{
		Modules::GetLog().WriteError("\nEntering Level = %d > 0 block, number of objects = %d",level, num_objects);

		// test for exit state
		if (num_objects <= MIN_OBJECTS_PER_BHV_CELL)
			return;

		// building a child node (hard part)
		// we must take the current node and split it into a number of children nodes, and then
		// create a bvh for each child and then insert all the objects into each child
		// the for each child call the recursion again....

		// create 3D cell temp storage to track cells
		BHVCell cells[MAX_BHV_CELL_DIVISIONS][MAX_BHV_CELL_DIVISIONS][MAX_BHV_CELL_DIVISIONS];

		// find origin of bounding volume based on radius and center
		int x0 = pos.x - radius.x;
		int y0 = pos.y - radius.y;
		int z0 = pos.z - radius.z;

		// compute cell sizes on x,y,z axis
		float cell_size_x = 2*radius.x / (float)num_divisions;
		float cell_size_y = 2*radius.y / (float)num_divisions;
		float cell_size_z = 2*radius.z / (float)num_divisions;

		Modules::GetLog().WriteError("\ncell pos=(%d, %d, %d) size=(%f, %f, %f)", x0,y0,z0, cell_size_x, cell_size_y, cell_size_z);

		int cell_x, cell_y, cell_z; // used to locate cell in 3D matrix

		// clear cell memory out
		memset(cells, 0, sizeof(cells));

		// now partition space up into num_divisions (must be < MAX_BHV_CELL_DIVISIONS)
		// and then sort each object's center into each cell of the 3D sorting matrix
		for (int obj_index = 0; obj_index < num_objects; obj_index++)
		{
			// compute cell position in temp sorting matrix
			cell_x = (objects[obj_index]->pos.x - x0)/cell_size_x;
			cell_y = (objects[obj_index]->pos.y - y0)/cell_size_y;
			cell_z = (objects[obj_index]->pos.z - z0)/cell_size_z;

			// insert this object into list
			cells[cell_x][cell_y][cell_z].obj_list[ cells[cell_x][cell_y][cell_z].num_objects ] = objects[obj_index];

			Modules::GetLog().WriteError("\ninserting object %d located at (%f, %f, %f) into cell (%d, %d, %d)", obj_index, 
				objects[obj_index]->pos.x,
				objects[obj_index]->pos.y,
				objects[obj_index]->pos.z,
				cell_x, cell_y, cell_z);

			// increment number of objects in this cell
			if (++cells[cell_x][cell_y][cell_z].num_objects >= MAX_OBJECTS_PER_BHV_CELL)
				cells[cell_x][cell_y][cell_z].num_objects  = MAX_OBJECTS_PER_BHV_CELL-1;   

		} // end for obj_index

		Modules::GetLog().WriteError("\nEntering sorting section...");

		// now the 3D sorting matrix has all the information we need, the next step
		// is to create a BHV node for each non-empty

		for (int icell_x = 0; icell_x < num_divisions; icell_x++)
		{
			for (int icell_y = 0; icell_y < num_divisions; icell_y++)
			{
				for (int icell_z = 0; icell_z < num_divisions; icell_z++)
				{
					// are there any objects in this node?
					if ( cells[icell_x][icell_y][icell_z].num_objects > 0)
					{
						Modules::GetLog().WriteError("\nCell %d, %d, %d contains %d objects", icell_x, icell_y, icell_z, 
							cells[icell_x][icell_y][icell_z].num_objects);

						Modules::GetLog().WriteError("\nCreating child node...");

						// create a node and set the link to it
						links[ num_children ] = (BHVNode*)malloc(sizeof(BHVNode));

						// zero node out
						memset(links[ num_children ], 0, sizeof(BHVNode) );

						// set the node up
						BHVNode* curr_node = links[ num_children ];

						// position
						curr_node->pos.x = (icell_x*cell_size_x + cell_size_x/2) + x0;
						curr_node->pos.y = (icell_y*cell_size_y + cell_size_y/2) + y0;
						curr_node->pos.z = (icell_z*cell_size_z + cell_size_z/2) + z0;
						curr_node->pos.w = 1;

						// radius is cell_size / 2
						curr_node->radius.x = cell_size_x/2;
						curr_node->radius.y = cell_size_y/2;
						curr_node->radius.z = cell_size_z/2;
						curr_node->radius.w = 1;

						// set number of objects
						curr_node->num_objects = cells[icell_x][icell_y][icell_z].num_objects;

						// set num children
						curr_node->num_children = 0;

						// set state and attr
						curr_node->state        = 1; // enable node               
						curr_node->attr         = 0;

						// now insert each object into this node's object list
						for (int icell_index = 0; icell_index < curr_node->num_objects; icell_index++)
						{
							curr_node->objects[icell_index] = cells[icell_x][icell_y][icell_z].obj_list[icell_index];    
						} // end for icell_index

						Modules::GetLog().WriteError("\nChild node pos=(%f, %f, %f), r=(%f, %f, %f)", 
							curr_node->pos.x,curr_node->pos.y,curr_node->pos.z,
							curr_node->radius.x,curr_node->radius.y,curr_node->radius.z);

						// increment number of children of parent
						num_children++;

					} // end if

				} // end for icell_z

			} // end for icell_y

		} // end for icell_x

		Modules::GetLog().WriteError("\nParent has %d children..", num_children);

		// now for each child build a BHV
		for (int inode = 0; inode < num_children; inode++)
		{
			Modules::GetLog().WriteError("\nfor Level %d, creating child %d", level, inode);

			links[inode]->Build(NULL, // unused now
				                NULL, // unused now
								level+1, 
								num_divisions,
								universe_radius);
		} // end if     

	} // end else level > 0

	Modules::GetLog().WriteError("\nExiting BHV...level = %d", level);
}

void BHVNode::FrustrumCull(const Camera& cam, int cull_flags)
{
	// NOTE: is matrix based
	// this function culls the BHV from the viewing
	// frustrum by using the sent camera information and
	// the cull_flags determine what axes culling should take place
	// x, y, z or all which is controlled by ORing the flags
	// together. as the BHV is culled the state information in each node is 
	// modified, so rendering functions can refer to it

	// we need to walk the tree from top to bottom culling

	// step 1: transform the center of the nodes bounding
	// sphere into camera space

	vec4 sphere_pos; // hold result of transforming center of bounding sphere

	// transform point
	sphere_pos = cam.CameraMat() * pos;

	// step 2:  based on culling flags remove the object
	if (cull_flags & CULL_OBJECT_Z_PLANE)
	{
		// cull only based on z clipping planes

		// test far plane
		if ( ((sphere_pos.z - radius.z) > cam.FarClipZ()) ||
			((sphere_pos.z + radius.z) < cam.NearClipZ()) )
		{ 
			// this entire node is culled, so we need to set the culled flag
			// for every object
			for (int iobject = 0; iobject < num_objects; iobject++)
			{
				SET_BIT(objects[iobject]->state, OBJECT_STATE_CULLED);
			} // end for iobject
#if 0
			Modules::GetLog().WriteError("\n[ZBHV p(%f, %f, %f) r(%f) #objs(%d)]", pos.x, 
				pos.y,
				pos.z,
				radius.x, 
				num_objects);
#endif


			// this node was visited and culled
			bhv_nodes_visited++;

			return;
		} // end if

	} // end if

	if (cull_flags & CULL_OBJECT_X_PLANE)
	{
		// cull only based on x clipping planes
		// we could use plane equations, but simple similar triangles
		// is easier since this is really a 2D problem
		// if the view volume is 90 degrees the the problem is trivial
		// buts lets assume its not

		// test the the right and left clipping planes against the leftmost and rightmost
		// points of the bounding sphere
		float z_test = (0.5)*cam.ViewplaneWidth()*sphere_pos.z/cam.ViewDist();

		if ( ((sphere_pos.x - radius.x) > z_test)  || // right side
			((sphere_pos.x + radius.x) < -z_test) )  // left side, note sign change
		{ 
			// this entire node is culled, so we need to set the culled flag
			// for every object
			for (int iobject = 0; iobject < num_objects; iobject++)
			{
				SET_BIT(objects[iobject]->state, OBJECT_STATE_CULLED);
			} // end for iobject
#if 0
			Modules::GetLog().WriteError("\n[XBHV p(%f, %f, %f) r(%f) #objs(%d)]", pos.x, 
				pos.y,
				pos.z,
				radius.x, num_objects);

#endif

			// this node was visited and culled
			bhv_nodes_visited++;

			return;
		} // end if
	} // end if

	if (cull_flags & CULL_OBJECT_Y_PLANE)
	{
		// cull only based on y clipping planes
		// we could use plane equations, but simple similar triangles
		// is easier since this is really a 2D problem
		// if the view volume is 90 degrees the the problem is trivial
		// buts lets assume its not

		// test the the top and bottom clipping planes against the bottommost and topmost
		// points of the bounding sphere
		float z_test = (0.5)*cam.ViewplaneHeight()*sphere_pos.z/cam.ViewDist();

		if ( ((sphere_pos.y - radius.y) > z_test)  || // top side
			((sphere_pos.y + radius.y) < -z_test) )  // bottom side, note sign change
		{ 
			// this entire node is culled, so we need to set the culled flag
			// for every object
			for (int iobject = 0; iobject < num_objects; iobject++)
			{
				SET_BIT(objects[iobject]->state, OBJECT_STATE_CULLED);
			} // end for iobject

#if 0
			Modules::GetLog().WriteError("\n[YBHV p(%f, %f, %f) r(%f) #objs(%d)]", pos.x, 
				pos.y,
				pos.z,
				radius.x, 
				num_objects);
#endif

			// this node was visited and culled
			bhv_nodes_visited++;

			return;
		} // end if

	} // end if

	// at this point, we have concluded that this BHV node is too large
	// to cull, so we need to traverse the children and see if we can cull them
	for (int ichild = 0; ichild < num_children; ichild++)
	{
		// recursively call..
		if (links[ichild])
			links[ichild]->FrustrumCull(cam, cull_flags);

		// here's where we can optimize by tracking the total number 
		// of objects culled and we can exit early if all the objects
		// are already culled...

	} // end ichild
}

void BHVNode::Reset()
{
	// this function simply resets all of the culled flags in the root of the
	// tree which will enable the entire tree as visible
	for (int iobject = 0; iobject < num_objects; iobject++)
	{
		// reset the culled flag
		RESET_BIT(objects[iobject]->state, OBJECT_STATE_CULLED);
	} // end if
}

}