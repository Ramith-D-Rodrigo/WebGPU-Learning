#pragma once
#include <vector>
using namespace std;
class Mesh;

class SceneObject
{
public:
	SceneObject();
	~SceneObject();

	void setTranslation(vector<double> translation);
	void setRotation(vector<double> rotation);
	void setScale(vector<double> scale);
	void addChild(SceneObject* child);
	void addVisualObject(Mesh* visualObject);

private:
	vector<double> localTranslation;
	vector<double> localRotation;
	vector<double> localScale;
	vector<SceneObject*> children;
	vector<Mesh*> visualObjects;
};

