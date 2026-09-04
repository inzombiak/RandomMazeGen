#ifndef BROADPHASE_AABB_H
#define BROADPHASE_AABB_H

#include "IBroadphase.h"

namespace orb
{

class BroadphaseAABB : public IBroadphase
{
public:

	virtual void AddAABB(const PhysicsDefs::AABB *aabb)
	{
		m_aabbs.push_back(aabb);
	}

	virtual void Update()
	{
		//TODO: Nothing
	}

	virtual const std::vector<PhysicsDefs::CollisionPair>& GetCollisionPairs();

protected:
	typedef std::vector<const PhysicsDefs::AABB*> AABBVector;
	AABBVector m_aabbs;

};

}

#endif // BROADPHASE_AABB_H
