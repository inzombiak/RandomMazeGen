#include "PhysicsScene.h"

#include "PhysicsWorld.h"
#include "BroadphaseAABB.h"
#include "NarrowphaseSAT.h"
#include "ConstraintSolverSeqImpulse.h"
#include "BoxShape.h"
#include "IRigidBody.h"
#include "PhysicsComponent.h"
#include "RenderComponent.h"

const float PhysicsScene::FIXED_TIMESTEP    = 1.0f / 60.0f;
const int   PhysicsScene::MAX_SUBSTEPS      = 8;
const float PhysicsScene::MAX_FRAME_SECONDS = 0.25f;
const float PhysicsScene::DEFAULT_LINEAR_DAMPING  = 0.0f;
const float PhysicsScene::DEFAULT_ANGULAR_DAMPING = 0.0f;

PhysicsScene::PhysicsScene()
{
}

PhysicsScene::~PhysicsScene()
{
	Clear();
}

void PhysicsScene::Clear()
{
	for (orb::IRigidBody* b : m_bodies)
		delete b;
	m_bodies.clear();

	for (orb::ICollisionShape* s : m_shapes)
		delete s;
	m_shapes.clear();

	m_entities.clear();
	m_renderScales.clear();
	m_isStatic.clear();

	delete m_world;      m_world = nullptr;
	delete m_broadphase; m_broadphase = nullptr;
	delete m_narrowphase; m_narrowphase = nullptr;
	delete m_solver;     m_solver = nullptr;
}

orb::IRigidBody* PhysicsScene::AddBox(const glm::vec3& extents, const glm::vec3& position,
                                      float mass, bool enableGravity,
                                      float linearDamping, float angularDamping)
{
	// BoxShape takes FULL extents and halves them internally, so the render
	// scale is half the extents -- the shared box mesh already spans -1..+1.
	orb::BoxShape* shape = new orb::BoxShape(extents);

	orb::PhysicsDefs::RigidBodyConstructionInfo rbci;
	rbci.mass           = mass;
	rbci.enableGravity  = enableGravity;
	rbci.collisionShape = shape;
	rbci.friction       = 0.5f;
	rbci.resititution   = 0.1f;
	rbci.linearDamping  = linearDamping;
	rbci.angularDamping = angularDamping;
	rbci.transform.SetIdentity();
	rbci.transform.SetOrigin(position);

	orb::IRigidBody* body = new orb::IRigidBody(rbci);

	m_shapes.push_back(shape);
	m_bodies.push_back(body);
	m_renderScales.push_back(extents * 0.5f);
	m_isStatic.push_back(mass == 0.0f);

	m_world->AddRigidBody(body);

	// Give the body an entity with a physics + render component. The physics
	// component is what pulls the transform across each frame.
	{
		auto entity = std::make_shared<Entity>("Body" + std::to_string(m_entities.size()),
		                                       (unsigned int)m_entities.size());
		entity->SetPosition(position);
		entity->SetScale(extents * 0.5f);

		auto physComp = std::make_shared<PhysicsComponent>((unsigned int)m_entities.size());
		physComp->SetBody(body);
		entity->AddComponent(physComp);

		auto renderComp = std::make_shared<RenderComponent>((unsigned int)m_entities.size());
		renderComp->SetRenderScale(extents * 0.5f);
		renderComp->SetEntityData(mass == 0.0f ? 0u : 1u);
		entity->AddComponent(renderComp);

		m_entities.push_back(entity);
	}

	return body;
}

void PhysicsScene::BuildBoxStackTest()
{
	Clear();

	m_broadphase  = new orb::BroadphaseAABB();
	m_narrowphase = new orb::NarrowphaseSAT();
	m_solver      = new orb::ConstraintSolverSeqImpulse();
	m_world       = new orb::PhysicsWorld(m_broadphase, m_narrowphase, m_solver);

	// PhysicsSystem::Init set this; it is not part of the ported core.
	m_world->SetGravity(glm::vec3(0.0f, -9.8f, 0.0f));
	m_world->SetPhysDebugDrawer(m_debugDraw);

	// Ground slab: mass 0, gravity off.
	AddBox(glm::vec3(60.0f, 2.0f, 60.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, false, 0.0f, 0.0f);

	// The handedness probe. Gravity off and a torque impulse about -X, so it
	// hangs in place and only spins. If the render basis disagrees with the
	// solver, this is the object that shows it -- it will spin the wrong way
	// while everything else still looks plausible.
	orb::IRigidBody* spinner = AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 3.0f, -7.0f), 5.0f, false, 0.0f, 0.0f);
	spinner->ApplyTorqueImpulse(glm::vec3(-4.0f, 0.0f, 0.0f));

	// Falling stack.
	AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 14.0f, 0.0f), 5.0f, true);
	AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 18.0f, 0.0f), 1.0f, true);
	AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 22.0f, 0.0f), 1.0f, true);
}

void PhysicsScene::SetDebugDraw(orb::IDebugDraw* debugDraw)
{
	m_debugDraw = debugDraw;
	if (m_world)
		m_world->SetPhysDebugDrawer(m_debugDraw);
}

void PhysicsScene::Step(float frameSeconds)
{
	if (!m_world)
		return;

	if (frameSeconds > MAX_FRAME_SECONDS)
		frameSeconds = MAX_FRAME_SECONDS;

	m_world->StepSimulation(frameSeconds, MAX_SUBSTEPS, FIXED_TIMESTEP);
}

void PhysicsScene::UpdateComponents(float dt)
{
	for (auto& entity : m_entities)
		entity->UpdateComponents(dt);
}

void PhysicsScene::GetResidualMotion(float& outMaxLinear, float& outMaxAngular) const
{
	outMaxLinear  = 0.0f;
	outMaxAngular = 0.0f;

	for (size_t i = 2; i < m_bodies.size(); ++i)
	{
		if (m_isStatic[i])
			continue;

		const float lin = glm::length(m_bodies[i]->GetLinearVelocity());
		const float ang = glm::length(m_bodies[i]->GetAngularVelocity());
		if (lin > outMaxLinear)  outMaxLinear  = lin;
		if (ang > outMaxAngular) outMaxAngular = ang;
	}
}

void PhysicsScene::CollectBodyViews(std::vector<BodyView>& out) const
{
	out.clear();
	out.reserve(m_entities.size());

	for (size_t i = 0; i < m_entities.size(); ++i)
	{
		const Entity& e = *m_entities[i];
		auto renderComp = e.GetComponent<RenderComponent>();

		BodyView v;
		// Written by PhysicsComponent::Update from the body transform.
		v.position    = e.GetPosition();
		v.orientation = e.GetRotation();
		v.renderScale = renderComp ? renderComp->GetRenderScale() : e.GetScale();
		v.isStatic    = m_isStatic[i];
		out.push_back(v);
	}
}
