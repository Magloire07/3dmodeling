#pragma once
#include "myFace.h"
#include "myHalfedge.h"
#include "myVertex.h"
#include <vector>
#include <string>

class myMesh
{
public:
	std::vector<myVertex *> vertices;
	std::vector<myHalfedge *> halfedges;
	std::vector<myFace *> faces;
	std::string name;

	void checkMesh();
	bool readFile(std::string filename);
	void computeNormals();
	void normalize();

	void subdivisionCatmullClark();

	void splitFaceTRIS(myFace *, myPoint3D *);

	void splitEdge(myHalfedge *, myPoint3D *);
	void splitFaceQUADS(myFace *, myPoint3D *);

	void triangulate();
	bool triangulate(myFace *);

	int countFaceVertices(myFace *f);
	void collectFaceData(myFace *f, std::vector<myVertex *> &face_vertices, std::vector<myHalfedge *> &original_halfedges);
	void createFanTriangles(std::vector<myVertex *> &face_vertices, int vertex_count, std::vector<std::vector<myHalfedge *>> &triangle_edges);
	void setupFanTwins(std::vector<std::vector<myHalfedge *>> &triangle_edges, std::vector<myHalfedge *> &original_halfedges, int vertex_count);
	void updateOriginOfPointers(std::vector<myVertex *> &face_vertices, std::vector<myHalfedge *> &original_halfedges, std::vector<std::vector<myHalfedge *>> &triangle_edges, int vertex_count);
	void removeOldFace(myFace *f, std::vector<myHalfedge *> &original_halfedges);

	void simplify();
	void simplify(myVertex *);

	void clear();

	myMesh(void);
	~myMesh(void);
};

