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
	map<pair<int, int>, myHalfedge *> twin_map;
	map<pair<int, int>, myHalfedge *>::iterator it;
    if(isProfileFile(filename)){
				cout << "Reading mesh from file...\n";
				myPoint3D *profile;
				int num_profile_points, num_segments;
				while(getline(fin, s))
				{
					t.clear();
					stringstream myline(s);
					if (!(myline >> t)) continue;
					if (t == "num_profile_points") {
						myline >> num_profile_points;
						profile = new myPoint3D[num_profile_points];
					}
					else if (t == "num_segments") {
						myline >> num_segments;
					}
					else if (t == "v")
					{
						float x, y, z;
						myline >> x >> y >> z;
						profile[vertices.size()] = myPoint3D(x, y, z);
						vertices.push_back(new myVertex());
						vertices.back()->point = &profile[vertices.size() - 1];
						// cout << "v " << x << " " << y << " " << z << endl;
					}
				}
				drawSurfaceOfRevolution(profile, num_profile_points, num_segments);

	}else{

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
			unsigned int iplusone, iminusone;
			for (unsigned int i = 0; i < n; i++)
			{
				hedges[i]->source = vertices[faceids[i] - 1];
				vertices[faceids[i] - 1]->originof = hedges[i];
				halfedges.push_back(hedges[i]);
				iplusone = (i + 1) % n;
				iminusone = (i + n - 1) % n;
				hedges[i]->next = hedges[iplusone];
				hedges[i]->prev = hedges[iminusone];
				it = twin_map.find(make_pair(faceids[i], faceids[iplusone]));
				if (it != twin_map.end())
				{
					hedges[i]->twin = it->second;
					it->second->twin = hedges[i];
					twin_map.erase(it);
				}
				else
					twin_map[make_pair(faceids[iplusone], faceids[i])] = hedges[i];
			}
			myFace *f = new myFace();
			f->adjacent_halfedge = hedges[0];
			for (unsigned int i = 0; i < n; i++) hedges[i]->adjacent_face = f;
			faces.push_back(f);
		}
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
double lengthHalfEdge(myVertex *v1, myVertex *v2)
{
	double dx = v2->point->X - v1->point->X;
	double dy = v2->point->Y - v1->point->Y;
	double dz = v2->point->Z - v1->point->Z;
	return sqrt(dx*dx + dy*dy + dz*dz);
}
myVertex  barycenter(myVertex *v1, myVertex *v2)
{
	myVertex result;
	result.point = new myPoint3D((v1->point->X + v2->point->X) / 2,
		(v1->point->Y + v2->point->Y) / 2,
		(v1->point->Z + v2->point->Z) / 2);
	return result;
}
void myMesh::simplify()
{
	cout << "[DBG simplify] start, faces=" << faces.size() << " verts=" << vertices.size() << " he=" << halfedges.size() << endl; cout.flush();
	// Find the shortest edge in the mesh whose both adjacent faces are triangles
	myHalfedge *h = NULL;
	double min_length = 1e10;
	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge *he = halfedges[i];
		if (he->twin == NULL) continue;
		// Both faces must be triangles (3-cycle: prev->next == he)
		if (he->prev->next != he) continue;
		if (he->twin->prev->next != he->twin) continue;
		// All boundary twins must exist (no boundary edges on the two triangles)
		if (!he->next->twin || !he->prev->twin) continue;
		if (!he->twin->next->twin || !he->twin->prev->twin) continue;
		double length = lengthHalfEdge(he->source, he->twin->source);
		if (length < min_length) {
			min_length = length;
			h = he;
		}
	}
	if (h == NULL) { cout << "[DBG simplify] no valid edge found" << endl; return; }
	cout << "[DBG simplify] shortest edge found, length=" << min_length << endl; cout.flush();

	myHalfedge *ht = h->twin;
	myVertex   *v1 = h->source;
	myVertex   *v2 = ht->source;

	// Halfedges of the two faces adjacent to edge (v1->v2)
	myHalfedge *h_n  = h->next;   // v2 -> v3
	myHalfedge *h_p  = h->prev;   // v3 -> v1
	myHalfedge *ht_n = ht->next;  // v1 -> v4
	myHalfedge *ht_p = ht->prev;  // v4 -> v2

	myFace *f1 = h->adjacent_face;
	myFace *f2 = ht->adjacent_face;
	if (f1 == f2) { cout << "[DBG simplify] f1==f2 degenerate" << endl; return; }
	cout << "[DBG simplify] v1=" << v1 << " v2=" << v2 << " f1=" << f1 << " f2=" << f2 << endl; cout.flush();

	// Move v1 to the barycenter of v1 and v2
	myVertex bary = barycenter(v1, v2);
	v1->point->X = bary.point->X;
	v1->point->Y = bary.point->Y;
	v1->point->Z = bary.point->Z;
	delete bary.point;

	// Bypass f1: the surviving boundary edges become twins of each other
	cout << "[DBG simplify] bypass f1: h_n->twin=" << h_n->twin << " h_p->twin=" << h_p->twin << endl; cout.flush();
	h_n->twin->twin = h_p->twin;
	h_p->twin->twin = h_n->twin;

	// Bypass f2: the surviving boundary edges become twins of each other
	cout << "[DBG simplify] bypass f2: ht_n->twin=" << ht_n->twin << " ht_p->twin=" << ht_p->twin << endl; cout.flush();
	ht_n->twin->twin = ht_p->twin;
	ht_p->twin->twin = ht_n->twin;
	cout << "[DBG simplify] bypass done" << endl; cout.flush();

	// Fix originof pointers that point to halfedges about to be deleted
	myVertex *v3 = h_p->source;
	myVertex *v4 = ht_p->source;

	if (v1->originof == h     || v1->originof == ht_n) v1->originof = h_p->twin;
	if (v3->originof == h_p)                           v3->originof = h_n->twin;
	if (v4->originof == ht_p)                          v4->originof = ht_n->twin;

	// Redirect all halfedges sourced at v2 to v1
	for (unsigned int i = 0; i < halfedges.size(); i++) {
		if (halfedges[i]->source == v2)
			halfedges[i]->source = v1;
	}

	// Remove v2 from the mesh
	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i] == v2) { vertices.erase(vertices.begin() + i); break; }
	}
	delete v2->point;
	delete v2;
	cout << "[DBG simplify] v2 deleted" << endl; cout.flush();

	// Remove and delete the 6 halfedges belonging to f1 and f2
	myHalfedge *to_delete[6] = { h, h_n, h_p, ht, ht_n, ht_p };
	for (int k = 0; k < 6; k++) {
		for (unsigned int i = 0; i < halfedges.size(); i++) {
			if (halfedges[i] == to_delete[k]) { halfedges.erase(halfedges.begin() + i); break; }
		}
		delete to_delete[k];
	}

	// Remove and delete f1 and f2
	for (unsigned int i = 0; i < faces.size(); i++) {
		if (faces[i] == f1) { faces.erase(faces.begin() + i); break; }
	}
	delete f1;
	for (unsigned int i = 0; i < faces.size(); i++) {
		if (faces[i] == f2) { faces.erase(faces.begin() + i); break; }
	}
	delete f2;
	cout << "[DBG simplify] done, faces=" << faces.size() << " verts=" << vertices.size() << " he=" << halfedges.size() << endl; cout.flush();
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

void myMesh::testMeshIsCorrect(){
	checkMesh();
	testclosecircuit();
} 
void myMesh::testclosecircuit(){
	for (unsigned int i = 0; i < halfedges.size(); i++) {
		myHalfedge *h = halfedges[i];
		if (h->next == NULL || h->prev == NULL || h->source == NULL || h->adjacent_face == NULL) {
			cout << "Error: Halfedge " << i << " has null pointer(s)!" << endl;
			break;
		}
		if (h->next->prev != h) {
			cout << "Error: Halfedge " << i << "'s next's prev does not point back to it!" << endl;
			break;
		}
		if (h->prev->next != h) {
			cout << "Error: Halfedge " << i << "'s prev's next does not point back to it!" << endl;
			break;
		}
	}
}
void myMesh::drawSurfaceOfRevolution(myPoint3D *profile, int num_profile_points, int num_segments)
{
	// Clear the temporary vertices added by readFile while reading the profile
	clear();

	const double PI = acos(-1.0);
	const int n = num_profile_points; // profile points (latitude)
	const int s = num_segments;       // rotation steps  (longitude)

	// ----- Create vertices -----
	// Vertex at grid position (j, i): profile[i] rotated by angle j * 2*PI/s around the Y-axis.
	// Stored at index j*n + i.
	for (int j = 0; j < s; j++) {
		double theta = 2.0 * PI * j / s; //angle for current segment
		double cosT  = cos(theta);
		double sinT  = sin(theta);
		for (int i = 0; i < n; i++) {
			myVertex *v  = new myVertex();
			v->point     = new myPoint3D(profile[i].X * cosT, profile[i].Y, profile[i].X * sinT);
			vertices.push_back(v);
		}
	}

	// ----- Create quad faces and halfedges -----
	// Face (j, i) has corners: v[j][i], v[j][i+1], v[j+1][i+1], v[j+1][i]
	// (all indices wrap around modulo s and n → closed torus topology)
	//
	// Four halfedges per quad:
	//   h0: v[j][i]     → v[j][ip1]     (along profile)
	//   h1: v[j][ip1]   → v[jp1][ip1]   (around revolution)
	//   h2: v[jp1][ip1] → v[jp1][i]     (along profile, reversed)
	//   h3: v[jp1][i]   → v[j][i]       (around revolution, reversed)

	// Store halfedges indexed by face then local index (0-3)
	std::vector<std::array<myHalfedge*, 4>> fhe(s * n);

	for (int j = 0; j < s; j++) {
		for (int i = 0; i < n; i++) {
			const int fi  = j * n + i;
			const int jp1 = (j + 1) % s;
			const int ip1 = (i + 1) % n;

			// Allocate halfedges
			for (int k = 0; k < 4; k++) {
				fhe[fi][k] = new myHalfedge();
				halfedges.push_back(fhe[fi][k]);
			}

			myHalfedge *h0 = fhe[fi][0];
			myHalfedge *h1 = fhe[fi][1];
			myHalfedge *h2 = fhe[fi][2];
			myHalfedge *h3 = fhe[fi][3];

			// Sources
			h0->source = vertices[j   * n + i  ];
			h1->source = vertices[j   * n + ip1];
			h2->source = vertices[jp1 * n + ip1];
			h3->source = vertices[jp1 * n + i  ];

			// Cycle: next / prev
			h0->next = h1; h1->next = h2; h2->next = h3; h3->next = h0;
			h0->prev = h3; h1->prev = h0; h2->prev = h1; h3->prev = h2;

			// Face
			myFace *f = new myFace();
			f->adjacent_halfedge = h0;
			h0->adjacent_face = h1->adjacent_face = h2->adjacent_face = h3->adjacent_face = f;
			faces.push_back(f);

			// Each vertex keeps an arbitrary outgoing halfedge
			vertices[j   * n + i  ]->originof = h0;
			vertices[j   * n + ip1]->originof = h1;
			vertices[jp1 * n + ip1]->originof = h2;
			vertices[jp1 * n + i  ]->originof = h3;
		}
	}

	// ----- Set up twin halfedges -----
	// twin(h0(j,i)) = h2(j-1, i)
	// twin(h1(j,i)) = h3(j,  i+1)
	// twin(h2(j,i)) = h0(j+1, i)
	// twin(h3(j,i)) = h1(j,  i-1)
	for (int j = 0; j < s; j++) {
		for (int i = 0; i < n; i++) {
			const int fi  = j * n + i;
			const int jm1 = (j - 1 + s) % s;
			const int jp1 = (j + 1) % s;
			const int im1 = (i - 1 + n) % n;
			const int ip1 = (i + 1) % n;

			fhe[fi][0]->twin = fhe[jm1 * n + i  ][2];
			fhe[fi][1]->twin = fhe[j   * n + ip1][3];
			fhe[fi][2]->twin = fhe[jp1 * n + i  ][0];
			fhe[fi][3]->twin = fhe[j   * n + im1][1];
		}
	}
}

bool myMesh::isProfileFile(std::string filename) {
	std::ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	std::string line;
	while (std::getline(fin, line)) {
		std::istringstream iss(line);
		std::string token;
		if (!(iss >> token)) continue; // Skip empty lines
		return token == "num_profile_points"; // First non-empty line decides
	}
	return false;
}
