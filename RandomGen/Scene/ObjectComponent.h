#ifndef OBJECT_COMPONENT_H
#define OBJECT_COMPONENT_H

#include <memory>

class Entity;

// Base for anything attachable to an Entity.
//
// Ported from Orbitals' IObjectComponent, with its ownership model corrected.
// The original SetOwner did:
//
//     m_owner->AddComponent(std::shared_ptr<IObjectComponent>(this));
//
// while the owning systems stored components BY VALUE in std::vector. That
// shared_ptr would delete a pointer into a vector's buffer that was never
// new'd, and any push_back reallocation left every m_owner dangling. Components
// are now created as shared_ptr by their system and handed to the entity
// explicitly, so SetOwner only records the back-pointer.
class ObjectComponent
{
public:
	ObjectComponent() = default;
	virtual ~ObjectComponent() = default;

	virtual void Update(float dt) {}

	// Records the back-pointer only. The caller attaches the component via
	// Entity::AddComponent, which is what holds the shared_ptr.
	void SetOwner(Entity* owner) { m_owner = owner; }
	Entity* GetOwner() const { return m_owner; }

	void SetInUse(bool inUse) { m_inUse = inUse; }
	bool GetInUse() const { return m_inUse; }

	unsigned int GetID() const { return m_id; }

	virtual const char* GetName() const = 0;
	virtual size_t GetComponentID() const = 0;

protected:
	Entity*      m_owner = nullptr;
	bool         m_inUse = true;
	unsigned int m_id = 0;
};

#endif // OBJECT_COMPONENT_H
