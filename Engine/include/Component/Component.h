#pragma once

#include "Core.h"
#include "Entity/Entity.h"

enum COMPONENT_TYPE
{
	NONE,
};

class APP_API Component
{
public:
	Component(Entity* ownerEntity) : OwnerEntity(ownerEntity), ComponentType(NONE) {}
	virtual ~Component() {}

	virtual void Update(float deltatime) {}
	virtual void Draw() {}

	COMPONENT_TYPE GetComponentType() { return ComponentType; }
	Entity* GetOwnerEntity() { return OwnerEntity; }

protected:
	COMPONENT_TYPE ComponentType;
	Entity* OwnerEntity;
};