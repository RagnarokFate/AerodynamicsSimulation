#pragma once

#include <vector>
#include <memory>

#include "Camera.h"
#include "MeshModel.h"
#include "Lights.h"

using namespace std;

class Scene {
public:
	Scene();

	void AddModel(const shared_ptr<MeshModel>& mesh_model);
	int GetModelCount() const;
	MeshModel& GetModel(int index) const;
	MeshModel& GetActiveModel() const;
	std::shared_ptr<MeshModel> GetActiveModelPointer() const;
	
	void AddCamera(const shared_ptr<Camera>& camera);
	int GetCameraCount() const;
	Camera& GetCamera(int index);
	Camera& Scene::GetActiveCamera() const;

	void AddLight(const shared_ptr<Light>& Light);
	int GetLightCount() const;
	Light& Scene::GetLight(int index) const;
	Light& Scene::GetActiveLight() const;


	void SetActiveCameraIndex(int index);
	int GetActiveCameraIndex() const;

	void SetActiveModelIndex(int index);
	int GetActiveModelIndex() const;

	void SetActiveLightIndex(int index);
	int GetActiveLightIndex() const;
	
private:
	vector<shared_ptr<MeshModel>> mesh_models;
	vector<shared_ptr<Camera>> cameras;
	vector<shared_ptr<Light>> lights;

public :int active_camera_index;
public: int active_model_index;
public: int active_light_index;


public:
	bool World_Axis = false;
	float World_Length = 50.0f;

	vec3 X_World = vec3(1.0f, 0.0f, 0.0f);
	vec3 Y_World = vec3(0.0f, 1.0f, 0.0f);
	vec3 Z_World = vec3(0.0f, 0.0f, 1.0f);

	int Mode = Flat_Shading;
	bool Activiate_LightSource = false;
	bool DrawRays = false;
	float RayLength = 50;

	bool Cubemap = false;
	bool Skybox = false;

};