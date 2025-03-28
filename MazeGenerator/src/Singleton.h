#ifndef SINGLETON_H
#define SINGLETON_H

#include <memory>
#include <algorithm>
#include <list>
#include <vector>
#include <assert.h>

typedef void(__cdecl *atexit_pfn_t)();

namespace SingletonPrivate
{
	class LifetimeTracker
	{
	public:
		LifetimeTracker(unsigned int x) : m_longevity(x)
		{}

		virtual ~LifetimeTracker() = 0;

		static bool Compare(const LifetimeTracker* lhs,
			const LifetimeTracker* rhs)
		{
			return lhs->m_longevity > rhs->m_longevity;
		}

	private:
		unsigned int m_longevity;
	};

	inline LifetimeTracker::~LifetimeTracker() {}

	template <typename T>
	struct Deleter
	{
		typedef void(*Type)(T*);
		static void Delete(T* pObj)
		{
			delete pObj;
		}
	};

	template <typename T, typename Destroyer>
	class ConcreteLifetimeTracker : public LifetimeTracker
	{
	public:
		ConcreteLifetimeTracker(T* p, unsigned int longevity, Destroyer d)
			: LifetimeTracker(longevity)
			, m_tracked(p)
			, m_destroyer(d)
		{}

		~ConcreteLifetimeTracker()
		{
			m_destroyer(m_tracked);
		}

	private:
		T* m_tracked;
		Destroyer m_destroyer;

	};

	template <class T>
	struct Adapter
	{
		void operator()(T*) { return m_pFun(); }
		atexit_pfn_t m_pFun;
	};

	typedef std::list<LifetimeTracker*> TrackerArray;
	extern TrackerArray pTrackerArray;
	extern unsigned int elements;
}

namespace LifetimePolicies
{
	typedef atexit_pfn_t AtExitFn();

	template <class T>
	struct DefaultLifetime
	{
		static void ScheduleDestruction(T*, atexit_pfn_t pFun)
		{
			std::atexit(pFun);
		}

		static void OnDeadReference()
		{
			throw std::logic_error("Dead Reference Detected!");
		}
	};

	template <class T>
	class PhoenixSingle
	{
	public:
		static void ScheduleDestruction(T*, atexit_pfn_t *pFun)
		{
			std::atexit(pFun);
		}

		static void OnDeadReference()
		{
		}
	};

	template <class T>
	class DeletableSingleton
	{
	public:

		static void ScheduleDestruction(T*, atexit_pfn_t pFun)
		{
			static bool firstPass = true;
			m_isDead = false;
			m_deleter = pFun;
			if (firstPass || m_needCallback)
			{
				std::atexit(atexitCallback);
				firstPass = false;
				m_needCallback = false;
			}
		}

		static void OnDeadReference()
		{
		}
		static void GracefulDelete()
		{
			if (m_isDead)
				return;
			m_isDead = true;
			m_deleter();
		}

	protected:
		static atexit_pfn_t m_deleter;
		static bool m_isDead;
		static bool m_needCallback;

		static void atexitCallback()
		{
			m_needCallback = false;
			GracefulDelete();
		}
	};

	template <class T>
	atexit_pfn_t DeletableSingleton<T>::m_deleter = 0;

	template <class T>
	bool DeletableSingleton<T>::m_isDead = true;

	template <class T>
	bool DeletableSingleton<T>::m_needCallback = true;

}

namespace CreationPolicies
{
	template <class T>
	struct CreateWithNew
	{
		static T* Create()
		{
			return new T;
		}

		static void Destroy(T* p)
		{
			delete p;
		}
	};

	template <class T> struct CreateWithMalloc
	{
		static T* Create()
		{
			void* p = std::malloc(sizeof(T));
			if (!p) return 0;
			return new(p)T;
		}

		static void Destroy(T* p)
		{
			p->~T();
			std::free(p);
		}
	};

	template<template<class> class Alloc>
	struct CreateWithCustomAllocator
	{
		template <class T>
		struct Allocator
		{
			static Alloc<T> allocator;

			static T* Create()
			{
				return new (allocator.allocate(1)) T;
			}

			static void Destroy(T* p)
			{
				//allocator.destroy(p);
				p->~T();
				allocator.deallocate(p, 1);
			}
		};
	};
}

template
<
	class T,
	template <class> class CreationPolicy,
	template <class> class LifetimePolicy
>
class SingletonHolder
{
public:
	static T& Instance()
	{
		if (!m_instance)
		{
			//TODO: Thread guard
			if (!m_instance)
			{
				if (m_destroyed)
				{
					LifetimePolicy<T>::OnDeadReference();
					m_destroyed = false;
				}

				m_instance = CreationPolicy<T>::Create();
				LifetimePolicy<T>::ScheduleDestruction(m_instance, (atexit_pfn_t)DestorySingleton);
			}
		}

		return *m_instance;
	}

private:
	static void DestorySingleton()
	{

		assert(!m_destroyed);
		CreationPolicy<T>::Destroy(m_instance);
		m_destroyed = true;

	}

	SingletonHolder();
	SingletonHolder(const SingletonHolder&) {};
	SingletonHolder& operator=(const SingletonHolder&) {};
	~SingletonHolder() {};

	static T* m_instance;
	static bool m_destroyed;
};


template
<
	class T,
	template <class> class C,
	template <class> class L
>
T* SingletonHolder<T, C, L>::m_instance = 0;

template
<
	class T,
	template <class> class C,
	template <class> class L
>
bool SingletonHolder<T, C, L>::m_destroyed = false;


#endif