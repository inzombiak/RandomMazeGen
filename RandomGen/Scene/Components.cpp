#include "PhysicsComponent.h"
#include "RenderComponent.h"

#include <string>

// Component ids are hashes of the component name, as in Orbitals -- stable
// across runs and cheap to compare, without a central registry to keep in sync.
static size_t HashName(const char* name)
{
	return std::hash<std::string>{}(std::string(name));
}

const char*  PhysicsComponent::COMPONENT_NAME = "PHYSICS_COMPONENT";
const size_t PhysicsComponent::COMPONENT_ID   = HashName(PhysicsComponent::COMPONENT_NAME);

const char*  RenderComponent::COMPONENT_NAME  = "RENDER_COMPONENT";
const size_t RenderComponent::COMPONENT_ID    = HashName(RenderComponent::COMPONENT_NAME);
