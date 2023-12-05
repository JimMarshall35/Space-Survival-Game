#pragma once

#include "Objects/Object.h"

class TransformComponent;
enum COMPONENT_TYPE
{
	TRANSFORM,
	MESH,
	MESHRENDERER,
};

//////////////////////////////////////////////////

class Component
{
public:
	Component(Object* _owner);
	~Component();

	virtual void Update(const float& _deltaTime) = 0;
	virtual void Render() = 0;

	COMPONENT_TYPE GetComponentType();
	Object* GetOwner();
	inline TransformComponent* GetTransform() { return Owner->GetTransform(); }

	inline const bool& IsActive() { return Active; }
	inline void SetActive(const bool& _active) { Active = _active; }

protected:
	COMPONENT_TYPE Type;
	Object* Owner;

	bool Active = true;
};