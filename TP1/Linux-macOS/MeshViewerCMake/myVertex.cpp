#include "myVertex.h"
#include "myVector3D.h"
#include "myHalfedge.h"
#include "myFace.h"

myVertex::myVertex(void)
{
	point = NULL;
	originof = NULL;
	normal = new myVector3D(1.0,1.0,1.0);
}

myVertex::~myVertex(void)
{
	if (normal) delete normal;
}


void myVertex::computeNormal()
{
	normal->clear();
	myHalfedge *h = originof;
	if (!h) return;

	do {
		// Accumulate cross product contribution from this face
		myPoint3D *p0 = h->prev->source->point;
		myPoint3D *p1 = point;
		myPoint3D *p2 = h->next->source->point;
		myVector3D v1(p1->X - p0->X, p1->Y - p0->Y, p1->Z - p0->Z);
		myVector3D v2(p2->X - p1->X, p2->Y - p1->Y, p2->Z - p1->Z);
		*normal += v1.crossproduct(v2);

		// Advance to the next outgoing halfedge around this vertex
		if (!h->prev->twin) break;
		h = h->prev->twin;
	} while (h != originof);

	normal->normalize();
}
