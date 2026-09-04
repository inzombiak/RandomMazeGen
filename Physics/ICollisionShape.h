#ifndef I_COLLISION_SHAPE_H
#define I_COLLISION_SHAPE_H

#include "PhysicsDefs.h"
#include <Core/MathConfig.h>

namespace orb
{

class ICollisionShape
{
public:

	virtual ~ICollisionShape()
	{
		;
	}

	virtual glm::mat3 GetTensor(float mass) = 0;
	virtual PhysicsDefs::OBB GetLocalOBB() = 0;
	PhysicsDefs::CollisionShapeType GetType();
	virtual glm::vec3 GetSupportPoint(const glm::vec3& dir) const = 0;

protected:
	PhysicsDefs::CollisionShapeType m_type;

};

}

#endif