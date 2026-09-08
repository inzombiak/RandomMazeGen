#ifndef BROADPHASE_UNIFORM_GRID_H
#define BROADPHASE_UNIFORM_GRID_H

#include "IBroadphase.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace orb
{

// Uniform grid broadphase.
//
// The maze is a grid, so a uniform grid is the natural fit; the default cell
// size matches its 2-unit tile pitch.
//
// Statics are bucketed once and reused, because maze walls never move. Only
// dynamic bodies are re-bucketed per query, and pairs are gathered by walking
// the dynamic bodies alone, so static-vs-static is not merely rejected but
// never considered.
//
// A body spanning more than MAX_CELLS_PER_BODY cells goes into an "oversized"
// list tested against every dynamic body instead of being smeared across
// hundreds of buckets. That is what keeps a single large ground slab from
// dominating the grid.
class BroadphaseUniformGrid : public IBroadphase
{
public:
	explicit BroadphaseUniformGrid(float cellSize = 2.0f)
		: m_cellSize(cellSize > 0.f ? cellSize : 2.0f)
	{}

	void AddAABB(const PhysicsDefs::AABB* aabb) override
	{
		if (!aabb)
			return;
		m_aabbs.push_back(aabb);
		m_staticsDirty = true;
	}

	void RemoveAABB(const PhysicsDefs::AABB* aabb) override
	{
		auto it = std::find(m_aabbs.begin(), m_aabbs.end(), aabb);
		if (it != m_aabbs.end())
		{
			std::swap(*it, m_aabbs.back());
			m_aabbs.pop_back();
			m_staticsDirty = true;
		}
	}

	void Update() override { m_staticsDirty = true; }

	const std::vector<PhysicsDefs::CollisionPair>& GetCollisionPairs() override;

	// Candidate pairs considered in the last query. The pair count, not the
	// frame time, is what says whether the grid is doing its job.
	size_t GetLastCandidateCount() const { return m_lastCandidateCount; }

private:
	typedef std::vector<const PhysicsDefs::AABB*> AABBVector;
	typedef std::unordered_map<uint64_t, AABBVector> Grid;

	static const int MAX_CELLS_PER_BODY = 64;

	static uint64_t CellKey(int x, int y, int z)
	{
		// 21 bits per axis, biased to keep negative coordinates positive.
		const uint64_t ux = (uint64_t)(x + 0x100000) & 0x1FFFFF;
		const uint64_t uy = (uint64_t)(y + 0x100000) & 0x1FFFFF;
		const uint64_t uz = (uint64_t)(z + 0x100000) & 0x1FFFFF;
		return ux | (uy << 21) | (uz << 42);
	}

	static void WorldBounds(const PhysicsDefs::AABB* aabb, glm::vec3& outMin, glm::vec3& outMax)
	{
		// min/max are stored relative to the body centre; worldTransform carries
		// the translation.
		const glm::vec3 pos(aabb->worldTransform[3]);
		outMin = pos + aabb->min;
		outMax = pos + aabb->max;
	}

	static bool Overlaps(const glm::vec3& minA, const glm::vec3& maxA,
	                     const glm::vec3& minB, const glm::vec3& maxB)
	{
		if (maxA.x <= minB.x || minA.x >= maxB.x) return false;
		if (maxA.y <= minB.y || minA.y >= maxB.y) return false;
		if (maxA.z <= minB.z || minA.z >= maxB.z) return false;
		return true;
	}

	void CellRange(const glm::vec3& mn, const glm::vec3& mx,
	               int& x0, int& y0, int& z0, int& x1, int& y1, int& z1) const
	{
		const float inv = 1.0f / m_cellSize;
		x0 = (int)std::floor(mn.x * inv); x1 = (int)std::floor(mx.x * inv);
		y0 = (int)std::floor(mn.y * inv); y1 = (int)std::floor(mx.y * inv);
		z0 = (int)std::floor(mn.z * inv); z1 = (int)std::floor(mx.z * inv);
	}

	void Insert(Grid& grid, AABBVector& oversized, const PhysicsDefs::AABB* aabb);
	void RebuildStatics();

	float m_cellSize;
	bool  m_staticsDirty = true;

	AABBVector m_aabbs;

	Grid       m_staticGrid;
	AABBVector m_staticOversized;

	Grid       m_dynamicGrid;
	AABBVector m_dynamicOversized;
	AABBVector m_dynamicBodies;

	AABBVector m_candidates;
	size_t     m_lastCandidateCount = 0;
};

}

#endif // BROADPHASE_UNIFORM_GRID_H
