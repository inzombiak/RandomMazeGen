#ifndef CORE_SINGLETON_H
#define CORE_SINGLETON_H

// Meyers singleton.
//
// Replaces the Loki/Alexandrescu policy-based SingletonHolder that previously
// lived in MazeGenerator/src/Singleton.h (and, before the physics merge, in
// Orbitals' Metaprogramming/Common/Singleton.h).
//
// Why it went: every instantiation in this codebase used the identical
// <CreateWithNew, DefaultLifetime> pair, so the combinatorial payoff that
// justifies policy-based design was never realised -- and SingletonHolder's
// Instance() carried an unimplemented "//TODO: Thread guard" while GridManager
// reaches its singletons from detached worker threads.
//
// Since C++11, initialisation of a function-local static is guaranteed
// thread-safe ("magic statics"), and statics are destroyed in reverse order of
// construction. That ordering subsumes the dead-reference problem the old
// m_destroyed flag existed to detect: a singleton touched while another is
// being constructed is necessarily destroyed after it.
//
// What was given up: explicit cross-TU destruction ordering (Loki's
// SingletonWithLongevity) and resurrection semantics (PhoenixSingleton).
// Neither was used.
//
// Call sites are unchanged -- FooSingleton::Instance().

namespace Core
{
	template <class T>
	class Singleton
	{
	public:
		static T& Instance()
		{
			static T s_instance;
			return s_instance;
		}

		Singleton() = delete;
		~Singleton() = delete;
		Singleton(const Singleton&) = delete;
		Singleton& operator=(const Singleton&) = delete;
	};
}

#endif // CORE_SINGLETON_H
