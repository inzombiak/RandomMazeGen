#include "BroadphaseUniformGrid.h"

namespace orb
{

void BroadphaseUniformGrid::Insert(Grid& grid, AABBVector& oversized, const PhysicsDefs::AABB* aabb)
{
	glm::vec3 mn, mx;
	WorldBounds(aabb, mn, mx);

	int x0, y0, z0, x1, y1, z1;
	CellRange(mn, mx, x0, y0, z0, x1, y1, z1);

	const long long spanX = (long long)x1 - x0 + 1;
	const long long spanY = (long long)y1 - y0 + 1;
	const long long spanZ = (long long)z1 - z0 + 1;

	if (spanX * spanY * spanZ > MAX_CELLS_PER_BODY)
	{
		oversized.push_back(aabb);
		return;
	}

	for (int x = x0; x <= x1; ++x)
		for (int y = y0; y <= y1; ++y)
			for (int z = z0; z <= z1; ++z)
				grid[CellKey(x, y, z)].push_back(aabb);
}

void BroadphaseUniformGrid::RebuildStatics()
{
	m_staticGrid.clear();
	m_staticOversized.clear();

	for (size_t i = 0; i < m_aabbs.size(); ++i)
	{
		const PhysicsDefs::AABB* aabb = m_aabbs[i];
		if (aabb->body && aabb->body->IsStatic())
			Insert(m_staticGrid, m_staticOversized, aabb);
	}

	m_staticsDirty = false;
}

const std::vector<PhysicsDefs::CollisionPair>& BroadphaseUniformGrid::GetCollisionPairs()
{
	m_colliderPairs.clear();
	m_lastCandidateCount = 0;

	if (m_staticsDirty)
		RebuildStatics();

	// Statics hold their buckets between queries; only dynamics are re-bucketed.
	m_dynamicGrid.clear();
	m_dynamicOversized.clear();
	m_dynamicBodies.clear();

	for (size_t i = 0; i < m_aabbs.size(); ++i)
	{
		const PhysicsDefs::AABB* aabb = m_aabbs[i];
		if (aabb->body && aabb->body->IsStatic())
			continue;
		m_dynamicBodies.push_back(aabb);
		Insert(m_dynamicGrid, m_dynamicOversized, aabb);
	}

	glm::vec3 mn, mx, omn, omx;
	int x0, y0, z0, x1, y1, z1;

	// Walking dynamics only means a static pair is never even formed.
	for (size_t i = 0; i < m_dynamicBodies.size(); ++i)
	{
		const PhysicsDefs::AABB* a = m_dynamicBodies[i];
		if (!a->body)
			continue;

		WorldBounds(a, mn, mx);
		CellRange(mn, mx, x0, y0, z0, x1, y1, z1);

		m_candidates.clear();

		for (int x = x0; x <= x1; ++x)
			for (int y = y0; y <= y1; ++y)
				for (int z = z0; z <= z1; ++z)
				{
					const uint64_t key = CellKey(x, y, z);

					Grid::const_iterator dit = m_dynamicGrid.find(key);
					if (dit != m_dynamicGrid.end())
						m_candidates.insert(m_candidates.end(), dit->second.begin(), dit->second.end());

					Grid::const_iterator sit = m_staticGrid.find(key);
					if (sit != m_staticGrid.end())
						m_candidates.insert(m_candidates.end(), sit->second.begin(), sit->second.end());
				}

		m_candidates.insert(m_candidates.end(), m_dynamicOversized.begin(), m_dynamicOversized.end());
		m_candidates.insert(m_candidates.end(), m_staticOversized.begin(), m_staticOversized.end());

		// A pair sharing several cells is gathered several times.
		std::sort(m_candidates.begin(), m_candidates.end());
		m_candidates.erase(std::unique(m_candidates.begin(), m_candidates.end()), m_candidates.end());

		for (size_t c = 0; c < m_candidates.size(); ++c)
		{
			const PhysicsDefs::AABB* b = m_candidates[c];
			if (b == a || !b->body)
				continue;

			// Each dynamic-dynamic pair is emitted once, by the lower id.
			if (!b->body->IsStatic() && b->body->GetId() < a->body->GetId())
				continue;

			++m_lastCandidateCount;

			WorldBounds(b, omn, omx);
			if (Overlaps(mn, mx, omn, omx))
				m_colliderPairs.emplace_back(std::make_pair(a->body, b->body));
		}
	}

	return m_colliderPairs;
}

}
