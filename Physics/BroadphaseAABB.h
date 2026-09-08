#ifndef BROADPHASE_AABB_H
#define BROADPHASE_AABB_H

#include "IBroadphase.h"

#include <algorithm>

namespace orb
{

class BroadphaseAABB : public IBroadphase
{
public:

	virtual void AddAABB(const PhysicsDefs::AABB *aabb)
	{
		m_aabbs.push_back(aabb);
	}

	virtual void RemoveAABB(const PhysicsDefs::AABB *aabb)
	{
		auto it = std::find(m_aabbs.begin(), m_aabbs.end(), aabb);
		if (it != m_aabbs.end())
		{
			std::swap(*it, m_aabbs.back());
			m_aabbs.pop_back();
		}
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
