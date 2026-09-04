#ifndef PHYSICS_SCENE_H
#define PHYSICS_SCENE_H

#include <Core/MathConfig.h>

#include <vector>

namespace orb
{
	class PhysicsWorld;
	class IBroadphase;
	class INarrowphase;
	class IConstraintSolver;
	class IRigidBody;
	class ICollisionShape;
}

// Owns a physics world and everything in it.
//
// This is the integration seam between the Physics lib and the DX12 host: it
// builds bodies, drives the simulation from real frame time, and hands back
// plain transforms the renderer can turn into Renderables. It deliberately
// knows nothing about D3D, and Physics knows nothing about it.
class PhysicsScene
{
public:
	// A body reduced to what the renderer needs.
	struct BodyView
	{
		glm::vec3 position;
		glm::quat orientation;
		glm::vec3 renderScale;   // half-extents: the shared box mesh spans -1..+1
		bool      isStatic;
	};

	PhysicsScene();
	~PhysicsScene();

	PhysicsScene(const PhysicsScene&) = delete;
	PhysicsScene& operator=(const PhysicsScene&) = delete;

	// Reproduces Orbitals' Engine::Test(): a static ground slab, one box that
	// only spins (gravity off, torque impulse), and a three-box falling stack.
	void BuildBoxStackTest();
	void Clear();

	// Feed real frame time; PhysicsWorld does the fixed-step accumulation and
	// clamps the substep count itself.
	void Step(float frameSeconds);

	bool   IsBuilt() const { return m_world != nullptr && !m_bodies.empty(); }
	size_t BodyCount() const { return m_bodies.size(); }
	void   CollectBodyViews(std::vector<BodyView>& out) const;

	// Orbitals ran 1/60 with maxSubSteps=1, which silently drops time whenever a
	// frame runs long. Allowing catch-up steps keeps the sim in step with the
	// wall clock; the cap is what stops a slow frame spiralling.
	static const float FIXED_TIMESTEP;
	static const int   MAX_SUBSTEPS;
	static const float MAX_FRAME_SECONDS;

private:
	orb::IRigidBody* AddBox(const glm::vec3& extents, const glm::vec3& position,
	                        float mass, bool enableGravity);

	orb::PhysicsWorld*      m_world = nullptr;
	orb::IBroadphase*       m_broadphase = nullptr;
	orb::INarrowphase*      m_narrowphase = nullptr;
	orb::IConstraintSolver* m_solver = nullptr;

	std::vector<orb::IRigidBody*>      m_bodies;
	std::vector<orb::ICollisionShape*> m_shapes;
	std::vector<glm::vec3>             m_renderScales;
	std::vector<bool>                  m_isStatic;
};

#endif // PHYSICS_SCENE_H
