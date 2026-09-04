#ifndef ENTITY_H
#define ENTITY_H

#include <Core/MathConfig.h>

#include "ObjectComponent.h"

#include <memory>
#include <string>
#include <unordered_map>

// A named thing in the world: a transform plus a set of components.
//
// Ported from Orbitals' ICelestialObject. Renamed on the way in: it was neither
// an interface (it is concrete and instantiated directly) nor celestial (a
// leftover from Orbitals' original orbital-mechanics premise), and the I-prefix
// on a concrete class was one of the traps flagged before the port.
//
// The original also carried a GetActorComponentByID template that did not
// compile if instantiated -- it referenced an undeclared EntityComponents type
// and an unqualified weak_ptr. That is replaced by GetComponent<T>() below.
class Entity
{
public:
	Entity(std::string name, unsigned int id)
		: m_name(std::move(name)), m_id(id)
	{}

	// --- transform -------------------------------------------------------
	void SetPosition(const glm::vec3& pos) { m_position = pos; }
	const glm::vec3& GetPosition() const { return m_position; }

	void SetRotation(const glm::quat& rot) { m_rotation = rot; }
	const glm::quat& GetRotation() const { return m_rotation; }

	void SetScale(const glm::vec3& scale) { m_scale = scale; }
	const glm::vec3& GetScale() const { return m_scale; }

	// --- components ------------------------------------------------------
	void AddComponent(const std::shared_ptr<ObjectComponent>& component)
	{
		if (!component)
			return;
		component->SetOwner(this);
		m_components[component->GetComponentID()] = component;
	}

	std::shared_ptr<ObjectComponent> GetComponent(size_t componentId) const
	{
		auto it = m_components.find(componentId);
		return it == m_components.end() ? nullptr : it->second;
	}

	// Typed lookup. T must expose a static COMPONENT_ID.
	template <class T>
	std::shared_ptr<T> GetComponent() const
	{
		auto it = m_components.find(T::COMPONENT_ID);
		if (it == m_components.end())
			return nullptr;
		return std::static_pointer_cast<T>(it->second);
	}

	void UpdateComponents(float dt)
	{
		for (auto& [id, component] : m_components)
			if (component && component->GetInUse())
				component->Update(dt);
	}

	const std::string& GetName() const { return m_name; }
	unsigned int GetID() const { return m_id; }

private:
	std::string  m_name;
	unsigned int m_id = 0;

	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 m_scale    = glm::vec3(1.0f);

	std::unordered_map<size_t, std::shared_ptr<ObjectComponent>> m_components;
};

#endif // ENTITY_H
