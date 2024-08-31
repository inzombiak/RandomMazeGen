#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#define USE_SFML 0

#include <chrono>
#include <random> 
#include <array>
#include <map>

#include "Singleton.h"
#include <SFML/Graphics/Color.hpp>
#include "Math.h"

namespace GameDefs
{
	enum GenerateType
	{
		Step,
		Full
	};

	enum TileType
	{
		Empty = 0,
		Passage = 1,
		Room = 2,
	};

	enum PassageDirection
	{
		North = 1,
		East = 2,
		South = 4,
		West = 8,
		None = 16
	};

	inline PassageDirection operator|(PassageDirection a, PassageDirection b)
	{
		return static_cast<PassageDirection>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline PassageDirection operator&(PassageDirection a, PassageDirection b)
	{
		return static_cast<PassageDirection>(static_cast<int>(a) & static_cast<int>(b));
	}
	inline PassageDirection operator~(PassageDirection a)
	{
		return static_cast<PassageDirection>(~static_cast<int>(a));
	}

	//Set management
	
	namespace Private
	{
		class SetIDManager
		{
		public:
			inline void Reset()
			{
				m_setIDToColor.clear();
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
			inline sf::Color GetSetColor(int set, int seed)
			{
				auto it = m_setIDToColor.find(set);
				if (it != m_setIDToColor.end())
					return it->second;

				std::uniform_int_distribution<int> dist(0, 255);

				if (seed != m_prevRandomSeed)
				{
					m_prevRandomSeed = seed;
					m_randomEngine.seed(seed);
				}

				sf::Color color;

				color.a = 255;
				color.r = dist(m_randomEngine);
				color.g = dist(m_randomEngine);
				color.b = dist(m_randomEngine);

				m_setIDToColor[set] = color;

				return color;

			}
		private:

			typedef std::map<int, sf::Color> SetIDToColorMap;
			SetIDToColorMap m_setIDToColor;
			std::default_random_engine m_randomEngine;
			std::map<int, int> m_setMemberCount;
			int m_prevRandomSeed = -1;

			int m_lastSetID = -1;
		};
	}

	typedef SingletonHolder<Private::SetIDManager, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> SetIDManagerSingleton;

	static std::array<PassageDirection, 4>			DIRECTIONS = { GameDefs::East, GameDefs::West, GameDefs::North, GameDefs::South };
	static std::array<PassageDirection, 4>			OPPOSITE_DIRECTIONS = { GameDefs::West, GameDefs::East, GameDefs::South, GameDefs::North };;
	static std::array<std::pair<int, int>, 4>		DIRECTION_CHANGES = { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };
}

class Renderer_D12;
class Window;
namespace Globals {

	struct StartupValues {
		int window_width = 1200;
		int window_height = 800;
		const wchar_t* window_className = L"DX12WindowClass";
		bool use_warp = false;
	};

	extern bool VSYNC_ENABLED;

	extern StartupValues STARTUP_VALS;
	
	struct InputState {
		sf::Vector2i mousePos;
		int mouseBtnState = 0;

		sf::Vector2i lastMouseDownPos = sf::Vector2i(0, 0);
	};

	extern InputState INPUT_STATE;
}

extern std::shared_ptr<Window>		 GAME_WINDOW;
extern std::shared_ptr<Renderer_D12> RENDERER;

#endif