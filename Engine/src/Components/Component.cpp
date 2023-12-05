#include "Components/Component.h"

Component::Component(Object* _owner)
	: Owner(_owner)
{}

Component::~Component()
{}

COMPONENT_TYPE Component::GetComponentType()
{
	return Type;
}

Object* Component::GetOwner()
{
	return Owner;
}
