#include "SceneObject.h"

SceneObject::SceneObject()
{
	this->localTranslation = vector<double>{0, 0, 0};
	this->localRotation = vector<double>{0, 0, 0, 0};
	this->localScale = vector<double>{1, 1, 1};
	this->children = vector<SceneObject*>();
	this->visualObjects = vector<Mesh*>();
}

SceneObject::~SceneObject()
{
}

void SceneObject::setTranslation(vector<double> translation)
{
	this->localTranslation = translation;
}

void SceneObject::setRotation(vector<double> rotation)
{
	this->localRotation = rotation;
}

void SceneObject::setScale(vector<double> scale)
{
	this->localScale = scale;
}

void SceneObject::addChild(SceneObject* child)
{
	this->children.push_back(child);
}

void SceneObject::addVisualObject(Mesh* visualObject)
{
	this->visualObjects.push_back(visualObject);
}
