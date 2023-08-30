#pragma once

#include "Core.h"

#include <vector>
#include <map>

class Entity;
class Component;
enum COMPONENT_TYPE;

typedef std::map<const unsigned int, Entity*> ENTITY_MAP;
typedef std::pair<const unsigned int, Entity*> ENTITY_PAIR;

class APP_API Entity
{
public:
	Entity();
	virtual ~Entity() {}

	virtual void Update(float deltaTime);
	virtual void Draw();

	void AddComponent(Component* component);
	Component* FindComponentOfType(COMPONENT_TYPE type) const;

	unsigned int GetEntityID() const { return EntityID; }
	static ENTITY_MAP GetEntityList() { return EntityList; }

private:
	unsigned int EntityID;
	std::vector<Component*> Components;

	static unsigned int EntityIDCount;
	static ENTITY_MAP EntityList;
};