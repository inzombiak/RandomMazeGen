#ifndef I_DEBUG_DRAW_H
#define I_DEBUG_DRAW_H

#include <Core/MathConfig.h>

namespace orb
{
	// Rendering seam for the physics debug visualisation.
	//
	// PhysicsWorld::StepSimulation used to call a concrete OpenGL PhysDebugDrawer
	// directly, which made the physics core depend on the renderer. The host now
	// implements this interface instead, so Physics links against nothing but GLM.
	//
	// These are the only three primitives the simulation actually emits: AABB and
	// OBB wireframes, contact points, and contact-normal lines.
	class IDebugDraw
	{
	public:
		virtual ~IDebugDraw() {}

		virtual void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) = 0;
		virtual void DrawPoint(const glm::vec3& p, float size, const glm::vec3& color) = 0;
		virtual void DrawAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) = 0;
	};
}

#endif // I_DEBUG_DRAW_H
