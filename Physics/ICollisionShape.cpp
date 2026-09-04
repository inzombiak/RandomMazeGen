#include "ICollisionShape.h"

namespace orb
{

PhysicsDefs::CollisionShapeType ICollisionShape::GetType()
{
	return m_type;
}
}
