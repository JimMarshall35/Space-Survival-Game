#include "Components/TransformComponent.h"

TransformComponent::TransformComponent(Object* _owner)
	: Component(_owner), Translation(glm::vec3()), Rotation(glm::identity<glm::quat>()), Scale(glm::vec3(1.f, 1.f, 1.f)), TransformationMatrix(glm::identity<glm::mat4>()), RebuildTransformationMatrix(true)
{
	Type = TRANSFORM;
}

TransformComponent::~TransformComponent()
{}

void TransformComponent::Update(const float& _deltaTime)
{}

void TransformComponent::Render()
{}

void TransformComponent::SetScale(const float& _scale)
{
	Scale = glm::vec3(_scale, _scale, _scale);
	RebuildTransformationMatrix = true;
}

void TransformComponent::SetScale(const float& _x, const float& _y, const float& _z)
{
	Scale = glm::vec3(_x, _y, _z);
	RebuildTransformationMatrix = true;
}

void TransformComponent::ScaleBy(const float& _scale)
{
	Scale *= _scale;
	RebuildTransformationMatrix = true;
}

void TransformComponent::ScaleBy(const float& _x, const float& _y, const float& _z)
{
	Scale *= glm::vec3(_x, _y, _z);
	RebuildTransformationMatrix = true;
}

void TransformComponent::SetRotation(const glm::vec3& _eulerAngles)
{
	Rotation = glm::rotate(glm::identity<glm::quat>(), glm::radians(_eulerAngles));
	RebuildTransformationMatrix = true;
}

glm::vec3 TransformComponent::GetRotation(bool _degrees/*= true*/) const
{
	return glm::degrees(glm::eulerAngles(Rotation));
}

void TransformComponent::Rotate(const float& _theta, const glm::vec3& _axis)
{
	Rotation = glm::rotate(Rotation, glm::radians(_theta), glm::normalize(_axis));
	RebuildTransformationMatrix = true;
}

void TransformComponent::RotateX(const float& _theta)
{
	Rotation = glm::rotate(Rotation, glm::radians(_theta), glm::vec3(1.0f, 0.0f, 0.0f));
	RebuildTransformationMatrix = true;
}

void TransformComponent::RotateY(const float& _theta)
{
	Rotation = glm::rotate(Rotation, glm::radians(_theta), glm::vec3(0.0f, 1.0f, 0.0f));
	RebuildTransformationMatrix = true;
}

void TransformComponent::RotateZ(const float& _theta)
{
	Rotation = glm::rotate(Rotation, glm::radians(_theta), glm::vec3(0.0f, 0.0f, 1.0f));
	RebuildTransformationMatrix = true;
}

void TransformComponent::SetPosition(const glm::vec3& _pos)
{
	SetPosition(_pos.x, _pos.y, _pos.z);
}

void TransformComponent::SetPosition(const float& _x, const float& _y, const float& _z)
{
	Translation.x = _x;
	Translation.y = _y;
	Translation.z = _z;

	RebuildTransformationMatrix = true;
}

void TransformComponent::Translate(const glm::vec3& _translation)
{
	Translate(_translation.x, _translation.y, _translation.z);
}

void TransformComponent::Translate(const float& _x, const float& _y, const float& _z)
{
	Translation.x += _x;
	Translation.y += _y;
	Translation.z += _z;

	RebuildTransformationMatrix = true;
}

glm::vec3 TransformComponent::GetForward() const
{
	return glm::rotate(Rotation, glm::vec3(0.f, 0.f, 1.f));
}

glm::vec3 TransformComponent::GetUp() const
{
	return glm::rotate(Rotation, glm::vec3(0.f, 1.f, 0.f));
}

glm::vec3 TransformComponent::GetRight() const
{
	return glm::rotate(Rotation, glm::vec3(1.f, 0.f, 0.f));
}

glm::mat4 TransformComponent::GetTranslationMatrix()
{
	RebuildTransformation();

	return TranslationMatrix;
}

glm::mat4 TransformComponent::GetRotationMatrix()
{
	RebuildTransformation();

	return RotationMatrix;
}

glm::mat4 TransformComponent::GetScaleMatrix()
{
	RebuildTransformation();

	return ScaleMatrix;
}

void TransformComponent::RebuildTransformation()
{
	if (!RebuildTransformationMatrix)
		return;

	ScaleMatrix = glm::mat4(
		Scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, Scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, Scale.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	RotationMatrix = glm::toMat4(Rotation);

	TranslationMatrix = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.f,
		0.0f, 1.0f, 0.0f, 0.f,
		0.0f, 0.0f, 1.0f, 0.f,
		Translation.x, Translation.y, Translation.z, 1.0f
	);

	TransformationMatrix = TranslationMatrix * RotationMatrix * ScaleMatrix;
	RebuildTransformationMatrix = false;
}

glm::mat4 TransformComponent::GetTransformation()
{
	RebuildTransformation();

	glm::mat4 parentTransform = glm::identity<glm::mat4>();
	if (Object* parent = GetOwner()->GetParent())
	{
		parentTransform = parent->GetTransform()->GetTransformation();
	}

	return parentTransform * TransformationMatrix;
}