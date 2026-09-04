#include "PhysicsScene.h"

#include "PhysicsWorld.h"
#include "BroadphaseAABB.h"
#include "NarrowphaseSAT.h"
#include "ConstraintSolverSeqImpulse.h"
#include "BoxShape.h"
#include "IRigidBody.h"

const float PhysicsScene::FIXED_TIMESTEP    = 1.0f / 60.0f;
const int   PhysicsScene::MAX_SUBSTEPS      = 8;
const float PhysicsScene::MAX_FRAME_SECONDS = 0.25f;

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

	m_renderScales.clear();
	m_isStatic.clear();

	delete m_world;      m_world = nullptr;
	delete m_broadphase; m_broadphase = nullptr;
	delete m_narrowphase; m_narrowphase = nullptr;
	delete m_solver;     m_solver = nullptr;
}

orb::IRigidBody* PhysicsScene::AddBox(const glm::vec3& extents, const glm::vec3& position,
                                      float mass, bool enableGravity)
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
	rbci.transform.SetIdentity();
	rbci.transform.SetOrigin(position);

	orb::IRigidBody* body = new orb::IRigidBody(rbci);

	m_shapes.push_back(shape);
	m_bodies.push_back(body);
	m_renderScales.push_back(extents * 0.5f);
	m_isStatic.push_back(mass == 0.0f);

	m_world->AddRigidBody(body);
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

	// Ground slab: mass 0, gravity off.
	AddBox(glm::vec3(60.0f, 2.0f, 60.0f), glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, false);

	// The handedness probe. Gravity off and a torque impulse about -X, so it
	// hangs in place and only spins. If the render basis disagrees with the
	// solver, this is the object that shows it -- it will spin the wrong way
	// while everything else still looks plausible.
	orb::IRigidBody* spinner = AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 3.0f, -7.0f), 5.0f, false);
	spinner->ApplyTorqueImpulse(glm::vec3(-4.0f, 0.0f, 0.0f));

	// Falling stack.
	AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 14.0f, 0.0f), 5.0f, true);
	AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 18.0f, 0.0f), 1.0f, true);
	AddBox(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 22.0f, 0.0f), 1.0f, true);
}

void PhysicsScene::Step(float frameSeconds)
{
	if (!m_world)
		return;

	if (frameSeconds > MAX_FRAME_SECONDS)
		frameSeconds = MAX_FRAME_SECONDS;

	m_world->StepSimulation(frameSeconds, MAX_SUBSTEPS, FIXED_TIMESTEP);
}

void PhysicsScene::CollectBodyViews(std::vector<BodyView>& out) const
{
	out.clear();
	out.reserve(m_bodies.size());

	for (size_t i = 0; i < m_bodies.size(); ++i)
	{
		const orb::OTransform& t = m_bodies[i]->GetTransform();

		BodyView v;
		v.position = t.GetOrigin();
		// GetRotation()/SetRotation() are the pair the integrator itself uses.
		// OTransform::GetOpenGLMatrix() transposes the basis and would hand back
		// the inverse rotation, which is not what the solver is working in.
		v.orientation  = t.GetRotation();
		v.renderScale  = m_renderScales[i];
		v.isStatic     = m_isStatic[i];
		out.push_back(v);
	}
}
