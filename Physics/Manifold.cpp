#include "Manifold.h"

namespace orb
{

using namespace PhysicsDefs;

Manifold::Manifold(IRigidBody* a, IRigidBody* b)
{
	if (a->GetId() < b->GetId())
	{
		m_bodyA = a;
		m_bodyB = b;
	}
	else
	{
		m_bodyA = b;
		m_bodyB = a;
	}

}

void Manifold::Solve()
{
	
}

void Manifold::Update(PhysicsDefs::ContactInfo* newContacts, int newContactCount)
{
	//Merge the old and new contacts
	//Based on Allen Chou's Game Physics series
	std::vector<ContactInfo> mergedContacts;
	mergedContacts.reserve(newContactCount);

	for (int i = 0; i < newContactCount; ++i)
	{
		ContactInfo contact = newContacts[i];

		for (size_t j = 0; j < m_contacts.size(); ++j)
		{
			const glm::vec3 rA = contact.localPointA - m_contacts[j].localPointA;
			const glm::vec3 rB = contact.localPointB - m_contacts[j].localPointB;

			if (glm::dot(rA, rA) < PERSISTENT_CONTACT_TOLERANCE_SQ &&
			    glm::dot(rB, rB) < PERSISTENT_CONTACT_TOLERANCE_SQ)
			{
				contact.prevNormalImp = m_contacts[j].prevNormalImp;
				contact.prevTangImp1  = m_contacts[j].prevTangImp1;
				contact.prevTangImp2  = m_contacts[j].prevTangImp2;
				break;
			}
		}

		mergedContacts.push_back(contact);
	}

	if ((int)mergedContacts.size() <= MIN_POINTS)
	{
		m_contacts = mergedContacts;
		return;
	}

	m_contacts = ReduceToFour(mergedContacts);
}

std::vector<PhysicsDefs::ContactInfo> Manifold::ReduceToFour(const std::vector<PhysicsDefs::ContactInfo>& candidates)
{
	std::vector<ContactInfo> result;
	result.reserve(MIN_POINTS);

	int taken[MIN_POINTS] = { -1, -1, -1, -1 };
	int takenCount = 0;

	auto alreadyTaken = [&](int idx)
	{
		for (int t = 0; t < takenCount; ++t)
			if (taken[t] == idx)
				return true;
		return false;
	};

	auto take = [&](int idx)
	{
		taken[takenCount++] = idx;
		result.push_back(candidates[idx]);
	};

	//Find deepest point
	int best = -1;
	float currMax = -FLT_MAX;
	for (size_t i = 0; i < candidates.size(); ++i)
	{
		if (candidates[i].depth > currMax)
		{
			currMax = candidates[i].depth;
			best = (int)i;
		}
	}
	if (best < 0)
		return result;
	take(best);

	//Find second point, furthest from the deepest point
	best = -1;
	currMax = -FLT_MAX;
	for (size_t i = 0; i < candidates.size(); ++i)
	{
		if (alreadyTaken((int)i))
			continue;
		const glm::vec3 diff = candidates[i].localPointA - result[0].localPointA;
		const float distance = glm::dot(diff, diff);
		if (distance > currMax)
		{
			currMax = distance;
			best = (int)i;
		}
	}
	if (best < 0)
		return result;
	take(best);

	//Find third point, furthest from the line between the first two
	best = -1;
	currMax = -FLT_MAX;
	for (size_t i = 0; i < candidates.size(); ++i)
	{
		if (alreadyTaken((int)i))
			continue;
		const float distance = DistanceToLineSq(result[0].localPointA, result[1].localPointA,
		                                        candidates[i].localPointA);
		if (distance > currMax)
		{
			currMax = distance;
			best = (int)i;
		}
	}
	if (best < 0)
		return result;
	take(best);

	//Find fourth point, furthest from the triangle formed by the first three
	best = -1;
	currMax = -FLT_MAX;
	for (size_t i = 0; i < candidates.size(); ++i)
	{
		if (alreadyTaken((int)i))
			continue;
		const float distance = DistanceToTriangleSq(result[0].localPointA, result[1].localPointA,
		                                            result[2].localPointA, candidates[i].localPointA);
		if (distance > currMax)
		{
			currMax = distance;
			best = (int)i;
		}
	}
	if (best >= 0)
		take(best);

	return result;
} 
}
