#pragma once

#include "Polygon.h"
#include "Matrix.h"

namespace t3d {

#define WALL_SPLIT_ID                  4096   // used to tag walls that are split

class RenderList;
class Camera;

class BSPNode
{
public:
	BSPNode();

	void Translate(const vec4& trans);

	void Delete();

	void Print();

	void Build();

	void Transform(const mat4& mt, int coord_select);

	void InsertionTraversal(RenderList& list, const Camera& cam, int insert_local);
	void InsertionTraversalRemoveBF(RenderList& list, const Camera& cam, int insert_local);
	void InsertionTraversalFrustrumCull(RenderList& list, const Camera& cam, int insert_local);

public:
	int        id;    // id tage for debugging
	PolygonQF  wall;  // the actual wall quad

	BSPNode *link;  // pointer to next wall
	BSPNode *front; // pointer to walls in front
	BSPNode *back;  // pointer to walls behind

}; // BSPNode

class BSP
{
public:
	void Translate(const vec4& trans);

	void Delete();

	void Print();

	void BuildTree();

	void Transform(const mat4& mt, int coord_select);

	void InsertionTraversal(RenderList& list, const Camera& cam, int insert_local);
	void InsertionTraversalRemoveBF(RenderList& list, const Camera& cam, int insert_local);
	void InsertionTraversalFrustrumCull(RenderList& list, const Camera& cam, int insert_local);

private:
	BSPNode* _root;

}; // BSP

}