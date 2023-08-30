#include "Entity/Entity.h"

#include "Component/Component.h"

#include <algorithm>
#include <execution>

unsigned int Entity::EntityIDCount = 0;
ENTITY_MAP Entity::EntityList;

#define MULTITHREADED

Entity::Entity()
{
	EntityID = EntityIDCount;
	++EntityIDCount;

	EntityList.insert(ENTITY_PAIR(EntityID, this));
}

void Entity::Update(float deltaTime)
{
#ifdef MULTITHREADED
	std::for_each(std::execution::par, Components.begin(), Components.end(),
		[deltaTime](Component* component)
		{
			if (component)
			{
				component->Update(deltaTime);
			}
		}
	);
#else
	for (unsigned int i = 0; i < Components.size(); ++i)
	{
		if (Components[i])
		{
			Components[i]->Update(deltaTime);
		}
	}
#endif
}

void Entity::Draw()
{
#ifdef MULTITHREADED
	std::for_each(std::execution::par, Components.begin(), Components.end(),
		[](Component* component)
		{
			if (component)
			{
				component->Draw();
			}
		}
	);
#else
	for (unsigned int i = 0; i < Components.size(); ++i)
	{
		if (Components[i])
		{
			Components[i]->Draw(programID, vbo, ibo);
		}
	}
#endif
}

void Entity::AddComponent(Component* component)
{
	Components.push_back(component);
}

Component* Entity::FindComponentOfType(COMPONENT_TYPE type) const
{
#ifdef MULTITHREADED
	std::for_each(std::execution::par, Components.begin(), Components.end(),
		[type](Component* component)
		{
			if (component)
			{
				if (component->GetComponentType() == type)
				{
					return component;
				}
			}
		}
	);
#else
	for (unsigned int i = 0; i < Components.size(); ++i)
	{
		return Components.at(i);
	}
#endif

	return nullptr;
}