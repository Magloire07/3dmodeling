#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <cmath>
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
		t.clear();
		stringstream myline(s);
		if (!(myline >> t)) continue;
		if (t == "g") {}
		else if (t == "v")
		{
			float x, y, z;
			myline >> x >> y >> z;
			myVertex *v = new myVertex();
			v->point = new myPoint3D(x, y, z);
			vertices.push_back(v);
			// cout << "v " << x << " " << y << " " << z << endl;
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			faceids.clear();
			while (myline >> u)
				faceids.push_back(atoi((u.substr(0, u.find("/"))).c_str()));

			unsigned int n = faceids.size();
			hedges = new myHalfedge *[n];
			for (unsigned int i = 0; i < n; i++) hedges[i] = new myHalfedge();

			for (unsigned int i = 0; i < n; i++)
			{
				hedges[i]->source = vertices[faceids[i] - 1];
				vertices[faceids[i] - 1]->originof = hedges[i];
				halfedges.push_back(hedges[i]);
				hedges[i]->next = hedges[(i + 1) % n];
				hedges[i]->prev = hedges[(i + n - 1) % n];
				it = twin_map.find(make_pair(faceids[i], faceids[(i + 1) % n]));
				if (it != twin_map.end())
				{
					hedges[i]->twin = it->second;
					it->second->twin = hedges[i];
					twin_map.erase(it);
				}
				else
					twin_map[make_pair(faceids[(i + 1) % n], faceids[i])] = hedges[i];
			}
			myFace *f = new myFace();
			f->adjacent_halfedge = hedges[0];
			for (unsigned int i = 0; i < n; i++) hedges[i]->adjacent_face = f;
			faces.push_back(f);
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

	// Create a new center vertex.
	myVertex *center_vertex = new myVertex();
	center_vertex->point = p;
	vertices.push_back(center_vertex);

	// Collect boundary cycle data from the original face.
	std::vector<myVertex *> face_vertices;
	std::vector<myHalfedge *> original_halfedges;
	myHalfedge *start = f->adjacent_halfedge;
	myHalfedge *h = start;
	do {
		face_vertices.push_back(h->source);
		original_halfedges.push_back(h);
		h = h->next;
	} while (h != start);

	const int n = (int)face_vertices.size();
	if (n < 3) return;

	std::vector<myHalfedge *> edge_boundary(n, NULL);   // vi -> v(i+1)
	std::vector<myHalfedge *> edge_to_center(n, NULL);  // v(i+1) -> c
	std::vector<myHalfedge *> edge_from_center(n, NULL);// c -> vi

	for (int i = 0; i < n; i++) {
		const int next_i = (i + 1) % n;

		myHalfedge *e1 = new myHalfedge();
		myHalfedge *e2 = new myHalfedge();
		myHalfedge *e3 = new myHalfedge();

		e1->source = face_vertices[i];
		e2->source = face_vertices[next_i];
		e3->source = center_vertex;

		e1->next = e2; e2->next = e3; e3->next = e1;
		e1->prev = e3; e2->prev = e1; e3->prev = e2;

		myFace *new_face = new myFace();
		new_face->adjacent_halfedge = e1;
		e1->adjacent_face = new_face;
		e2->adjacent_face = new_face;
		e3->adjacent_face = new_face;

		// Keep twin relationship on boundary edges with neighboring faces.
		e1->twin = original_halfedges[i]->twin;
		if (e1->twin != NULL) e1->twin->twin = e1;

		if (e1->source->originof == original_halfedges[i]) {
			e1->source->originof = e1;
		}

		faces.push_back(new_face);
		halfedges.push_back(e1);
		halfedges.push_back(e2);
		halfedges.push_back(e3);

		edge_boundary[i] = e1;
		edge_to_center[i] = e2;
		edge_from_center[i] = e3;
	}

	// Connect radial twins between neighboring new triangles.
	for (int i = 0; i < n; i++) {
		const int next_i = (i + 1) % n;
		edge_to_center[i]->twin = edge_from_center[next_i];
		edge_from_center[next_i]->twin = edge_to_center[i];
	}

	center_vertex->originof = edge_from_center[0];

	// Remove old halfedges from mesh storage, then delete them.
	for (unsigned int i = 0; i < original_halfedges.size(); i++) {
		for (unsigned int j = 0; j < halfedges.size(); j++) {
			if (halfedges[j] == original_halfedges[i]) {
				halfedges.erase(halfedges.begin() + j);
				break;
			}
		}
		delete original_halfedges[i];
	}

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

bool pointInTriangle(myVertex *A, myVertex *B, myVertex *C, myVertex *P)
{
	double denom = (B->point->Y - C->point->Y) * (A->point->X - C->point->X) +
		(C->point->X - B->point->X) * (A->point->Y - C->point->Y);
	if (std::fabs(denom) < 1e-12) return false;

	double alpha = ((B->point->Y - C->point->Y) * (P->point->X - C->point->X) +
		(C->point->X - B->point->X) * (P->point->Y - C->point->Y)) / denom;
	double beta = ((C->point->Y - A->point->Y) * (P->point->X - C->point->X) +
		(A->point->X - C->point->X) * (P->point->Y - C->point->Y)) / denom;
	double gamma = 1.0 - alpha - beta;

	const double eps = 1e-10;
	return alpha >= -eps && beta >= -eps && gamma >= -eps;
}

double turn2D(myVertex *prev, myVertex *curr, myVertex *next)
{
	return (curr->point->X - prev->point->X) * (next->point->Y - curr->point->Y) -
		(curr->point->Y - prev->point->Y) * (next->point->X - curr->point->X);
}

double polygonArea2D(const std::vector<myVertex *> &poly)
{
	double area2 = 0.0;
	for (unsigned int i = 0; i < poly.size(); i++) {
		unsigned int j = (i + 1) % poly.size();
		area2 += poly[i]->point->X * poly[j]->point->Y - poly[j]->point->X * poly[i]->point->Y;
	}
	return area2;
}

void myMesh::triangulate()
{
	bool has_non_triangles = true;
	while (has_non_triangles) {
		has_non_triangles = false;

		std::vector<myFace *> faces_to_triangulate;
		for (unsigned int i = 0; i < faces.size(); i++) {
			faces_to_triangulate.push_back(faces[i]);
		}

		for (unsigned int i = 0; i < faces_to_triangulate.size(); i++) {
			bool face_exists = false;
			for (unsigned int j = 0; j < faces.size(); j++) {
				if (faces[j] == faces_to_triangulate[i]) {
					face_exists = true;
					break;
				}
			}

			if (face_exists && triangulate(faces_to_triangulate[i])) {
				has_non_triangles = true;
			}
		}
	}
}

// void myMesh::triangulate()
// {
// 	// Keep triangulating until all faces are triangles
// 	// We use a while loop because splitFaceTRIS modifies the faces vector
// 	bool has_non_triangles = true;
// 	while (has_non_triangles) {
// 		has_non_triangles = false;
		
// 		// Create a fresh copy of the current faces vector
// 		std::vector<myFace *> faces_to_triangulate;
// 		for (unsigned int i = 0; i < faces.size(); i++) {
// 			faces_to_triangulate.push_back(faces[i]);
// 		}
		
// 		// Triangulate each non-triangle face
// 		for (unsigned int i = 0; i < faces_to_triangulate.size(); i++) {
// 			// Check if face pointer is still valid (wasn't deleted in a previous iteration)
// 			bool face_exists = false;
// 			for (unsigned int j = 0; j < faces.size(); j++) {
// 				if (faces[j] == faces_to_triangulate[i]) {
// 					face_exists = true;
// 					break;
// 				}
// 			}
			
// 			if (face_exists) {
// 				if (triangulate(faces_to_triangulate[i])) {
// 					has_non_triangles = true;
// 				}
// 			}
// 		}
// 	}
// }

//return false if already triangle, true othewise.
int myMesh::countFaceVertices(myFace *f)
{
	int count = 0;
	myHalfedge *h = f->adjacent_halfedge;
	myHalfedge *start = h;
	do {
		count++;
		h = h->next;
		if (h == NULL) return -1;
	} while (h != start);
	return count;
}

void myMesh::collectFaceData(myFace *f, std::vector<myVertex *> &face_vertices, std::vector<myHalfedge *> &original_halfedges)
{
	myHalfedge *h = f->adjacent_halfedge;
	myHalfedge *start = h;
	do {
		face_vertices.push_back(h->source);
		original_halfedges.push_back(h);
		h = h->next;
	} while (h != start);
}

void myMesh::createFanTriangles(std::vector<myVertex *> &face_vertices, int vertex_count, std::vector<std::vector<myHalfedge *>> &triangle_edges)
{
	myVertex *base_vertex = face_vertices[0];
	for (int i = 1; i < vertex_count - 1; i++) {
		myHalfedge *e1 = new myHalfedge();
		myHalfedge *e2 = new myHalfedge();
		myHalfedge *e3 = new myHalfedge();

		e1->source = base_vertex;
		e2->source = face_vertices[i];
		e3->source = face_vertices[i + 1];

		e1->next = e2; e2->next = e3; e3->next = e1;
		e1->prev = e3; e2->prev = e1; e3->prev = e2;

		myFace *new_face = new myFace();
		new_face->adjacent_halfedge = e1;
		e1->adjacent_face = new_face;
		e2->adjacent_face = new_face;
		e3->adjacent_face = new_face;

		e1->twin = NULL;
		e2->twin = NULL;
		e3->twin = NULL;

		faces.push_back(new_face);
		halfedges.push_back(e1);
		halfedges.push_back(e2);
		halfedges.push_back(e3);

		std::vector<myHalfedge *> tri_edges;
		tri_edges.push_back(e1);
		tri_edges.push_back(e2);
		tri_edges.push_back(e3);
		triangle_edges.push_back(tri_edges);
	}
}

void myMesh::setupFanTwins(std::vector<std::vector<myHalfedge *>> &triangle_edges, std::vector<myHalfedge *> &original_halfedges, int vertex_count)
{
	if (triangle_edges.empty()) return;

	// First triangle: e1 twins with original_halfedges[0]
	triangle_edges[0][0]->twin = original_halfedges[0]->twin;
	if (triangle_edges[0][0]->twin != NULL)
		triangle_edges[0][0]->twin->twin = triangle_edges[0][0];

	// Each triangle's e2 twins with original_halfedges[i+1]
	for (unsigned int i = 0; i < triangle_edges.size(); i++) {
		myHalfedge *e2 = triangle_edges[i][1];
		e2->twin = original_halfedges[i + 1]->twin;
		if (e2->twin != NULL)
			e2->twin->twin = e2;
	}

	// Last triangle: e3 twins with original_halfedges[n-1]
	int last = triangle_edges.size() - 1;
	triangle_edges[last][2]->twin = original_halfedges[vertex_count - 1]->twin;
	if (triangle_edges[last][2]->twin != NULL)
		triangle_edges[last][2]->twin->twin = triangle_edges[last][2];

	// Interior fan edges: consecutive triangles
	for (int i = 0; i < (int)triangle_edges.size() - 1; i++) {
		triangle_edges[i][2]->twin = triangle_edges[i + 1][0];
		triangle_edges[i + 1][0]->twin = triangle_edges[i][2];
	}
}

void myMesh::updateOriginOfPointers(std::vector<myVertex *> &face_vertices, std::vector<myHalfedge *> &original_halfedges, std::vector<std::vector<myHalfedge *>> &triangle_edges, int vertex_count)
{
	for (int i = 0; i < vertex_count; i++) {
		bool needs_update = false;
		for (unsigned int j = 0; j < original_halfedges.size(); j++) {
			if (face_vertices[i]->originof == original_halfedges[j]) {
				needs_update = true;
				break;
			}
		}
		if (needs_update) {
			for (unsigned int j = 0; j < triangle_edges.size(); j++) {
				for (int k = 0; k < 3; k++) {
					if (triangle_edges[j][k]->source == face_vertices[i]) {
						face_vertices[i]->originof = triangle_edges[j][k];
						goto done_update;
					}
				}
			}
			done_update:;
		}
	}
}

void myMesh::removeOldFace(myFace *f, std::vector<myHalfedge *> &original_halfedges)
{
	for (unsigned int i = 0; i < original_halfedges.size(); i++) {
		for (unsigned int j = 0; j < halfedges.size(); j++) {
			if (halfedges[j] == original_halfedges[i]) {
				halfedges.erase(halfedges.begin() + j);
				break;
			}
		}
		delete original_halfedges[i];
	}
	for (unsigned int i = 0; i < faces.size(); i++) {
		if (faces[i] == f) {
			faces.erase(faces.begin() + i);
			break;
		}
	}
	delete f;
}

namespace {
	typedef std::pair<int, int> EdgeKey;

	bool buildEarTriangles(const std::vector<myVertex *> &face_vertices, std::vector<std::vector<int>> &triangle_indices)
	{
		const int vertex_count = (int)face_vertices.size();
		if (vertex_count < 3) return false;

		double area2 = polygonArea2D(face_vertices);
		if (std::fabs(area2) < 1e-12) return false;
		bool ccw = area2 > 0.0;

		std::vector<int> polygon_indices;
		for (int i = 0; i < vertex_count; i++) polygon_indices.push_back(i);

		while (polygon_indices.size() > 3) {
			bool ear_found = false;
			int m = (int)polygon_indices.size();

			for (int i = 0; i < m; i++) {
				int i_prev = polygon_indices[(i - 1 + m) % m];
				int i_curr = polygon_indices[i];
				int i_next = polygon_indices[(i + 1) % m];

				myVertex *prev = face_vertices[i_prev];
				myVertex *curr = face_vertices[i_curr];
				myVertex *next = face_vertices[i_next];

				double turn = turn2D(prev, curr, next);
				if ((ccw && turn <= 1e-12) || (!ccw && turn >= -1e-12)) continue;

				bool contains_vertex = false;
				for (int j = 0; j < m; j++) {
					int candidate = polygon_indices[j];
					if (candidate == i_prev || candidate == i_curr || candidate == i_next) continue;
					if (pointInTriangle(prev, curr, next, face_vertices[candidate])) {
						contains_vertex = true;
						break;
					}
				}

				if (!contains_vertex) {
					std::vector<int> tri;
					tri.push_back(i_prev);
					tri.push_back(i_curr);
					tri.push_back(i_next);
					triangle_indices.push_back(tri);
					polygon_indices.erase(polygon_indices.begin() + i);
					ear_found = true;
					break;
				}
			}

			if (!ear_found) return false;
		}

		std::vector<int> final_tri;
		final_tri.push_back(polygon_indices[0]);
		final_tri.push_back(polygon_indices[1]);
		final_tri.push_back(polygon_indices[2]);
		triangle_indices.push_back(final_tri);

		return true;
	}

	void createTrianglesAndEdges(
		myMesh *mesh,
		const std::vector<myVertex *> &face_vertices,
		const std::vector<std::vector<int>> &triangle_indices,
		std::map<EdgeKey, myHalfedge *> &created_edges)
	{
		for (unsigned int t = 0; t < triangle_indices.size(); t++) {
			int a = triangle_indices[t][0];
			int b = triangle_indices[t][1];
			int c = triangle_indices[t][2];

			myHalfedge *e1 = new myHalfedge();
			myHalfedge *e2 = new myHalfedge();
			myHalfedge *e3 = new myHalfedge();

			e1->source = face_vertices[a];
			e2->source = face_vertices[b];
			e3->source = face_vertices[c];

			e1->next = e2; e2->next = e3; e3->next = e1;
			e1->prev = e3; e2->prev = e1; e3->prev = e2;

			myFace *new_face = new myFace();
			new_face->adjacent_halfedge = e1;

			e1->adjacent_face = new_face;
			e2->adjacent_face = new_face;
			e3->adjacent_face = new_face;

			e1->twin = NULL;
			e2->twin = NULL;
			e3->twin = NULL;

			mesh->faces.push_back(new_face);
			mesh->halfedges.push_back(e1);
			mesh->halfedges.push_back(e2);
			mesh->halfedges.push_back(e3);

			created_edges[EdgeKey(a, b)] = e1;
			created_edges[EdgeKey(b, c)] = e2;
			created_edges[EdgeKey(c, a)] = e3;
		}
	}

	void connectCreatedEdgeTwins(
		const std::map<EdgeKey, myHalfedge *> &boundary_map,
		std::map<EdgeKey, myHalfedge *> &created_edges)
	{
		for (std::map<EdgeKey, myHalfedge *>::iterator it = created_edges.begin(); it != created_edges.end(); ++it) {
			int src = it->first.first;
			int dst = it->first.second;
			myHalfedge *edge = it->second;

			std::map<EdgeKey, myHalfedge *>::const_iterator boundary_it = boundary_map.find(EdgeKey(src, dst));
			if (boundary_it != boundary_map.end()) {
				edge->twin = boundary_it->second->twin;
				if (edge->twin != NULL) edge->twin->twin = edge;
				continue;
			}

			std::map<EdgeKey, myHalfedge *>::iterator reverse_it = created_edges.find(EdgeKey(dst, src));
			if (reverse_it != created_edges.end()) {
				edge->twin = reverse_it->second;
			}
		}
	}

	void updateOriginOfAfterTriangulation(
		const std::vector<myVertex *> &face_vertices,
		const std::vector<myHalfedge *> &original_halfedges,
		const std::map<EdgeKey, myHalfedge *> &created_edges)
	{
		for (unsigned int i = 0; i < face_vertices.size(); i++) {
			bool needs_update = false;
			for (unsigned int j = 0; j < original_halfedges.size(); j++) {
				if (face_vertices[i]->originof == original_halfedges[j]) {
					needs_update = true;
					break;
				}
			}
			if (!needs_update) continue;

			for (std::map<EdgeKey, myHalfedge *>::const_iterator it = created_edges.begin(); it != created_edges.end(); ++it) {
				if (it->first.first == (int)i) {
					face_vertices[i]->originof = it->second;
					break;
				}
			}
		}
	}

	void doEarClipTriangulate(
		myMesh *mesh,
		const std::vector<myVertex *> &face_vertices,
		const std::vector<myHalfedge *> &original_halfedges)
	{
		int vertex_count = (int)face_vertices.size();

		// Build boundary map: local edge (i -> (i+1)%n) -> original halfedge
		std::map<EdgeKey, myHalfedge *> boundary_map;
		for (int i = 0; i < vertex_count; i++)
			boundary_map[EdgeKey(i, (i + 1) % vertex_count)] = original_halfedges[i];

		// Try ear-clipping triangulation
		std::vector<std::vector<int>> triangle_indices;
		if (!buildEarTriangles(face_vertices, triangle_indices)) {
			// Fallback to fan triangulation for degenerate/flat faces
			for (int i = 1; i < vertex_count - 1; i++) {
				std::vector<int> tri;
				tri.push_back(0);
				tri.push_back(i);
				tri.push_back(i + 1);
				triangle_indices.push_back(tri);
			}
		}

		std::map<EdgeKey, myHalfedge *> created_edges;
		createTrianglesAndEdges(mesh, face_vertices, triangle_indices, created_edges);
		connectCreatedEdgeTwins(boundary_map, created_edges);
		updateOriginOfAfterTriangulation(face_vertices, original_halfedges, created_edges);
	}
} // end anonymous namespace

bool myMesh::triangulate(myFace *f)
{
	if (f == NULL || f->adjacent_halfedge == NULL) return false;

	int vertex_count = countFaceVertices(f);
	if (vertex_count < 0 || vertex_count == 3) return false;

	std::vector<myVertex *> face_vertices;
	std::vector<myHalfedge *> original_halfedges;
	collectFaceData(f, face_vertices, original_halfedges);

	doEarClipTriangulate(this, face_vertices, original_halfedges);

	removeOldFace(f, original_halfedges);

	return true;
}

