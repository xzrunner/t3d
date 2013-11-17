#include "BSP.h"

#include "Modules.h"
#include "Log.h"
#include "defines.h"
#include "Camera.h"
#include "RenderList.h"

namespace t3d {

//////////////////////////////////////////////////////////////////////////
// class BSPNode
//////////////////////////////////////////////////////////////////////////

BSPNode::BSPNode()
{
	link = front = back = NULL;
}

void BSPNode::Translate(const vec4& trans)
{
	// this function translates all the walls that make up the bsp world
	// note function is recursive, we don't really need this function, but
	// it's a good example of how we might perform transformations on the BSP
	// tree and similar tree like structures using recursion
	// note the translation is perform from local to world coords

	static int index; // looping variable

	// translate back most sub-tree
	if (back) back->Translate(trans);

	// iterate thru all vertices of current wall and translate them
	for (index=0; index < 4; index++)
	{
		// perform translation
		wall.tvlist[index].x = wall.vlist[index].x + trans.x;
		wall.tvlist[index].y = wall.vlist[index].y + trans.y;
		wall.tvlist[index].z = wall.vlist[index].x + trans.z;
	} // end for index

	// translate front most sub-tree
	if (front) front->Translate(trans);
}

void BSPNode::Delete()
{
	delete back;
	delete front;

	//////////////////////////////////////////////////////////////////////////

// 	// this function recursively deletes all the nodes in the bsp tree and free's
// 	// the memory back to the OS.
// 
// 	BSPNode* temp_wall; // a temporary wall
// 
// 	// test if we have hit a dead end
// 	if (node==NULL)
// 		return;
// 
// 	// delete back sub tree
// 	Delete(node->back);
// 
// 	// delete this node, but first save the front sub-tree
// 	temp_wall = node->front;
// 
// 	// delete the memory
// 	free(node);
// 
// 	// assign the root to the saved front most sub-tree
// 	node = temp_wall;
// 
// 	// delete front sub tree
// 	Delete(node);
}

void BSPNode::Print()
{
	// this function performs a recursive in-order traversal of the BSP tree and
	// prints the results out to the file opened with fp_out as the handle

	// search left tree (back walls)
	Modules::GetLog().WriteError("\nTraversing back sub-tree...");

	// call recursively
	if (back) back->Print();

	// visit node
	Modules::GetLog().WriteError("\n\n\nWall ID #%d",id);

	Modules::GetLog().WriteError("\nstate   = %d", wall.state);      // state information
	Modules::GetLog().WriteError("\nattr    = %d", wall.attr);       // physical attributes of polygon
	Modules::GetLog().WriteError("\ncolor   = %d", wall.color);      // color of polygon
	Modules::GetLog().WriteError("\ntexture = %x", wall.texture);    // pointer to the texture information for simple texture mapping
	Modules::GetLog().WriteError("\nmati    = %d", wall.mati);       // material index (-1) for no material  (new)

	Modules::GetLog().WriteError("\nVertex 0: (%f,%f,%f,%f)",
		wall.vlist[0].x,
		wall.vlist[0].y,
		wall.vlist[0].z,
		wall.vlist[0].w);

	Modules::GetLog().WriteError("\nVertex 1: (%f,%f,%f, %f)",
		wall.vlist[1].x,
		wall.vlist[1].y,
		wall.vlist[1].z,
		wall.vlist[1].w);

	Modules::GetLog().WriteError("\nVertex 2: (%f,%f,%f, %f)",
		wall.vlist[2].x,
		wall.vlist[2].y,
		wall.vlist[2].z,
		wall.vlist[2].w);

	Modules::GetLog().WriteError("\nVertex 3: (%f,%f,%f, %f)",
		wall.vlist[3].x,
		wall.vlist[3].y,
		wall.vlist[3].z,
		wall.vlist[3].w);

	Modules::GetLog().WriteError("\nNormal (%f,%f,%f, %f), length=%f",
		wall.normal.x,
		wall.normal.y,
		wall.normal.z,
		wall.nlength);

	Modules::GetLog().WriteError("\nTextCoords (%f,%f)",wall.vlist[1].u0,
		wall.vlist[1].v0);

	Modules::GetLog().WriteError("\nEnd wall data\n");

	// search right tree (front walls)
	Modules::GetLog().WriteError("\nTraversing front sub-tree..");

	if (front) front->Print();
}

void BSPNode::Build()
{
	// this function recursively builds the bsp tree from the root of the wall list
	// the function works by taking the wall list which begins as a linked list
	// of walls in no particular order, then it uses the wall at the TOP of the list
	// as the separating plane and then divides space with that wall computing
	// the walls on the front and back of the separating plane. The walls that are
	// determined to be on the front and back, are once again linked lists, that 
	// are each processed recursively in the same manner. When the process is 
	// complete the entire BSP tree is built with exactly one node/leaf per wall

	static BSPNode *next_wall,  // pointer to next wall to be processed
				   *front_wall,    // the front wall
				   *back_wall,     // the back wall
				   *temp_wall;     // a temporary wall

	static float dot_wall_1,              // dot products for test wall
				 dot_wall_2,
				 wall_x0,wall_y0,wall_z0, // working vars for test wall
				 wall_x1,wall_y1,wall_z1,
				 pp_x0,pp_y0,pp_z0,       // working vars for partitioning plane
				 pp_x1,pp_y1,pp_z1,
				 xi,zi;                   // points of intersection when the partioning
										  // plane cuts a wall in two

	static vec4 test_vector_1,  // test vectors from the partioning plane
				test_vector_2;  // to the test wall to test the side
								// of the partioning plane the test wall
								// lies on

	static int front_flag = 0,      // flags if a wall is on the front or back
			   back_flag  = 0,      // of the partioning plane
			   index;               // looping index

	// SECTION 1 ////////////////////////////////////////////////////////////////

	// the root is the partitioning plane, partition the polygons using it
	next_wall  = link;
	link = NULL;

	// extract top two vertices of partioning plane wall for ease of calculations
	pp_x0 = wall.vlist[0].x;
	pp_y0 = wall.vlist[0].y;
	pp_z0 = wall.vlist[0].z;

	pp_x1 = wall.vlist[1].x;
	pp_y1 = wall.vlist[1].y;
	pp_z1 = wall.vlist[1].z;

	// SECTION 2  ////////////////////////////////////////////////////////////////

	// test if all walls have been partitioned
	while(next_wall) 
	{
		// test which side test wall is relative to partioning plane
		// defined by root

		 // first compute vectors from point on partioning plane to points on
		 // test wall
		 test_vector_1 = next_wall->wall.vlist[0].v - wall.vlist[0].v;
		 test_vector_2 = next_wall->wall.vlist[1].v - wall.vlist[0].v;

		 // now dot each test vector with the surface normal and analyze signs
		 // to determine half space
		 dot_wall_1 = test_vector_1.Dot(wall.normal);
		 dot_wall_2 = test_vector_2.Dot(wall.normal);

	// SECTION 3  ////////////////////////////////////////////////////////////////

		 // perform the tests

		 // case 0, the partioning plane and the test wall have a point in common
		 // this is a special case and must be accounted for, shorten the code
		 // we will set a pair of flags and then the next case will handle
		 // the actual insertion of the wall into BSP

		 // reset flags
		 front_flag = back_flag = 0;

		 // determine if wall is tangent to endpoints of partitioning wall
		 if (wall.vlist[0].v.Equal(next_wall->wall.vlist[0].v))
			{
			// p0 of partioning plane is the same at p0 of test wall
			// we only need to see what side p1 of test wall in on
			if (dot_wall_2 > 0)
			   front_flag = 1;
			else
			   back_flag = 1;

			} // end if
		 else
		 if (wall.vlist[0].v.Equal(next_wall->wall.vlist[1].v) )
			{
			// p0 of partioning plane is the same at p1 of test wall
			// we only need to see what side p0 of test wall in on
			if (dot_wall_1 > 0)
			   front_flag = 1;
			else
			   back_flag = 1;

			} // end if
		 else
		 if (wall.vlist[1].v.Equal(next_wall->wall.vlist[0].v) )
			{
			// p1 of partioning plane is the same at p0 of test wall
			// we only need to see what side p1 of test wall in on

			if (dot_wall_2 > 0)
			   front_flag = 1;
			else
			   back_flag = 1;

			} // end if
		 else
		 if (wall.vlist[1].v.Equal(next_wall->wall.vlist[1].v) )
			{
			// p1 of partioning plane is the same at p1 of test wall
			// we only need to see what side p0 of test wall in on

			if (dot_wall_1 > 0)
			   front_flag = 1;
			else
			   back_flag = 1;

			} // end if

	// SECTION 4  ////////////////////////////////////////////////////////////////

		 // case 1 both signs are the same or the front or back flag has been set
		 if ( (dot_wall_1 >= 0 && dot_wall_2 >= 0) || front_flag )
			{
			// place this wall on the front list
			if (front==NULL)
			   {
			   // this is the first node
			   front            = next_wall;
			   next_wall        = next_wall->link;
			   front_wall       = front;
			   front_wall->link = NULL;

			   } // end if
			else
			   {
			   // this is the nth node
			   front_wall->link = next_wall;
			   next_wall        = next_wall->link;
			   front_wall       = front_wall->link;
			   front_wall->link = NULL;

			   } // end else

			} // end if both positive

	// SECTION  5 ////////////////////////////////////////////////////////////////

		 else // back side sub-case
		 if ( (dot_wall_1 < 0 && dot_wall_2 < 0) || back_flag)
			{
			// place this wall on the back list
			if (back==NULL)
			   {
			   // this is the first node
			   back            = next_wall;
			   next_wall       = next_wall->link;
			   back_wall       = back;
			   back_wall->link = NULL;

			   } // end if
			else
			   {
			   // this is the nth node
			   back_wall->link = next_wall;
			   next_wall       = next_wall->link;
			   back_wall       = back_wall->link;
			   back_wall->link = NULL;

			   } // end else

			} // end if both negative

		 // case 2 both signs are different, we must partition the wall

	// SECTION 6  ////////////////////////////////////////////////////////////////

		 else
		 if ( (dot_wall_1 < 0 && dot_wall_2 >= 0) ||
			  (dot_wall_1 >= 0 && dot_wall_2 < 0))
			{
			// the partioning plane cuts the wall in half, the wall
			// must be split into two walls

			// extract top two vertices of test wall for ease of calculations
			wall_x0 = next_wall->wall.vlist[0].x;
			wall_y0 = next_wall->wall.vlist[0].y;
			wall_z0 = next_wall->wall.vlist[0].z;

			wall_x1 = next_wall->wall.vlist[1].x;
			wall_y1 = next_wall->wall.vlist[1].y;
			wall_z1 = next_wall->wall.vlist[1].z;

			// compute the point of intersection between the walls
			// note that the intersection takes place in the x-z plane 
			IntersectLines(wall_x0, wall_z0, wall_x1, wall_z1,
						   pp_x0,   pp_z0,   pp_x1,   pp_z1,
						   &xi, &zi);

			// here comes the tricky part, we need to split the wall in half and
			// create two new walls. We'll do this by creating two new walls,
			// placing them on the appropriate front and back lists and
			// then deleting the original wall (hold your breath)...

			// process first wall...

			// allocate the memory for the wall
			temp_wall = (BSPNode*)malloc(sizeof(BSPNode));

			// set links
			temp_wall->front = NULL;
			temp_wall->back  = NULL;
			temp_wall->link  = NULL;

			// poly normal is the same
			temp_wall->wall.normal  = next_wall->wall.normal;
			temp_wall->wall.nlength = next_wall->wall.nlength;

			// poly color is the same
			temp_wall->wall.color = next_wall->wall.color;

			// poly material is the same
			temp_wall->wall.mati = next_wall->wall.mati;

			// poly texture is the same
			temp_wall->wall.texture = next_wall->wall.texture;

			// poly attributes are the same
			temp_wall->wall.attr = next_wall->wall.attr;

			// poly states are the same
			temp_wall->wall.state = next_wall->wall.state;

			temp_wall->id          = next_wall->id + WALL_SPLIT_ID; // add factor denote a split

			// compute wall vertices
			for (index = 0; index < 4; index++)
				{
				temp_wall->wall.vlist[index].x = next_wall->wall.vlist[index].x;
				temp_wall->wall.vlist[index].y = next_wall->wall.vlist[index].y;
				temp_wall->wall.vlist[index].z = next_wall->wall.vlist[index].z;
				temp_wall->wall.vlist[index].w = 1;

				// copy vertex attributes, texture coordinates, normal
				temp_wall->wall.vlist[index].attr = next_wall->wall.vlist[index].attr;
				temp_wall->wall.vlist[index].n    = next_wall->wall.vlist[index].n;
				temp_wall->wall.vlist[index].t    = next_wall->wall.vlist[index].t;
				} // end for index

			// now modify vertices 1 and 2 to reflect intersection point
			// but leave y alone since it's invariant for the wall spliting
			temp_wall->wall.vlist[1].x = xi;
			temp_wall->wall.vlist[1].z = zi;

			temp_wall->wall.vlist[2].x = xi;
			temp_wall->wall.vlist[2].z = zi;

	// SECTION 7  ////////////////////////////////////////////////////////////////

			// insert new wall into front or back of root
			if (dot_wall_1 >= 0)
			   {
			   // place this wall on the front list
			   if (front==NULL)
				  {
				  // this is the first node
				  front            = temp_wall;
				  front_wall       = front;
				  front_wall->link = NULL;

				  } // end if
			   else
				  {
				  // this is the nth node
				  front_wall->link = temp_wall;
				  front_wall       = front_wall->link;
				  front_wall->link = NULL;

				  } // end else

			   } // end if positive
			else
			if (dot_wall_1 < 0)
			   {
			   // place this wall on the back list
			   if (back==NULL)
				  {
				  // this is the first node
				  back            = temp_wall;
				  back_wall       = back;
				  back_wall->link = NULL;

				  } // end if
			   else
				  {
				  // this is the nth node
				  back_wall->link = temp_wall;
				  back_wall       = back_wall->link;
				  back_wall->link = NULL;

				  } // end else

			   } // end if negative

	// SECTION  8 ////////////////////////////////////////////////////////////////

			// process second wall...

			// allocate the memory for the wall
			temp_wall = (BSPNode*)malloc(sizeof(BSPNode));

			// set links
			temp_wall->front = NULL;
			temp_wall->back  = NULL;
			temp_wall->link  = NULL;

			// poly normal is the same
			temp_wall->wall.normal  = next_wall->wall.normal;
			temp_wall->wall.nlength = next_wall->wall.nlength;

			// poly color is the same
			temp_wall->wall.color = next_wall->wall.color;

			// poly material is the same
			temp_wall->wall.mati = next_wall->wall.mati;

			// poly texture is the same
			temp_wall->wall.texture = next_wall->wall.texture;

			// poly attributes are the same
			temp_wall->wall.attr = next_wall->wall.attr;

			// poly states are the same
			temp_wall->wall.state = next_wall->wall.state;

			temp_wall->id          = next_wall->id + WALL_SPLIT_ID; // add factor denote a split

			// compute wall vertices
			for (index=0; index < 4; index++)
				{
				temp_wall->wall.vlist[index].x = next_wall->wall.vlist[index].x;
				temp_wall->wall.vlist[index].y = next_wall->wall.vlist[index].y;
				temp_wall->wall.vlist[index].z = next_wall->wall.vlist[index].z;
				temp_wall->wall.vlist[index].w = 1;

				// copy vertex attributes, texture coordinates, normal
				temp_wall->wall.vlist[index].attr = next_wall->wall.vlist[index].attr;
				temp_wall->wall.vlist[index].n    = next_wall->wall.vlist[index].n;
				temp_wall->wall.vlist[index].t    = next_wall->wall.vlist[index].t;

				} // end for index

			// now modify vertices 0 and 3 to reflect intersection point
			// but leave y alone since it's invariant for the wall spliting
			temp_wall->wall.vlist[0].x = xi;
			temp_wall->wall.vlist[0].z = zi;

			temp_wall->wall.vlist[3].x = xi;
			temp_wall->wall.vlist[3].z = zi;

			// insert new wall into front or back of root
			if (dot_wall_2 >= 0)
			   {
			   // place this wall on the front list
			   if (front==NULL)
				  {
				  // this is the first node
				  front            = temp_wall;
				  front_wall       = front;
				  front_wall->link = NULL;

				  } // end if
			   else
				  {
				  // this is the nth node
				  front_wall->link = temp_wall;
				  front_wall       = front_wall->link;
				  front_wall->link = NULL;

				  } // end else


			   } // end if positive
			else
			if (dot_wall_2 < 0)
			   {
			   // place this wall on the back list
			   if (back==NULL)
				  {
				  // this is the first node
				  back            = temp_wall;
				  back_wall       = back;
				  back_wall->link = NULL;

				  } // end if
			   else
				  {
				  // this is the nth node
				  back_wall->link = temp_wall;
				  back_wall       = back_wall->link;
				  back_wall->link = NULL;

				  } // end else

			   } // end if negative

	// SECTION  9  ////////////////////////////////////////////////////////////////

			// we are now done splitting the wall, so we can delete it
			temp_wall = next_wall;
			next_wall = next_wall->link;

			// release the memory
			free(temp_wall);

			} // end else

		 } // end while

	// SECTION  10 ////////////////////////////////////////////////////////////////

	// recursively process front and back walls/sub-trees
	if (front) front->Build();
	if (back) back->Build();
}

void BSPNode::Transform(const mat4& mt, int coord_select)
{
	// this function traverses the bsp tree and applies the transformation
	// matrix to each node, the function is of course recursive and uses 
	// and inorder traversal, other traversals such as preorder and postorder 
	// will would just as well...

	// transform back most sub-tree
	if (back) back->Transform(mt, coord_select);

	// iterate thru all vertices of current wall and transform them into
	// camera coordinates

	// what coordinates should be transformed?
	switch(coord_select)
		  {
		  case TRANSFORM_LOCAL_ONLY:
			   {
			   // transform each local/model vertex of the object mesh in place
			   for (int vertex = 0; vertex < 4; vertex++)
				  {
				  // transform point
				  wall.vlist[vertex].v = mt * wall.vlist[vertex].v;
	 
				  // transform vertex normal if needed
				  if (wall.vlist[vertex].attr & VERTEX_ATTR_NORMAL)
					  wall.vlist[vertex].n = mt * wall.vlist[vertex].n;
				   } // end for index
				} break;
	 
		  case TRANSFORM_TRANS_ONLY:
			   {
			   // transform each "transformed" vertex of the object mesh in place
			   // remember, the idea of the vlist_trans[] array is to accumulate
			   // transformations
			   for (int vertex = 0; vertex < 4; vertex++)
				  {
				  // transform point
				  wall.tvlist[vertex].v = mt * wall.tvlist[vertex].v;	 
				  // transform vertex normal if needed
				  if (wall.tvlist[vertex].attr & VERTEX_ATTR_NORMAL)
					  wall.tvlist[vertex].n = mt * wall.tvlist[vertex].n;
				   } // end for index
			   } break;

		   case TRANSFORM_LOCAL_TO_TRANS:
				{
				// transform each local/model vertex of the object mesh and store result
				// in "transformed" vertex list
				for (int vertex=0; vertex < 4; vertex++)
					{
					// transform point
					wall.tvlist[vertex].v = mt * wall.vlist[vertex].v;
					// transform vertex normal if needed
					if (wall.tvlist[vertex].attr & VERTEX_ATTR_NORMAL)
						wall.tvlist[vertex].n = mt * wall.vlist[vertex].n;
					 } // end for index
			   } break;

		  default: break;

		  } // end switch

	// transform front most sub-tree
	if (front) front->Transform(mt, coord_select);
}

void BSPNode::InsertionTraversal(RenderList& list, const Camera& cam, int insert_local)
{
	// converts the entire bsp tree into a face list and then inserts
	// the visible, active, non-clipped, non-culled polygons into
	// the render list, also note the flag insert_local control 
	// whether or not the vlist_local or vlist_trans vertex list
	// is used, thus you can insert a bsp tree "raw" totally untranformed
	// if you set insert_local to 1, default is 0, that is you would
	// only insert an object after at least the local to world transform

	// the functions walks the tree recursively in back to front order
	// relative to the viewpoint sent in cam, the bsp must be in world coordinates
	// for this to work correctly

	// this function works be testing the viewpoint against the current wall
	// in the bsp, then depending on the side the viewpoint is the algorithm
	// proceeds. the search takes place as the rest in an "inorder" method
	// with hooks to process and add each node into the polygon list at the
	// right time

	static vec4 test_vector;
	static float dot_wall;

	// SECTION 1  ////////////////////////////////////////////////////////////////

	//Write_Error("\nEntering Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

	//Write_Error("\nRoot was valid...");

	// test which side viewpoint is on relative to the current wall
	test_vector = cam.Pos() - wall.vlist[0].v;

	// now dot test vector with the surface normal and analyze signs
	dot_wall = test_vector.Dot(wall.normal);

	//Write_Error("\nTesting dot product...");

	// SECTION 2  ////////////////////////////////////////////////////////////////

	// if the sign of the dot product is positive then the viewer on on the
	// front side of current wall, so recursively process the walls behind then
	// in front of this wall, else do the opposite
	if (dot_wall > 0)
	   {
	   // viewer is in front of this wall
	   //Write_Error("\nDot > 0, front side...");

	   // process the back wall sub tree
	   if (back) 
		   back->InsertionTraversal(list, cam, insert_local);

	   // split quad into (2) triangles for insertion
	   PolygonF poly1, poly2;

	   // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
	   // the later has (4) vertices rather than (3), thus we are going to
	   // create (2) triangles from the single quad :)
	   // copy fields that are important
	   poly1.state   = wall.state;      // state information
	   poly1.attr    = wall.attr;       // physical attributes of polygon
	   poly1.color   = wall.color;      // color of polygon
	   poly1.texture = wall.texture;    // pointer to the texture information for simple texture mapping
	   poly1.mati    = wall.mati;       // material index (-1) for no material  (new)
	   poly1.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
	   poly1.normal  = wall.normal;     // the general polygon normal (new)

	   poly2.state   = wall.state;      // state information
	   poly2.attr    = wall.attr;       // physical attributes of polygon
	   poly2.color   = wall.color;      // color of polygon
	   poly2.texture = wall.texture;    // pointer to the texture information for simple texture mapping
	   poly2.mati    = wall.mati;       // material index (-1) for no material  (new)
	   poly2.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
	   poly2.normal  = wall.normal;     // the general polygon normal (new)

	   // now the vertices, they currently look like this
	   // v0      v1
	   //
	   //
	   // v3      v2
	   // we want to create (2) triangles that look like this
	   //    poly 1           poly2
	   // v0      v1                v1  
	   // 
	   //        
	   //
	   // v3                v3      v2        
	   // 
	   // where the winding order of poly 1 is v0,v1,v3 and the winding order
	   // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
	   if (insert_local==1)
		  {
		  // polygon 1
		  poly1.vlist[0]  = wall.vlist[0];  // the vertices of this triangle 
		  poly1.tvlist[0] = wall.vlist[0];  // the vertices of this triangle    

		  poly1.vlist[1]  = wall.vlist[1];  // the vertices of this triangle 
		  poly1.tvlist[1] = wall.vlist[1];  // the vertices of this triangle  

		  poly1.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
		  poly1.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  

		  // polygon 2
		  poly2.vlist[0]  = wall.vlist[1];  // the vertices of this triangle 
		  poly2.tvlist[0] = wall.vlist[1];  // the vertices of this triangle    

		  poly2.vlist[1]  = wall.vlist[2];  // the vertices of this triangle 
		  poly2.tvlist[1] = wall.vlist[2];  // the vertices of this triangle  

		  poly2.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
		  poly2.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  
		  } // end if
	   else
		  {
		  // polygon 1
		  poly1.vlist[0]  = wall.vlist[0];   // the vertices of this triangle 
		  poly1.tvlist[0] = wall.tvlist[0];  // the vertices of this triangle    

		  poly1.vlist[1]  = wall.vlist[1];   // the vertices of this triangle 
		  poly1.tvlist[1] = wall.tvlist[1];  // the vertices of this triangle  

		  poly1.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
		  poly1.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  

		  // polygon 2
		  poly2.vlist[0]  = wall.vlist[1];   // the vertices of this triangle 
		  poly2.tvlist[0] = wall.tvlist[1];  // the vertices of this triangle    

		  poly2.vlist[1]  = wall.vlist[2];   // the vertices of this triangle 
		  poly2.tvlist[1] = wall.tvlist[2];  // the vertices of this triangle  

		  poly2.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
		  poly2.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  
		  } // end if

		//Write_Error("\nInserting polygons...");

	   // insert polygons into rendering list
	   list.Insert(poly1);
	   list.Insert(poly2);

	   // now process the front walls sub tree
	   if (front) 
		   front->InsertionTraversal(list, cam, insert_local);

	   } // end if

	// SECTION 3 ////////////////////////////////////////////////////////////////

	else
	   {
	   // viewer is behind this wall
	   //Write_Error("\nDot < 0, back side...");

	   // process the back wall sub tree
	   if (front) 
		   front->InsertionTraversal(list, cam, insert_local);

	   // split quad into (2) triangles for insertion
	   PolygonF poly1, poly2;

	   // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
	   // the later has (4) vertices rather than (3), thus we are going to
	   // create (2) triangles from the single quad :)
	   // copy fields that are important
	   poly1.state   = wall.state;      // state information
	   poly1.attr    = wall.attr;       // physical attributes of polygon
	   poly1.color   = wall.color;      // color of polygon
	   poly1.texture = wall.texture;    // pointer to the texture information for simple texture mapping
	   poly1.mati    = wall.mati;       // material index (-1) for no material  (new)
	   poly1.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
	   poly1.normal  = wall.normal;     // the general polygon normal (new)

	   poly2.state   = wall.state;      // state information
	   poly2.attr    = wall.attr;       // physical attributes of polygon
	   poly2.color   = wall.color;      // color of polygon
	   poly2.texture = wall.texture;    // pointer to the texture information for simple texture mapping
	   poly2.mati    = wall.mati;       // material index (-1) for no material  (new)
	   poly2.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
	   poly2.normal  = wall.normal;     // the general polygon normal (new)

	   // now the vertices, they currently look like this
	   // v0      v1
	   //
	   //
	   // v3      v2
	   // we want to create (2) triangles that look like this
	   //    poly 1           poly2
	   // v0      v1                v1  
	   // 
	   //        
	   //
	   // v3                v3      v2        
	   // 
	   // where the winding order of poly 1 is v0,v1,v3 and the winding order
	   // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
	   if (insert_local==1)
		  {
		  // polygon 1
		  poly1.vlist[0]  = wall.vlist[0];  // the vertices of this triangle 
		  poly1.tvlist[0] = wall.vlist[0];  // the vertices of this triangle    

		  poly1.vlist[1]  = wall.vlist[1];  // the vertices of this triangle 
		  poly1.tvlist[1] = wall.vlist[1];  // the vertices of this triangle  

		  poly1.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
		  poly1.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  

		  // polygon 2
		  poly2.vlist[0]  = wall.vlist[1];  // the vertices of this triangle 
		  poly2.tvlist[0] = wall.vlist[1];  // the vertices of this triangle    

		  poly2.vlist[1]  = wall.vlist[2];  // the vertices of this triangle 
		  poly2.tvlist[1] = wall.vlist[2];  // the vertices of this triangle  

		  poly2.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
		  poly2.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  
		  } // end if
	   else
		  {
		  // polygon 1
		  poly1.vlist[0]  = wall.vlist[0];   // the vertices of this triangle 
		  poly1.tvlist[0] = wall.tvlist[0];  // the vertices of this triangle    

		  poly1.vlist[1]  = wall.vlist[1];   // the vertices of this triangle 
		  poly1.tvlist[1] = wall.tvlist[1];  // the vertices of this triangle  

		  poly1.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
		  poly1.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  

		  // polygon 2
		  poly2.vlist[0]  = wall.vlist[1];   // the vertices of this triangle 
		  poly2.tvlist[0] = wall.tvlist[1];  // the vertices of this triangle    

		  poly2.vlist[1]  = wall.vlist[2];   // the vertices of this triangle 
		  poly2.tvlist[1] = wall.tvlist[2];  // the vertices of this triangle  

		  poly2.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
		  poly2.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  
		  } // end if

	   //Write_Error("\nInserting polygons...");

	   // insert polygons into rendering list
	   list.Insert(poly1);
	   list.Insert(poly2);

	   // now process the front walls sub tree
	   if (back) 
		   back->InsertionTraversal(list, cam, insert_local);

	   } // end else

	//Write_Error("\nExiting Bsp_Insertion_Traversal_RENDERLIST4DV2()...");
}

void BSPNode::InsertionTraversalRemoveBF(RenderList& list, const Camera& cam, int insert_local)
{
	// converts the entire bsp tree into a face list and then inserts
	// the visible, active, non-clipped, non-culled polygons into
	// the render list, also note the flag insert_local control 
	// whether or not the vlist_local or vlist_trans vertex list
	// is used, thus you can insert a bsp tree "raw" totally untranformed
	// if you set insert_local to 1, default is 0, that is you would
	// only insert an object after at least the local to world transform

	// the functions walks the tree recursively in back to front order
	// relative to the viewpoint sent in cam, the bsp must be in world coordinates
	// for this to work correctly

	// this function works be testing the viewpoint against the current wall
	// in the bsp, then depending on the side the viewpoint is the algorithm
	// proceeds. the search takes place as the rest in an "inorder" method
	// with hooks to process and add each node into the polygon list at the
	// right time
	// additionally the function tests for backfaces on the fly and only inserts
	// polygons that are not backfacing the viewpoint
	// also, it's up to the caller to make sure that cam.n has the valid look at 
	// view vector for euler or UVN model

	static vec4 test_vector;
	static float dot_wall;

	// SECTION 1  ////////////////////////////////////////////////////////////////

	//Write_Error("\nEntering Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

	//Write_Error("\nTesting root...");

	//Write_Error("\nRoot was valid...");

	// test which side viewpoint is on relative to the current wall
	test_vector = cam.Pos() - wall.vlist[0].v;

	// now dot test vector with the surface normal and analyze signs
	dot_wall = test_vector.Dot(wall.normal);

	//Write_Error("\nTesting dot product...");

	// SECTION 2  ////////////////////////////////////////////////////////////////

	// if the sign of the dot product is positive then the viewer on on the
	// front side of current wall, so recursively process the walls behind then
	// in front of this wall, else do the opposite
	if (dot_wall > 0)
	{
		// viewer is in front of this wall
		//Write_Error("\nDot > 0, front side...");

		// process the back wall sub tree
		if (back)
			back->InsertionTraversalRemoveBF(list, cam, insert_local);

		// we want to cull polygons that can't be seen from the current viewpoint
		// based not only on the view position, but the viewing angle, hence
		// we first determine if we are on the front or back side of the partitioning
		// plane, then we compute the angle theta from the partitioning plane
		// normal to the viewing direction the player is currently pointing
		// then we ask the question given the field of view and the current
		// viewing direction, can the player see this plane? the answer boils down
		// to the following tests
		// front side test:
		// if theta > (90 + fov/2)
		// and for back side:
		// if theta < (90 - fov/2)
		// to compute theta we need this
		// u . v = |u| * |v| * cos theta
		// since |u| = |v| = 1, we can write
		// u . v = cos theta, hence using the inverse cosine we can get the angle
		// theta = arccosine(u . v)
		// we use the lookup table created with the value of u . v : [-1,1] scaled to
		// 0  to 180 (or 360 for .5 degree accuracy) to compute the angle which is 
		// ALWAYS from 0 - 180, the 360 table just has more accurate entries.
		float dp = cam.Normal().Dot(wall.normal);

		// polygon is visible if this is true
		if (FAST_INV_COS(dp) > (90 - cam.FOV()/2) )
		{
			//////////////////////////////////////////////////////////////////////////// 
			// split quad into (2) triangles for insertion
			PolygonF poly1, poly2;

			// the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
			// the later has (4) vertices rather than (3), thus we are going to
			// create (2) triangles from the single quad :)
			// copy fields that are important
			poly1.state   = wall.state;      // state information
			poly1.attr    = wall.attr;       // physical attributes of polygon
			poly1.color   = wall.color;      // color of polygon
			poly1.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly1.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly1.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly1.normal  = wall.normal;     // the general polygon normal (new)

			poly2.state   = wall.state;      // state information
			poly2.attr    = wall.attr;       // physical attributes of polygon
			poly2.color   = wall.color;      // color of polygon
			poly2.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly2.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly2.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly2.normal  = wall.normal;     // the general polygon normal (new)

			// now the vertices, they currently look like this
			// v0      v1
			//
			//
			// v3      v2
			// we want to create (2) triangles that look like this
			//    poly 1           poly2
			// v0      v1                v1  
			// 
			//        
			//
			// v3                v3      v2        
			// 
			// where the winding order of poly 1 is v0,v1,v3 and the winding order
			// of poly 2 is v1, v2, v3 to keep with our clockwise conventions
			if (insert_local==1)
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];  // the vertices of this triangle 
				poly1.tvlist[0] = wall.vlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];  // the vertices of this triangle 
				poly1.tvlist[1] = wall.vlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly1.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];  // the vertices of this triangle 
				poly2.tvlist[0] = wall.vlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];  // the vertices of this triangle 
				poly2.tvlist[1] = wall.vlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly2.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  
			} // end if
			else
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];   // the vertices of this triangle 
				poly1.tvlist[0] = wall.tvlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];   // the vertices of this triangle 
				poly1.tvlist[1] = wall.tvlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly1.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];   // the vertices of this triangle 
				poly2.tvlist[0] = wall.tvlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];   // the vertices of this triangle 
				poly2.tvlist[1] = wall.tvlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly2.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  
			} // end if

			//Write_Error("\nInserting polygons...");

			// insert polygons into rendering list
			list.Insert(poly1);
			list.Insert(poly2);
			////////////////////////////////////////////////////////////////////////////
		} // end if visible

		// now process the front walls sub tree
		if (front)
			front->InsertionTraversalRemoveBF(list, cam, insert_local);

	} // end if

	// SECTION 3 ////////////////////////////////////////////////////////////////

	else
	{
		// viewer is behind this wall
		//Write_Error("\nDot < 0, back side...");

		// process the back wall sub tree
		if (front)
			front->InsertionTraversalRemoveBF(list, cam, insert_local);

		// review explanation above...
		float dp = cam.Normal().Dot(wall.normal);

		// polygon is visible if this is true
		if (FAST_INV_COS(dp) < (90 + cam.FOV()/2) )
		{
			//////////////////////////////////////////////////////////////////////////// 

			// split quad into (2) triangles for insertion
			PolygonF poly1, poly2;

			// the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
			// the later has (4) vertices rather than (3), thus we are going to
			// create (2) triangles from the single quad :)
			// copy fields that are important
			poly1.state   = wall.state;      // state information
			poly1.attr    = wall.attr;       // physical attributes of polygon
			poly1.color   = wall.color;      // color of polygon
			poly1.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly1.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly1.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly1.normal  = wall.normal;     // the general polygon normal (new)

			poly2.state   = wall.state;      // state information
			poly2.attr    = wall.attr;       // physical attributes of polygon
			poly2.color   = wall.color;      // color of polygon
			poly2.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly2.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly2.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly2.normal  = wall.normal;     // the general polygon normal (new)

			// now the vertices, they currently look like this
			// v0      v1
			//
			//
			// v3      v2
			// we want to create (2) triangles that look like this
			//    poly 1           poly2
			// v0      v1                v1  
			// 
			//        
			//
			// v3                v3      v2        
			// 
			// where the winding order of poly 1 is v0,v1,v3 and the winding order
			// of poly 2 is v1, v2, v3 to keep with our clockwise conventions
			if (insert_local==1)
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];  // the vertices of this triangle 
				poly1.tvlist[0] = wall.vlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];  // the vertices of this triangle 
				poly1.tvlist[1] = wall.vlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly1.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];  // the vertices of this triangle 
				poly2.tvlist[0] = wall.vlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];  // the vertices of this triangle 
				poly2.tvlist[1] = wall.vlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly2.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  
			} // end if
			else
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];   // the vertices of this triangle 
				poly1.tvlist[0] = wall.tvlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];   // the vertices of this triangle 
				poly1.tvlist[1] = wall.tvlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly1.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];   // the vertices of this triangle 
				poly2.tvlist[0] = wall.tvlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];   // the vertices of this triangle 
				poly2.tvlist[1] = wall.tvlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly2.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  
			} // end if

			//Write_Error("\nInserting polygons...");

			// insert polygons into rendering list
			list.Insert(poly1);
			list.Insert(poly2);
			////////////////////////////////////////////////////////////////////////////
		} // end if visible

		// now process the front walls sub tree
		if (back)
			back->InsertionTraversalRemoveBF(list, cam, insert_local);

	} // end else

	//Write_Error("\nExiting Bsp_Insertion_Traversal_RENDERLIST4DV2()...");
}

void BSPNode::InsertionTraversalFrustrumCull(RenderList& list, const Camera& cam, int insert_local)
{
	// converts the entire bsp tree into a face list and then inserts
	// the visible, active, non-clipped, non-culled polygons into
	// the render list, also note the flag insert_local control 
	// whether or not the vlist_local or vlist_trans vertex list
	// is used, thus you can insert a bsp tree "raw" totally untranformed
	// if you set insert_local to 1, default is 0, that is you would
	// only insert an object after at least the local to world transform

	// the functions walks the tree recursively in back to front order
	// relative to the viewpoint sent in cam, the bsp must be in world coordinates
	// for this to work correctly

	// this function works be testing the viewpoint against the current wall
	// in the bsp, then depending on the side the viewpoint is the algorithm
	// proceeds. the search takes place as the rest in an "inorder" method
	// with hooks to process and add each node into the polygon list at the
	// right time
	// additionally the function cull the BSP on the fly and only inserts
	// polygons that are not backfacing the viewpoint
	// also, it's up to the caller to make sure that cam->n has the valid look at 
	// view vector for euler or UVN model

	static vec4 test_vector;
	static float dot_wall;

	// SECTION 1  ////////////////////////////////////////////////////////////////

	//Write_Error("\nEntering Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

	//Write_Error("\nRoot was valid...");

	// test which side viewpoint is on relative to the current wall
	test_vector = cam.Pos() - wall.vlist[0].v;

	// now dot test vector with the surface normal and analyze signs
	dot_wall = test_vector.Dot(wall.normal);

	//Write_Error("\nTesting dot product...");

	// SECTION 2  ////////////////////////////////////////////////////////////////

	// if the sign of the dot product is positive then the viewer on on the
	// front side of current wall, so recursively process the walls behind then
	// in front of this wall, else do the opposite
	if (dot_wall > 0)
	{
		// viewer is in front of this wall
		//Write_Error("\nDot > 0, front side...");

		// we want to cull sub spaces that can't be seen from the current viewpoint
		// based not only on the view position, but the viewing angle, hence
		// we first determine if we are on the front or back side of the partitioning
		// plane, then we compute the angle theta from the partitioning plane
		// normal to the viewing direction the player is currently pointing
		// then we ask the question given the field of view and the current
		// viewing direction, can the player see this plane? the answer boils down
		// to the following tests
		// front side test:
		// if theta > (90 + fov/2)
		// and for back side:
		// if theta < (90 - fov/2)
		// to compute theta we need this
		// u . v = |u| * |v| * cos theta
		// since |u| = |v| = 1, we can write
		// u . v = cos theta, hence using the inverse cosine we can get the angle
		// theta = arccosine(u . v)
		// we use the lookup table created with the value of u . v : [-1,1] scaled to
		// 0  to 180 (or 360 for .5 degree accuracy) to compute the angle which is 
		// ALWAYS from 0 - 180, the 360 table just has more accurate entries.
		float dp = cam.Normal().Dot(wall.normal);

		// polygon is visible if this is true
		if (FAST_INV_COS(dp) > (90 - cam.FOV()/2) )
		{
			// process the back wall sub tree
			if (back)
				back->InsertionTraversalFrustrumCull(list, cam, insert_local);

			//////////////////////////////////////////////////////////////////////////// 
			// split quad into (2) triangles for insertion
			PolygonF poly1, poly2;

			// the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
			// the later has (4) vertices rather than (3), thus we are going to
			// create (2) triangles from the single quad :)
			// copy fields that are important
			poly1.state   = wall.state;      // state information
			poly1.attr    = wall.attr;       // physical attributes of polygon
			poly1.color   = wall.color;      // color of polygon
			poly1.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly1.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly1.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly1.normal  = wall.normal;     // the general polygon normal (new)

			poly2.state   = wall.state;      // state information
			poly2.attr    = wall.attr;       // physical attributes of polygon
			poly2.color   = wall.color;      // color of polygon
			poly2.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly2.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly2.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly2.normal  = wall.normal;     // the general polygon normal (new)

			// now the vertices, they currently look like this
			// v0      v1
			//
			//
			// v3      v2
			// we want to create (2) triangles that look like this
			//    poly 1           poly2
			// v0      v1                v1  
			// 
			//        
			//
			// v3                v3      v2        
			// 
			// where the winding order of poly 1 is v0,v1,v3 and the winding order
			// of poly 2 is v1, v2, v3 to keep with our clockwise conventions
			if (insert_local==1)
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];  // the vertices of this triangle 
				poly1.tvlist[0] = wall.vlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];  // the vertices of this triangle 
				poly1.tvlist[1] = wall.vlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly1.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];  // the vertices of this triangle 
				poly2.tvlist[0] = wall.vlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];  // the vertices of this triangle 
				poly2.tvlist[1] = wall.vlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly2.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  
			} // end if
			else
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];   // the vertices of this triangle 
				poly1.tvlist[0] = wall.tvlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];   // the vertices of this triangle 
				poly1.tvlist[1] = wall.tvlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly1.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];   // the vertices of this triangle 
				poly2.tvlist[0] = wall.tvlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];   // the vertices of this triangle 
				poly2.tvlist[1] = wall.tvlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly2.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  
			} // end if

			//Write_Error("\nInserting polygons...");

			// insert polygons into rendering list
			list.Insert(poly1);
			list.Insert(poly2);
			////////////////////////////////////////////////////////////////////////////
		} // end if visible

		// now process the front walls sub tree
		if (front)
			front->InsertionTraversalFrustrumCull(list, cam, insert_local);

	} // end if

	// SECTION 3 ////////////////////////////////////////////////////////////////

	else
	{
		// viewer is behind this wall
		//Write_Error("\nDot < 0, back side...");


		// review explanation above...
		float dp = cam.Normal().Dot(wall.normal);

		// polygon is visible if this is true
		if (FAST_INV_COS(dp) < (90 + cam.FOV()/2) )
		{
			// process the back wall sub tree
			if (front)
				front->InsertionTraversalFrustrumCull(list, cam, insert_local);

			//////////////////////////////////////////////////////////////////////////// 

			// split quad into (2) triangles for insertion
			PolygonF poly1, poly2;

			// the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
			// the later has (4) vertices rather than (3), thus we are going to
			// create (2) triangles from the single quad :)
			// copy fields that are important
			poly1.state   = wall.state;      // state information
			poly1.attr    = wall.attr;       // physical attributes of polygon
			poly1.color   = wall.color;      // color of polygon
			poly1.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly1.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly1.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly1.normal  = wall.normal;     // the general polygon normal (new)

			poly2.state   = wall.state;      // state information
			poly2.attr    = wall.attr;       // physical attributes of polygon
			poly2.color   = wall.color;      // color of polygon
			poly2.texture = wall.texture;    // pointer to the texture information for simple texture mapping
			poly2.mati    = wall.mati;       // material index (-1) for no material  (new)
			poly2.nlength = wall.nlength;    // length of the polygon normal if not normalized (new)
			poly2.normal  = wall.normal;     // the general polygon normal (new)

			// now the vertices, they currently look like this
			// v0      v1
			//
			//
			// v3      v2
			// we want to create (2) triangles that look like this
			//    poly 1           poly2
			// v0      v1                v1  
			// 
			//        
			//
			// v3                v3      v2        
			// 
			// where the winding order of poly 1 is v0,v1,v3 and the winding order
			// of poly 2 is v1, v2, v3 to keep with our clockwise conventions
			if (insert_local==1)
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];  // the vertices of this triangle 
				poly1.tvlist[0] = wall.vlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];  // the vertices of this triangle 
				poly1.tvlist[1] = wall.vlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly1.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];  // the vertices of this triangle 
				poly2.tvlist[0] = wall.vlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];  // the vertices of this triangle 
				poly2.tvlist[1] = wall.vlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];  // the vertices of this triangle 
				poly2.tvlist[2] = wall.vlist[3];  // the vertices of this triangle  
			} // end if
			else
			{
				// polygon 1
				poly1.vlist[0]  = wall.vlist[0];   // the vertices of this triangle 
				poly1.tvlist[0] = wall.tvlist[0];  // the vertices of this triangle    

				poly1.vlist[1]  = wall.vlist[1];   // the vertices of this triangle 
				poly1.tvlist[1] = wall.tvlist[1];  // the vertices of this triangle  

				poly1.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly1.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  

				// polygon 2
				poly2.vlist[0]  = wall.vlist[1];   // the vertices of this triangle 
				poly2.tvlist[0] = wall.tvlist[1];  // the vertices of this triangle    

				poly2.vlist[1]  = wall.vlist[2];   // the vertices of this triangle 
				poly2.tvlist[1] = wall.tvlist[2];  // the vertices of this triangle  

				poly2.vlist[2]  = wall.vlist[3];   // the vertices of this triangle 
				poly2.tvlist[2] = wall.tvlist[3];  // the vertices of this triangle  
			} // end if

			//Write_Error("\nInserting polygons...");

			// insert polygons into rendering list
			list.Insert(poly1);
			list.Insert(poly2);
			////////////////////////////////////////////////////////////////////////////
		} // end if visible

		// now process the front walls sub tree
		if (back)
			back->InsertionTraversalFrustrumCull(list, cam, insert_local);

	} // end else

	//Write_Error("\nExiting Bsp_Insertion_Traversal_RENDERLIST4DV2()...");
}

//////////////////////////////////////////////////////////////////////////
// class BSP
//////////////////////////////////////////////////////////////////////////

void BSP::Translate(const vec4& trans)
{
	_root->Translate(trans);
}

void BSP::Delete()
{
	_root->Delete();
}

void BSP::Print()
{
	_root->Print();
}

void BSP::BuildTree()
{
	_root->Build();
}

void BSP::Transform(const mat4& mt, int coord_select)
{
	_root->Transform(mt, coord_select);
}

void BSP::InsertionTraversal(RenderList& list, const Camera& cam, int insert_local)
{
	_root->InsertionTraversal(list, cam, insert_local);
}

void BSP::InsertionTraversalRemoveBF(RenderList& list, const Camera& cam, int insert_local)
{
	_root->InsertionTraversalRemoveBF(list, cam, insert_local);
}

void BSP::InsertionTraversalFrustrumCull(RenderList& list, const Camera& cam, int insert_local)
{
	_root->InsertionTraversalFrustrumCull(list, cam, insert_local);
}

}