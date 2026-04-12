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
	if (originof == NULL) return;
	
	myHalfedge *e = originof;
	myHalfedge *step = originof;
	
	normal->dX = 0;
	normal->dY = 0;
	normal->dZ = 0;
	int counter = 0;
	
	do {
		if (step->adjacent_face != NULL && step->adjacent_face->normal != NULL) {
			myVector3D *h = step->adjacent_face->normal;
			normal->dX += h->dX;
			normal->dY += h->dY;
			normal->dZ += h->dZ;
			counter++;
		}
		
		if (step->twin == NULL || step->twin->next == NULL) break;
		step = step->twin->next;
	} while (e != step);
	
	if (counter > 0) {
		normal->normalize();
	}
}
