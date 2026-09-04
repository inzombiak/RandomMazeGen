#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

#include "ObjectComponent.h"
#include "Entity.h"

#include "IRigidBody.h"

// Drives its Entity's transform from a rigid body.
//
// Orbitals' version pushed OTransform::GetOpenGLMatrix() into the entity. That
// matrix is built from the TRANSPOSED basis, which is the inverse rotation of
// what GetRotation()/SetRotation() -- the pair the integrator itself uses --
// represent. This reads the rotation directly instead.
class PhysicsComponent : public ObjectComponent
{
public:
	static const char*  COMPONENT_NAME;
	static const size_t COMPONENT_ID;

	explicit PhysicsComponent(unsigned int id) { m_id = id; }

	void SetBody(orb::IRigidBody* body) { m_rigidBody = body; }
	orb::IRigidBody* GetBody() const { return m_rigidBody; }

	void Update(float dt) override
	{
		if (!m_rigidBody || !m_owner)
			return;

		const orb::OTransform& t = m_rigidBody->GetTransform();
		m_owner->SetPosition(t.GetOrigin());
		m_owner->SetRotation(t.GetRotation());
	}

	void ApplyImpulse(const glm::vec3& impulse)
	{
		if (m_rigidBody) m_rigidBody->ApplyImpulse(impulse);
	}
	void ApplyTorqueImpulse(const glm::vec3& torque)
	{
		if (m_rigidBody) m_rigidBody->ApplyTorqueImpulse(torque);
	}
	void ApplyForce(const glm::vec3& force)
	{
		if (m_rigidBody) m_rigidBody->ApplyForce(force);
	}

	const char* GetName() const override { return COMPONENT_NAME; }
	size_t GetComponentID() const override { return COMPONENT_ID; }

private:
	orb::IRigidBody* m_rigidBody = nullptr;
};

#endif // PHYSICS_COMPONENT_H
