#include "PhysicsWorld.h"
#include "IDebugDraw.h"
#include "IBroadphase.h"
#include "INarrowphase.h"
#include "IConstraintSolver.h"
#include "glm/gtx/quaternion.hpp"

namespace orb
{

PhysicsWorld::PhysicsWorld(IBroadphase* broadphase, INarrowphase* narrowphase, IConstraintSolver* constraintSolver)
{
	if (!broadphase || !narrowphase)
		assert(0);
	m_gravity = glm::vec3(0);
	m_broadphase = broadphase;
	m_narrowphase = narrowphase;
	m_constraintSolver = constraintSolver;
}

void PhysicsWorld::AddRigidBody(IRigidBody* body)
{
	body->SetGravity(m_gravity);
	if (body->IsStatic())
		m_staticRigidBodies.push_back(body);
	else
		m_nonStaticRigidBodies.push_back(body);
	m_broadphase->AddAABB(&body->GetAABB());
}

void PhysicsWorld::RemoveRigidBody(IRigidBody* body)
{
	m_broadphase->RemoveAABB(&body->GetAABB());

	// std::swap on the ITERATORS swapped two local copies and popped whichever
	// body happened to be last, leaving the target in the list.
	std::vector<IRigidBody*>& bodies = body->IsStatic() ? m_staticRigidBodies : m_nonStaticRigidBodies;
	auto it = std::find(bodies.begin(), bodies.end(), body);
	if (it != bodies.end())
	{
		std::swap(*it, bodies.back());
		bodies.pop_back();
	}

	// Manifolds hold raw body pointers, so any referencing this body has to go
	// with it or the next solve dereferences freed memory.
	for (auto mit = m_manifolds.begin(); mit != m_manifolds.end(); )
	{
		if (mit->second.m_bodyA == body || mit->second.m_bodyB == body)
			mit = m_manifolds.erase(mit);
		else
			++mit;
	}
}

void PhysicsWorld::SetGravity(const glm::vec3& grav)
{
	m_gravity = grav;
	for (unsigned int i = 0; i < m_nonStaticRigidBodies.size(); ++i)
	{
		m_nonStaticRigidBodies[i]->SetGravity(grav);
	}
}

void PhysicsWorld::StepSimulation(float timeStep, int maxSubSteps, float fixedTimeStep)
{
	int numSimulationSubSteps = 0;

	if (maxSubSteps)
	{
		//fixed timestep with interpolation
		m_fixedTimeStep = fixedTimeStep;
		m_localTime += timeStep;
		if (m_localTime >= fixedTimeStep)
		{
			numSimulationSubSteps = int(m_localTime / fixedTimeStep);
			m_localTime -= numSimulationSubSteps * fixedTimeStep;
		}
	}
	else
	{
		//variable timestep
		fixedTimeStep = timeStep;
		m_localTime =  timeStep;
		m_fixedTimeStep = 0;
		if (std::fabsf(timeStep) < FLT_EPSILON)
		{
			numSimulationSubSteps = 0;
			maxSubSteps = 0;
		}
		else
		{
			numSimulationSubSteps = 1;
			maxSubSteps = 1;
		}
	}

	//process some debugging flags
	if (numSimulationSubSteps)
	{

		//clamp the number of substeps, to prevent simulation grinding spiralling down to a halt
		int clampedSimulationSteps = (numSimulationSubSteps > maxSubSteps) ? maxSubSteps : numSimulationSubSteps;

		//saveKinematicState(fixedTimeStep*clampedSimulationSteps);

		ApplyGravity();



		for (int i = 0; i<clampedSimulationSteps; i++)
		{
			SingleSimulationStep(fixedTimeStep);

			//internalSingleStepSimulation(fixedTimeStep);
			//synchronizeMotionStates();
		}

	}

	// Debug drawing is optional. Orbitals always installed a drawer in
	// PhysicsSystem::Init, so these unguarded calls never fired there; with the
	// drawer now an injected interface, a null one must simply mean "no debug
	// output" rather than a crash on the first step.
	if (m_physDebugDrawer)
	{
	PhysicsDefs::AABB aabb;
	PhysicsDefs::OBB obb;
	glm::vec3 color(1.f, 0.f, 0.f), colorOBB(0.f, 0.f, 1.f);
	const std::vector<IRigidBody*>* debugLists[2] = { &m_nonStaticRigidBodies, &m_staticRigidBodies };
	for (int l = 0; l < 2; ++l)
	for (unsigned int i = 0; i < debugLists[l]->size(); ++i)
	{
		IRigidBody* dbgBody = (*debugLists[l])[i];
		aabb = dbgBody->GetAABB();
		auto transform = dbgBody->GetTransform();
		aabb.min = transform.GetOrigin() + aabb.min;
		aabb.max = transform.GetOrigin() + aabb.max;
		m_physDebugDrawer->DrawAABB(aabb.min, aabb.max, color);

		obb = dbgBody->GetOBB();

		glm::vec3 currentVertex(-1, -1, -1);
		glm::vec3 x, y, z;
		glm::vec3 from, to;
	
		static const glm::vec3 OFFSET(-0.01f, -0.01f, -0.01f);
		for (unsigned int i = 0; i < 4; ++i)
		{
			x = obb.localAxes[0] * obb.halfExtents.x * currentVertex.x;
			y = obb.localAxes[1] * obb.halfExtents.y * currentVertex.y;
			z = obb.localAxes[2] * obb.halfExtents.z * currentVertex.z;

			from = x + y + z;
			from += obb.pos;

			for (unsigned int j = 0; j < 3; ++j)
			{
				to = from;
				to -= 2.f * (currentVertex[j] * obb.halfExtents[j] * obb.localAxes[j]);

				m_physDebugDrawer->DrawLine(from, to, colorOBB);
			}

			currentVertex[(i % 2) + 1] *= -1;
			currentVertex[((i + 1) % 2) * 2] *= -1;
		}
	}

	if (m_narrowphaseError.size() > 1)
	{
		for (int i = 0; i < m_narrowphaseError.size(); ++i)
		{
			m_physDebugDrawer->DrawLine(m_narrowphaseError[i], m_narrowphaseError[(i + 1) % m_narrowphaseError.size()], glm::vec3(1.f, 1.f, 0.f));
		}
	}

	glm::vec3 pos1, pos2;
	PhysicsDefs::ContactInfo ci;
	if (m_manifolds.size() > 0)
	{
		ManifoldMapIter it;
		for (it = m_manifolds.begin(); it != m_manifolds.end(); ++it)
		{
			pos1 =it->second.m_bodyA->GetInterpolationTransform().GetOrigin();
			pos2 =it->second.m_bodyB->GetInterpolationTransform().GetOrigin();
			m_physDebugDrawer->DrawPoint(pos1, 0.2f, glm::vec3(0, 1, 0));
			m_physDebugDrawer->DrawPoint(pos2, 0.5f, glm::vec3(0, 0, 1));
			for (int j = 0; j < it->second.m_contacts.size(); ++j)
			{
				ci = it->second.m_contacts[j];
				m_physDebugDrawer->DrawPoint(ci.worldPos, 0.2f, glm::vec3(0, 1, 1));
				m_physDebugDrawer->DrawLine(pos1, pos1 + it->second.m_bodyA->GetOBB().localAxes * ci.localPointA, glm::vec3(0.5, 0.5, 0.5));
				m_physDebugDrawer->DrawLine(pos2, pos2 + it->second.m_bodyB->GetOBB().localAxes * ci.localPointB, glm::vec3(0.5, 0.f, 0.5));
			}
		}
	}
	}

	ClearForces();
}

void PhysicsWorld::ApplyGravity()
{
	for (unsigned int i = 0; i< m_nonStaticRigidBodies.size(); ++i)
	{
		m_nonStaticRigidBodies[i]->ApplyGravity();
	}
}

void PhysicsWorld::PredictMotion(float timeStep)
{
	float angMag;
	glm::vec3 linVel, angVel, totalF, pos;
	OTransform trans, finalTrans;
	glm::quat rot;
	for (unsigned int i = 0; i < m_nonStaticRigidBodies.size(); ++i)
	{
		m_nonStaticRigidBodies[i]->ApplyDamping(timeStep);

		linVel = m_nonStaticRigidBodies[i]->GetLinearVelocity();
		angVel = m_nonStaticRigidBodies[i]->GetAngularVelocity();
		totalF = m_nonStaticRigidBodies[i]->GetTotalForce();
		angMag = glm::length(angVel);

		if (m_nonStaticRigidBodies[i]->GetMass() != 0)
			linVel += totalF / m_nonStaticRigidBodies[i]->GetMass() * timeStep;

		trans = m_nonStaticRigidBodies[i]->GetTransform();
		rot = trans.GetRotation();
		pos = trans.GetOrigin();

		rot *= glm::quat(angVel* timeStep);
		pos += linVel * timeStep;
		rot = glm::normalize(rot);
		trans.SetOrigin(pos);
		trans.SetRotation(rot);

		m_nonStaticRigidBodies[i]->UpdateInterpolationTransform(trans);
		m_nonStaticRigidBodies[i]->SetLinearVelocity(linVel);

	}
}

void PhysicsWorld::PerformMovement(float timeStep)
{
	float angMag;
	glm::vec3 linVel, angVel, pos;
	OTransform trans, finalTrans;
	glm::quat rot;
	for (unsigned int i = 0; i < m_nonStaticRigidBodies.size(); ++i)
	{
		linVel = m_nonStaticRigidBodies[i]->GetLinearVelocity() ;
		angVel = m_nonStaticRigidBodies[i]->GetAngularVelocity();
		angMag = glm::length(angVel);

		
		trans = m_nonStaticRigidBodies[i]->GetTransform();

		rot = trans.GetRotation();
		pos = trans.GetOrigin();

		rot *= glm::quat(angVel* timeStep);
		pos += linVel * timeStep;
		rot = glm::normalize(rot);
		trans.SetOrigin(pos);
		trans.SetRotation(rot);
		if (glm::dot(linVel, linVel) < 0.00001)
		{
			linVel = glm::vec3(0);
			m_nonStaticRigidBodies[i]->SetLinearVelocity(linVel);
		}
		if (glm::dot(angVel, angVel) < 0.00001)
		{
			angVel = glm::vec3(0);
			m_nonStaticRigidBodies[i]->SetAngularVelocity(angVel);
		}

		m_nonStaticRigidBodies[i]->UpdateTransform(trans);
	}
}

void PhysicsWorld::PerformCollisionCheck(float dt)
{
	auto collidingPairs = m_broadphase->GetCollisionPairs();

	if (collidingPairs.size() == 0)
	{
		m_manifolds.clear();
		return;
	}

	std::vector<Manifold> newManifolds;
	newManifolds = m_narrowphase->CheckCollision(collidingPairs, NarrowphaseErrorCalback);
	if (newManifolds.size() == 0)
	{
		m_manifolds.clear();
		return;
	}

	ManifoldMapIter it;
	for (int i = 0; i < newManifolds.size(); ++i)
	{
		ManifoldKey key(newManifolds[i].m_bodyA, newManifolds[i].m_bodyB);

		it = m_manifolds.find(key);
		if (it == m_manifolds.end())
			continue;

		it->second.Update(newManifolds[i].m_contacts.data(), (unsigned int)newManifolds[i].m_contacts.size());
		newManifolds[i].m_isPersistent = true;
		newManifolds[i].m_contacts = it->second.m_contacts;
	}

	m_constraintSolver->SolveConstraints2(newManifolds, dt);

	ManifoldMap manifoldMap;
	for (int i = 0; i < newManifolds.size(); ++i)
	{
		ManifoldKey key(newManifolds[i].m_bodyA, newManifolds[i].m_bodyB);
		manifoldMap.emplace(key, newManifolds[i]);
	}
	m_manifolds = manifoldMap;
}

void PhysicsWorld::NarrowphaseErrorCalback(std::vector<glm::vec3> finalResult)
{
	m_narrowphaseError = finalResult;
}

void PhysicsWorld::ClearForces()
{
	for (unsigned int i = 0; i < m_nonStaticRigidBodies.size(); ++i)
	{
		m_nonStaticRigidBodies[i]->ClearForces();
	}
}

void PhysicsWorld::SingleSimulationStep(float fixedTimeStep)
{
	//Predict motion
	PredictMotion(fixedTimeStep);

	//Check collisions and apply
	PerformCollisionCheck(fixedTimeStep);

	//Solve Constraints
	//SolveConstraints();

	//Perform movement
	PerformMovement(fixedTimeStep);
}

void PhysicsWorld::SetPhysDebugDrawer(IDebugDraw* pdd)
{
	m_physDebugDrawer = pdd;
}

std::vector<glm::vec3> PhysicsWorld::m_narrowphaseError;
}
