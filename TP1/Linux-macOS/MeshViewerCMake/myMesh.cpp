#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <GL/glew.h>
#include "myVector3D.h"

using namespace std;



myMesh::myMesh(void)
{
	/**** TODO ****/
}


myMesh::~myMesh(void)
{
	/**** TODO ****/
}

void myMesh::clear()
{
	for (unsigned int i = 0; i < vertices.size(); i++) if (vertices[i]) delete vertices[i];
	for (unsigned int i = 0; i < halfedges.size(); i++) if (halfedges[i]) delete halfedges[i];
	for (unsigned int i = 0; i < faces.size(); i++) if (faces[i]) delete faces[i];

	vector<myVertex *> empty_vertices;    vertices.swap(empty_vertices);
	vector<myHalfedge *> empty_halfedges; halfedges.swap(empty_halfedges);
	vector<myFace *> empty_faces;         faces.swap(empty_faces);
}

void myMesh::checkMesh()
{
	vector<myHalfedge *>::iterator it;
	for (it = halfedges.begin(); it != halfedges.end(); it++)
	{
		if ((*it)->twin == NULL)
			break;
	}
	if (it != halfedges.end())
		cout << "Error! Not all edges have their twins!\n";
	else cout << "Each edge has a twin!\n";
}


bool myMesh::readFile(std::string filename)
{
	string s, t, u;
	vector<int> faceids;
	myHalfedge **hedges;

	ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	name = filename;

	map<pair<int, int>, myHalfedge *> twin_map;
	map<pair<int, int>, myHalfedge *>::iterator it;

	while (getline(fin, s))
	{
		stringstream myline(s);
		myline >> t;
		if (t == "g") {}
		else if (t == "v")
		{

			float x, y, z;
			myline >> x >> y >> z;
			//cout << "v " << x << " " << y << " " << z << endl;
			myVertex *v = new myVertex();
			myPoint3D *p = new myPoint3D(x, y, z);
			v->point = p;
			vertices.push_back(v);
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			faceids.clear();
			while (myline >> u) // read indices of vertices from a face into a container - it helps to access them later 
				faceids.push_back(atoi((u.substr(0, u.find("/"))).c_str()) - 1);
			if (faceids.size() < 3) // ignore degenerate faces
				continue;

			hedges = new myHalfedge *[faceids.size()]; // allocate the array for storing pointers to half-edges
			for (unsigned int i = 0; i < faceids.size(); i++) 
				hedges[i] = new myHalfedge(); // pre-allocate new half-edges

			myFace *f = new myFace(); // allocate the new face
			f->adjacent_halfedge = hedges[0]; // connect the face with incident edge
			faces.push_back(f); // push the face to faces in myMesh
			for (unsigned int i = 0; i < faceids.size(); i++)
			{
				int iplusone = (i + 1) % faceids.size();
				int iminusone = (i - 1 + faceids.size()) % faceids.size();

				// YOUR CODE COMES HERE!

				// connect prevs, and next
				hedges[i]->next = hedges[iplusone];
				hedges[i]->prev = hedges[iminusone];

				// search for the twins using twin_map
				it = twin_map.find(make_pair(faceids[i], faceids[iplusone]));
				if (it != twin_map.end()) // if the twin is found, connect the twins
				{
					hedges[i]->twin = it->second;
					it->second->twin = hedges[i];
					twin_map.erase(it); // remove the twin from the map
				}
				else // if the twin is not found, add the half-edge to the map
				{
					twin_map[make_pair(faceids[iplusone], faceids[i])] = hedges[i];
				}
				// set originof
				hedges[i]->source = vertices[faceids[i]];
				vertices[faceids[i]]->originof = hedges[i];
				hedges[i]->adjacent_face = f;
				// push edges to halfedges in myMesh
				halfedges.push_back(hedges[i]);
			}
			delete[] hedges;

			// push faces to faces in myMesh
			// cout << "f"; 

			// while (myline >> u) 
			// {
			// 	cout << " " << atoi((u.substr(0, u.find("/"))).c_str());
			// 	faceids.push_back(atoi((u.substr(0, u.find("/"))).c_str())-1);
			// 	cout << "size" << faceids.size();
			// }
			// cout << endl;
		}
	}
	checkMesh();
	normalize();

	return true;
}


void myMesh::computeNormals()
{
	// First, compute normals for all faces
	for (unsigned int i = 0; i < faces.size(); i++) {
		faces[i]->computeNormal();
	}
	
	// Then, compute normals for all vertices (needs face normals to be computed first)
	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->computeNormal();
	}
}

void myMesh::normalize()
{
	if (vertices.size() < 1) return;

	int tmpxmin = 0, tmpymin = 0, tmpzmin = 0, tmpxmax = 0, tmpymax = 0, tmpzmax = 0;

	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i]->point->X < vertices[tmpxmin]->point->X) tmpxmin = i;
		if (vertices[i]->point->X > vertices[tmpxmax]->point->X) tmpxmax = i;

		if (vertices[i]->point->Y < vertices[tmpymin]->point->Y) tmpymin = i;
		if (vertices[i]->point->Y > vertices[tmpymax]->point->Y) tmpymax = i;

		if (vertices[i]->point->Z < vertices[tmpzmin]->point->Z) tmpzmin = i;
		if (vertices[i]->point->Z > vertices[tmpzmax]->point->Z) tmpzmax = i;
	}

	double xmin = vertices[tmpxmin]->point->X, xmax = vertices[tmpxmax]->point->X,
		ymin = vertices[tmpymin]->point->Y, ymax = vertices[tmpymax]->point->Y,
		zmin = vertices[tmpzmin]->point->Z, zmax = vertices[tmpzmax]->point->Z;

	double scale = (xmax - xmin) > (ymax - ymin) ? (xmax - xmin) : (ymax - ymin);
	scale = scale > (zmax - zmin) ? scale : (zmax - zmin);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->point->X -= (xmax + xmin) / 2;
		vertices[i]->point->Y -= (ymax + ymin) / 2;
		vertices[i]->point->Z -= (zmax + zmin) / 2;

		vertices[i]->point->X /= scale;
		vertices[i]->point->Y /= scale;
		vertices[i]->point->Z /= scale;
	}
}


void myMesh::splitFaceTRIS(myFace *f, myPoint3D *p)
{
	if (f == NULL || f->adjacent_halfedge == NULL || p == NULL) return;
	
	// Create a new vertex at point p
	myVertex *center_vertex = new myVertex();
	center_vertex->point = p;
	vertices.push_back(center_vertex);
	
	// Collect all vertices and original halfedges of the face
	std::vector<myVertex *> face_vertices;
	std::vector<myHalfedge *> original_halfedges;
	myHalfedge *h = f->adjacent_halfedge;
	myHalfedge *start = h;
	
	do {
		face_vertices.push_back(h->source);
		original_halfedges.push_back(h);
		h = h->next;
	} while (h != start);
	
	int n = face_vertices.size();
	
	// Create new triangular faces
	for (int i = 0; i < n; i++) {
		int next_i = (i + 1) % n;
		
		// Create three new halfedges for the triangle
		myHalfedge *e1 = new myHalfedge(); // from vertex[i] to vertex[i+1]
		myHalfedge *e2 = new myHalfedge(); // from vertex[i+1] to center
		myHalfedge *e3 = new myHalfedge(); // from center to vertex[i]
		
		// Set sources
		e1->source = face_vertices[i];
		e2->source = face_vertices[next_i];
		e3->source = center_vertex;
		
		// Set next and prev
		e1->next = e2;
		e2->next = e3;
		e3->next = e1;
		
		e1->prev = e3;
		e2->prev = e1;
		e3->prev = e2;
		
		// Keep the twin relationship from the original edge
		e1->twin = original_halfedges[i]->twin;
		if (e1->twin != NULL) {
			e1->twin->twin = e1;
		}
		
		// Create new face
		myFace *new_face = new myFace();
		e1->adjacent_face = new_face;
		e2->adjacent_face = new_face;
		e3->adjacent_face = new_face;
		new_face->adjacent_halfedge = e1;
		
		faces.push_back(new_face);
		halfedges.push_back(e1);
		halfedges.push_back(e2);
		halfedges.push_back(e3);
		
		// Update originof for vertices
		if (e1->source->originof == original_halfedges[i]) {
			e1->source->originof = e1;
		}
		if (i == 0) {
			center_vertex->originof = e3;
		}
	}
	
	// Remove original halfedges from the halfedges vector
	for (unsigned int i = 0; i < original_halfedges.size(); i++) {
		for (unsigned int j = 0; j < halfedges.size(); j++) {
			if (halfedges[j] == original_halfedges[i]) {
				halfedges.erase(halfedges.begin() + j);
				break;
			}
		}
		delete original_halfedges[i];
	}
	
	// Remove the original face
	for (unsigned int i = 0; i < faces.size(); i++) {
		if (faces[i] == f) {
			faces.erase(faces.begin() + i);
			break;
		}
	}
	delete f;
}

void myMesh::splitEdge(myHalfedge *e1, myPoint3D *p)
{

	/**** TODO ****/
}

void myMesh::splitFaceQUADS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}


void myMesh::subdivisionCatmullClark()
{
	/**** TODO ****/
}

void myMesh::simplify()
{
	/**** TODO ****/
}

void myMesh::simplify(myVertex *)
{
	/**** TODO ****/
}

void myMesh::triangulate()
{
	// Make a copy of the faces vector to avoid issues with iterator invalidation
	std::vector<myFace *> faces_to_triangulate;
	for (unsigned int i = 0; i < faces.size(); i++) {
		faces_to_triangulate.push_back(faces[i]);
	}
	
	// Triangulate each face
	for (unsigned int i = 0; i < faces_to_triangulate.size(); i++) {
		triangulate(faces_to_triangulate[i]);
	}
}

//return false if already triangle, true othewise.
bool myMesh::triangulate(myFace *f)
{
	if (f == NULL || f->adjacent_halfedge == NULL) return false;
	
	// Count the number of vertices in the face
	int vertex_count = 0;
	myHalfedge *h = f->adjacent_halfedge;
	myHalfedge *start = h;
	
	do {
		vertex_count++;
		h = h->next;
		if (h == NULL) return false; // Broken face structure
	} while (h != start);
	
	// If already a triangle, nothing to do
	if (vertex_count == 3) {
		return false;
	}
	
	// Compute the center point of the face
	myPoint3D *center = new myPoint3D(0.0, 0.0, 0.0);
	h = f->adjacent_halfedge;
	do {
		center->X += h->source->point->X;
		center->Y += h->source->point->Y;
		center->Z += h->source->point->Z;
		h = h->next;
	} while (h != start);
	
	center->X /= vertex_count;
	center->Y /= vertex_count;
	center->Z /= vertex_count;
	
	// Split the face into triangles using the center point
	splitFaceTRIS(f, center);
	
	return true;
}

