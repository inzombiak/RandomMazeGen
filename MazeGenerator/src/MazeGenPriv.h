#pragma once

#include "Singleton.h"

namespace MazeDefs {
	struct Vector2f {
		Vector2f() {
			x = 0;
			y = 0;
		}
		Vector2f(float _x, float _y) {
			x = _x;
			y = _y;
		}
		float x, y;
	};

	struct Vector2i {
		Vector2i() {
			x = 0;
			y = 0;
		}
		Vector2i(int _x, int _y) {
			x = _x;
			y = _y;
		}
		int x, y;
	};

	struct IntRect {
		int top, left, width, height;
		bool intersects(const IntRect& other) {
			if (left + width < other.left)
				return false;
			if (left > other.left + other.width)
				return false;
			if (top + height < other.top)
				return false;
			if (top > other.top + other.height)
				return false;

			return true;
		}
	};
}

namespace Private
{
	class SetIDManager
	{
	public:
		inline void Reset()
		{
			m_setMemberCount.clear();
			m_prevRandomSeed = -1;
			m_lastSetID = -1;
		}
		inline int GetCurrentSetID()
		{
			return m_lastSetID;
		}

		inline int GetNextSetID()
		{
			int set = ++m_lastSetID;
			m_setMemberCount[set] = 0;

			return set;
		}
		inline void AddToSet(int set)
		{
			auto it = m_setMemberCount.find(set);
			if (it == m_setMemberCount.end())
				return;

			it->second++;
		}
		inline int GetSetMemberCount(int set)
		{
			auto it = m_setMemberCount.find(set);
			if (it == m_setMemberCount.end())
				return 0;

			return it->second;
		}
		inline void RemoveFromSet(int set)
		{
			auto it = m_setMemberCount.find(set);
			if (it == m_setMemberCount.end())
				return;

			it->second--;
			if (it->second <= 0)
				m_setMemberCount.erase(it);
		}
	private:

		std::default_random_engine m_randomEngine;
		std::map<int, int> m_setMemberCount;
		int m_prevRandomSeed = -1;

		int m_lastSetID = -1;
	};
}

typedef SingletonHolder<Private::SetIDManager, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> SetIDManagerSingleton;