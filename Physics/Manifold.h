#ifndef MANIFOLD_H
#define MANIFOLD_H

#include "PhysicsDefs.h"
#include "IRigidBody.h"

namespace orb
{

class Manifold
{
public:
	Manifold(IRigidBody* a, IRigidBody* b);
	
	void Solve();
	void Update(PhysicsDefs::ContactInfo* newContacts, int newContactCount);

	static const int MIN_POINTS = 4;

	static constexpr float PERSISTENT_CONTACT_TOLERANCE_SQ = 0.001f;
	std::vector<PhysicsDefs::ContactInfo> m_contacts;
	bool m_isPersistent = false;
	IRigidBody* m_bodyA = 0;
	IRigidBody* m_bodyB = 0;

private:
	static std::vector<PhysicsDefs::ContactInfo> ReduceToFour(
		const std::vector<PhysicsDefs::ContactInfo>& candidates);
};

//based on Box2D Lite's ArbiterKey
struct ManifoldKey
{
	ManifoldKey(IRigidBody* a, IRigidBody* b)
	{
		if (a->GetId() < b->GetId())
		{
			bodyA = a;
			bodyB = b;
		}
		else
		{
			bodyA = b;
			bodyB = a;
		}

	}

	IRigidBody* bodyA = 0;
	IRigidBody* bodyB = 0;
};

inline bool operator <(const ManifoldKey& m1, const ManifoldKey& m2)
{
	if (m1.bodyA->GetId() < m2.bodyA->GetId())
		return true;

	if (m1.bodyA->GetId() == m2.bodyA->GetId() && m1.bodyB->GetId() < m2.bodyB->GetId())
		return true;

	return false;
}

}

#endif