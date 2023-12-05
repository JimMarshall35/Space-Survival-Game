#include "Objects/Object.h"

#include "Components/Component.h"
#include "Components/TransformComponent.h"

unsigned int Object::sObjectIDCount = 0;
OBJECT_MAP Object::sObjectList;

Object::Object(Object* _parent /*= nullptr*/)
	: Parent(_parent)
	, bActive(true)
	, _Transform(new TransformComponent(this))
{
	ObjectID = sObjectIDCount;
	++sObjectIDCount;

	sObjectList.insert(std::pair<const unsigned int, Object*>(ObjectID, this));

	AddComponent(_Transform);
}

Object::~Object()
{}

void Object::Update(const float& _deltaTime)
{
	for (unsigned int i = 0; i < Components.size(); ++i)
	{
		Components[i]->Update(_deltaTime);
	}
}

void Object::Render()
{
	for (unsigned int i = 0; i < Components.size(); ++i)
	{
		Components[i]->Render();
	}
}

void Object::AddComponent(Component* _component)
{
	Components.push_back(_component);
}

Component* Object::FindComponentOfType(COMPONENT_TYPE _type)
{
	for (unsigned int i = 0; i < Components.size(); ++i)
	{
		if (Components[i]->GetComponentType() == _type)
			return Components[i];
	}
	return nullptr;
}

Component* Object::operator[](COMPONENT_TYPE _type)
{
	return FindComponentOfType(_type);
}

unsigned int Object::GetObjectID()
{
	return ObjectID;
}

OBJECT_MAP Object::GetObjectList()
{
	return sObjectList;
}
