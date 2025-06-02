#define _USE_MATH_DEFINES
#include <cmath>
#include <imgui/imgui.h>
#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Renderer.h"
#include "Scene.h"
#include "Utils.h"
#include "AerodynamicsSystem.h"
//static int slected_item_frame = 0;
/**
 * Fields
 */
bool show_demo_window = false;
bool show_another_window = false;

glm::vec4 clear_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.00f);
static int Model_Number = 0;
static int view_width = 0;
static int view_height = 0;

static vec2 PreviousPosition = vec2(0,0);
static vec2 CurrentPosition = vec2(0,0);

/**
 * Aerodynamics System
 */
static std::shared_ptr<AerodynamicsSystem> aerodynamicsSystem = nullptr;
static bool showAerodynamicsWindow = false;
static float lastFrameTime = 0.0f;

/**
 * Function declarations
 */
static void GlfwErrorCallback(int error, const char* description);
GLFWwindow* SetupGlfwWindow(int w, int h, const char* window_name);
ImGuiIO& SetupDearImgui(GLFWwindow* window);
void StartFrame();
void RenderFrame(GLFWwindow* window, Scene& scene, Renderer& renderer, ImGuiIO& io);
void Cleanup(GLFWwindow* window);
void DrawImguiMenus(ImGuiIO& io, Scene& scene);
void DrawAerodynamicsMenu(ImGuiIO& io, Scene& scene);
void PrintMatrix(mat4x4 input);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void InitializeAerodynamicsPostProcessing();
/**
 * Function implementation
 */
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	
	// Only handle scroll when ImGui doesn't want to capture mouse
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse) {
		// Get scene from user pointer (we'll set this in main)
		Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(window));
		if (scene && scene->GetCameraCount() > 0) {
			Camera& camera = scene->GetActiveCamera();
					// Zoom with mouse wheel (corrected direction)
		if (yoffset > 0.0) { // Scroll up - zoom in
			vec3 currentPos = camera.LocalTransformation.GetTranslateBar();
			currentPos.z -= 0.5f; // Move camera closer (zoom in)
			camera.LocalTransformation.SetTranslateBar(currentPos);
		} else if (yoffset < 0.0) { // Scroll down - zoom out
			vec3 currentPos = camera.LocalTransformation.GetTranslateBar();
			currentPos.z += 0.5f; // Move camera away (zoom out)
			camera.LocalTransformation.SetTranslateBar(currentPos);
		}
		}
	}
}

int main(int argc, char **argv)
{
	int windowWidth = 1920, windowHeight = 1080;
	view_width = windowWidth ,view_height = windowHeight;

	GLFWwindow* window = SetupGlfwWindow(windowWidth, windowHeight, "Mesh Viewer" );
	if (!window)
		return 1;

	int frameBufferWidth, frameBufferHeight;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);

	//Renderer renderer = Renderer(frameBufferWidth, frameBufferHeight);
	Renderer renderer;
	renderer.LoadShaders();

	Scene scene = Scene();
	Camera camera_main = Camera();
	std::shared_ptr<Camera> Cameras = std::make_shared<Camera>(camera_main);
	scene.AddCamera(Cameras);
	//shared_ptr<MeshModel> first_model = Utils::LoadMeshModel("C:\\Users\\user\\Desktop\\Graphics\\Data\\crate.obj");
	//shared_ptr<MeshModel> first_model = Utils::LoadMeshModel("C:\\Users\\user\\Desktop\\Graphics\\Data\\bunny.obj");
	//scene.AddModel(first_model);
	ImGuiIO& io = SetupDearImgui(window);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	
	// Set scene pointer for callbacks
	glfwSetWindowUserPointer(window, &scene);
    while (!glfwWindowShouldClose(window))
    {
		float currentTime = (float)glfwGetTime();
		float deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;

		float Application_average_mspf = 1000.0f / ImGui::GetIO().Framerate;
		float Application_frame = ImGui::GetIO().Framerate;

		//std::string fpx_line = "Application average = " + std::to_string(Application_average_mspf) + " frames per second = " + std::to_string(Application_frame);
		std::string fpx_line =  "| FPS = " + std::to_string(Application_frame);

		string title = "Mesh Viewer " + fpx_line;
		glfwSetWindowTitle(window, title.c_str());
        glfwPollEvents();
		StartFrame();
		DrawImguiMenus(io, scene);
		DrawAerodynamicsMenu(io, scene);
		
		// Update aerodynamics system
		if (aerodynamicsSystem) {
			aerodynamicsSystem->update(deltaTime);
		}
		
		RenderFrame(window, scene, renderer, io);
    }

	Cleanup(window);
    return 0;
}

static void GlfwErrorCallback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

GLFWwindow* SetupGlfwWindow(int w, int h, const char* window_name)
{
	glfwSetErrorCallback(GlfwErrorCallback);
	if (!glfwInit())
		return NULL;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	#if __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	
	GLFWwindow* window = glfwCreateWindow(w, h, window_name, NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync
						 // very importent!! initialization of glad
						 // https://stackoverflow.com/questions/48582444/imgui-with-the-glad-opengl-loader-throws-segmentation-fault-core-dumped

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	return window;
}

ImGuiIO& SetupDearImgui(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	ImGui::StyleColorsDark();
	return io;
}

void StartFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void RenderFrame(GLFWwindow* window, Scene& scene, Renderer& renderer, ImGuiIO& io)
{
	ImGui::Render();
	//int frameBufferWidth, frameBufferHeight;
	glfwMakeContextCurrent(window);
	//glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.a);
	glEnable(GL_DEPTH_TEST);
	
	// Clear the screen and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//renderer.ClearColorBuffer(clear_color);
	renderer.Render(scene);
	
	// Render aerodynamics visualization
	if (aerodynamicsSystem && scene.GetCameraCount() > 0) {
		Camera& camera = scene.GetActiveCamera();        glm::mat4 view = camera.view_transformation;
		glm::mat4 projection = camera.GetProjectionTransformation();
		aerodynamicsSystem->render(view, projection);
	}
	
	//renderer.SwapBuffers();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
	CurrentPosition = vec2(io.MousePos.x, io.MousePos.y);
	//// TODO: Handle keyboard events here
	if (io.KeysDown[65])// move with 'a'
	{
		if (scene.GetModelCount() > 0)
		{
			vec3 currVec = vec3(scene.GetActiveCamera().LocalTransformation.GetTranslateBar());
			currVec.x -= 0.01;
		    scene.GetActiveCamera().LocalTransformation.SetTranslateBar(currVec);

		}
		// D key is down
		// Use the ASCII table for more key codes (https://www.asciitable.com/)
	}
	if (io.KeysDown[68])// move with 'd'
	{
		if (scene.GetModelCount() > 0)
		{
			vec3 currVec = vec3(scene.GetActiveCamera().LocalTransformation.GetTranslateBar());
			currVec.x += 0.01;
			scene.GetActiveCamera().LocalTransformation.SetTranslateBar(currVec);
		}
		// S key is down
		// Use the ASCII table for more key codes (https://www.asciitable.com/)
	}
	if (io.KeysDown[83])// move with 's'
	{
		if (scene.GetModelCount() > 0)
		{
			vec3 currVec = vec3(scene.GetActiveCamera().LocalTransformation.GetTranslateBar());
			currVec.y -= 0.01;
			scene.GetActiveCamera().LocalTransformation.SetTranslateBar(currVec);
		}
		// W key is down
		// Use the ASCII table for more key codes (https://www.asciitable.com/)
	}
	if (io.KeysDown[87])// move with 'w'
	{
		if (scene.GetModelCount() > 0)
		{
			vec3 currVec = vec3(scene.GetActiveCamera().LocalTransformation.GetTranslateBar());
			currVec.y += 0.01;
			scene.GetActiveCamera().LocalTransformation.SetTranslateBar(currVec);
		}
		// A key is down
		// Use the ASCII table for more key codes (https://www.asciitable.com/)
	}

	if (io.KeysDown[82] || io.KeysDown[114])// move with 'r'
	{
		if (scene.GetModelCount() > 0)
		{
			scene.GetActiveCamera().LocalTransformation.ResetTransformation();
			scene.GetActiveCamera().WorldTransformation.ResetTransformation();

		}
		// A key is down
		// Use the ASCII table for more key codes (https://www.asciitable.com/)
	}
	if (io.KeysDown[88] || io.KeysDown[120])
	{
		if (scene.GetCameraCount())
		{
			vec3 Rotate_bar = scene.GetActiveCamera().LocalTransformation.GetRotateBar() + vec3(3.0f, 0.0f, 0.0f);
			scene.GetActiveCamera().LocalTransformation.SetRotateBar(Rotate_bar);
		}
	}
	if (io.KeysDown[89] || io.KeysDown[121])
	{
		if (scene.GetCameraCount())
		{
			vec3 Rotate_bar = scene.GetActiveCamera().LocalTransformation.GetRotateBar() + vec3(0.0f, 3.0f, 0.0f);
			scene.GetActiveCamera().LocalTransformation.SetRotateBar(Rotate_bar);
		}
	}
	if (io.KeysDown[90] || io.KeysDown[122]) // Z key
	{
		if (scene.GetCameraCount())
		{
			vec3 Rotate_bar = scene.GetActiveCamera().LocalTransformation.GetRotateBar() + vec3(0.0f, 0.0f, 3.0f);
			scene.GetActiveCamera().LocalTransformation.SetRotateBar(Rotate_bar);
		}
	}

	if (io.WantCaptureKeyboard)
	{
		
		//if (io.KeysDown[88] || io.KeysDown[120]) //x
		//{
		//	scene.GetActiveCamera().LocalTransformation.SetRotateBar(scene.GetActiveCamera().LocalTransformation.GetRotateBar() + vec3(3,0,0));
		//}
		//if (io.KeysDown[89] || io.KeysDown[121]) //y
		//{
		//	scene.GetActiveCamera().LocalTransformation.SetRotateBar(scene.GetActiveCamera().LocalTransformation.GetRotateBar() + vec3(0, 3, 0));
		//}
		//if (io.KeysDown[90] || io.KeysDown[122]) //z
		//{
		//	scene.GetActiveCamera().LocalTransformation.SetRotateBar(scene.GetActiveCamera().LocalTransformation.GetRotateBar() + vec3(0, 0, 3));
		//}
		
	}

	if (!io.WantCaptureMouse)
	{
		static vec2 lastMousePos = vec2(io.MousePos.x, io.MousePos.y);
		vec2 currentMousePos = vec2(io.MousePos.x, io.MousePos.y);
		vec2 mouseDelta = currentMousePos - lastMousePos;
		
		if (scene.GetCameraCount() > 0)
		{
			Camera& camera = scene.GetActiveCamera();
			
			// Left mouse button - rotate camera
			if (io.MouseDown[0])
			{
				float rotationSpeed = 0.5f;
				vec3 currentRotation = camera.LocalTransformation.GetRotateBar();
				currentRotation.y += mouseDelta.x * rotationSpeed; // Horizontal rotation
				currentRotation.x += mouseDelta.y * rotationSpeed; // Vertical rotation
				camera.LocalTransformation.SetRotateBar(currentRotation);
			}
			
			// Right mouse button - pan camera
			if (io.MouseDown[1])
			{
				float panSpeed = 0.01f;
				vec3 currentPos = camera.LocalTransformation.GetTranslateBar();
				currentPos.x += mouseDelta.x * panSpeed;
				currentPos.y -= mouseDelta.y * panSpeed; // Inverted for natural feel
				camera.LocalTransformation.SetTranslateBar(currentPos);
			}
		}
		
		lastMousePos = currentMousePos;
	}
}

void Cleanup(GLFWwindow* window)
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void DrawImguiMenus(ImGuiIO& io, Scene& scene)
{
	/**
	 * MeshViewer menu
	 */
	static bool TransformationWindow = false;
	static bool MeshModelFeatures = false;
	static bool Camera_Add = false;
	static bool Camera_Transformation = false;
	static bool Camera_Projection = false;
	static bool AddLightWindow = false;
	static bool LightSettings = false;

	// Menu Bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open Mesh Model", "CTRL+O"))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("obj;", NULL, &outPath);
				if (result == NFD_OKAY)
				{
					scene.AddModel(Utils::LoadMeshModel(outPath));
					free(outPath);
				}
				else if (result == NFD_CANCEL)
				{
				}
				else
				{
				}

			}
			if (ImGui::MenuItem("Open Texture File", "CTRL+T"))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("jpg,jpeg,png;", NULL, &outPath);
				if (result == NFD_OKAY)
				{
					scene.GetActiveModel().LoadTexture(outPath);
					scene.GetActiveModel().TriangleFill = true;
					scene.GetActiveModel().TriangleFillMode = WithTextureNoLight;
					free(outPath);
				}
				else if (result == NFD_CANCEL)
				{
				}
				else
				{
				}

			}
			ImGui::EndMenu();


		}
		if (ImGui::BeginMenu("Transformation")) {
			if (ImGui::MenuItem("Local / World")) {
				TransformationWindow = true;
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Camera"))
		{
			if (ImGui::MenuItem("Add Camera"))
			{
				Camera_Add = true;

			}
			if (ImGui::MenuItem("Projection Settings"))
			{
				Camera_Projection = true;
			}
			ImGui::EndMenu();
		}
		if (scene.GetModelCount())
		{
			if (ImGui::BeginMenu("Mesh Model")) {
				if (ImGui::MenuItem("Features"))
				{
					MeshModelFeatures = true;
				}

				ImGui::EndMenu();
			}
		}
		if (ImGui::BeginMenu("Aerodynamics"))
		{
			if (ImGui::MenuItem("Simulation Controls"))
			{
				showAerodynamicsWindow = true;
			}
			ImGui::EndMenu();
		}

		// TODO: Add more menubar items (if you want to)
		ImGui::EndMainMenuBar();
	}

	//Main Window
	{
		ImGui::Begin("Main Menu Controller");                          // Create a window called "Hello, world!" and append into it.
		if (ImGui::CollapsingHeader("Other"))
		{
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state	
			if (scene.GetLightCount())
			{
				ImGui::Checkbox("Draw Rays", &scene.DrawRays);
				if (scene.DrawRays)
					ImGui::InputFloat("Rays Length", &scene.RayLength, 25, 0, 0, 1);
			}
			ImGui::Checkbox("World Axis", &scene.World_Axis);
			if (scene.World_Axis)
			{
				ImGui::InputFloat("World Length", &scene.World_Length, 50.0f);
			}
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}
		if (ImGui::CollapsingHeader("Stats"))
		{
			static bool MeshModelWindow = false;
			static bool CameraWindow = false;

			ImGui::Text("Stats & Transformation");
			ImGui::Text("Cameras Count = %d", scene.GetCameraCount());
			ImGui::Text("Mesh Model Count = %d", scene.GetModelCount());
			ImGui::Text("Selected Camera Index = %d", scene.GetActiveCameraIndex());
			ImGui::Text("Selected Mesh Model Index = %d", scene.GetActiveModelIndex());
			ImGui::Checkbox("Mesh Model Stats", &MeshModelWindow);
			ImGui::Checkbox("Camera Stats", &CameraWindow);

			if (scene.GetModelCount() && MeshModelWindow)
			{
				ImGui::Text("Center Point");
				ImGui::NewLine();
				ImGui::Text("X = %f", scene.GetModel(Model_Number).UpdatedCenterPoint.x);
				ImGui::SameLine();
				ImGui::Text("Y = %f", scene.GetModel(Model_Number).UpdatedCenterPoint.y);
				ImGui::SameLine();
				ImGui::Text("Z = %f", scene.GetModel(Model_Number).UpdatedCenterPoint.z);
				ImGui::NewLine();

				ImGui::Text("WORLD x LOCAL Matrix:");
				PrintMatrix((scene.GetModel(Model_Number).WorldTransformation.MainMatrix) * scene.GetModel(Model_Number).MeshModelTransformation.MainMatrix);

				if (ImGui::CollapsingHeader("Local Matrices"))
				{
					ImGui::Text("Local Translation Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).MeshModelTransformation.TranslateMatrix);
					ImGui::Text("Local Rotation Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).MeshModelTransformation.RotateMatrix);
					ImGui::Text("Local Scale Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).MeshModelTransformation.ScaleMatrix);
					ImGui::Text("Local Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).MeshModelTransformation.MainMatrix);
				}
				if (ImGui::CollapsingHeader("World Matrices"))
				{
					ImGui::Text("World Translation Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).WorldTransformation.TranslateMatrix);
					ImGui::Text("World Rotation Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).WorldTransformation.RotateMatrix);
					ImGui::Text("World Scale Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).WorldTransformation.ScaleMatrix);
					ImGui::Text("World Matrix:");
					PrintMatrix(scene.GetModel(Model_Number).WorldTransformation.MainMatrix);
				}

			}

			if (scene.GetCameraCount() && CameraWindow)
			{
				Camera& CurrentCam = scene.GetActiveCamera();
				if (CurrentCam.ViewStatus == 0)
				{
					ImGui::Text("World x Local Matrix");
					PrintMatrix(CurrentCam.view_transformation);

				}
				if (CurrentCam.ViewStatus == 1)
				{
					ImGui::Text("Look At Matrix");
					PrintMatrix(CurrentCam.view_transformation);

				}
				ImGui::NewLine();
				if (CurrentCam.ProjectionStatus == 0)
				{
					ImGui::Text("Ortho");
					PrintMatrix(CurrentCam.OrthoMatrix);
					//PrintMatrix(CurrentCam.GetProjectionTransformation());

				}
				if (CurrentCam.ProjectionStatus == 1)
				{
					ImGui::Text("Perspective");
					PrintMatrix(CurrentCam.PerspectiveMatrix);
				}
				if (CurrentCam.ProjectionStatus == 2)
				{
					ImGui::Text("Perspective");
					PrintMatrix(CurrentCam.FrustumMatrix);
				}
				ImGui::NewLine();
			}

		}
		/*if (ImGui::CollapsingHeader("Shading Setup"))
		{
			if (scene.GetLightCount())
			{
				ImGui::RadioButton("Flat Shading", &scene.Mode, Flat_Shading);
				ImGui::RadioButton("Gouraud Shading", &scene.Mode, Gouraud_Shading);
				ImGui::RadioButton("Phong Shading", &scene.Mode, Phong_Shading);

				if (ImGui::Button("Activiate Light"))
				{
					scene.Activiate_LightSource = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("Deactiviate Light"))
				{
					scene.Activiate_LightSource = false;
				}
			}
			else
			{
				ImGui::Text("No Lights has been detected!");
			}
		}*/
		ImGui::End();
	}

	if (Camera_Add)
	{
		ImGui::Begin("New Camera");
		Camera camera_new = Camera();
		if (ImGui::CollapsingHeader("View Type"))
		{
			ImGui::RadioButton("Transformation", &camera_new.ViewStatus, 0);
			ImGui::RadioButton("Look at", &camera_new.ViewStatus, 1);
		}
		if (ImGui::CollapsingHeader("Projection Type"))
		{
			ImGui::RadioButton("Orthographic", &camera_new.ProjectionStatus, 0);
			ImGui::RadioButton("Perspective", &camera_new.ProjectionStatus, 1);
			ImGui::RadioButton("Frustum", &camera_new.ProjectionStatus, 2);
		}

		if (ImGui::Button("Add To Cameras"))
		{
			std::shared_ptr<Camera> new_camera = std::make_shared<Camera>(camera_new);
			scene.AddCamera(new_camera);
		}

		if (ImGui::Button("close"))
		{
			Camera_Add = false;
		}
		ImGui::End();
	}

	if (MeshModelFeatures)
	{
		ImGui::Begin("Mesh Model Features");
		if (scene.GetModelCount())
		{
			static int index = scene.GetActiveModelIndex();
			if (ImGui::InputInt("Mesh Model Index", &index, 1, 1, 1))
			{
				if (index < scene.GetModelCount() && index > -1)
					scene.SetActiveModelIndex(index);
			}
			//ImGui::InputInt("Mesh Model Number", &Model_Number, 1);
			MeshModel& CurrentMeshModel = scene.GetActiveModel();
			if (ImGui::CollapsingHeader("Color Settings"))
			{
				ImGui::ColorEdit3("Mesh Model Grid Color", (float*)&CurrentMeshModel.MeshModelGridColor);
				ImGui::ColorEdit3("Mesh Model Fill Color", (float*)&CurrentMeshModel.MeshModelFillColor);
				ImGui::NewLine();
				ImGui::ColorEdit3("Mesh Model Center Of Face Color", (float*)&CurrentMeshModel.CenterOfFaceColor);
				ImGui::ColorEdit3("Mesh Model Normals Color", (float*)&CurrentMeshModel.NormalsColor);
				ImGui::ColorEdit3("Mesh Model Bounding Box Color", (float*)&CurrentMeshModel.BoundingBoxColor);
				ImGui::ColorEdit3("World Bounding Box Color", (float*)&CurrentMeshModel.WorldBoundingBoxColor);
				ImGui::ColorEdit3("Bounding Rectangle Color", (float*)&CurrentMeshModel.FacesRectangleColor);
			}
			if (ImGui::CollapsingHeader("Draw Grid/Lines"))
			{
				ImGui::Checkbox("Mesh Model Local Axis", &CurrentMeshModel.Local_Axis);
				if (CurrentMeshModel.Local_Axis)
				{
					ImGui::InputFloat("Local Axis Length", &CurrentMeshModel.AxisLength, 50.0f);
				}
				ImGui::Checkbox("Mesh Model Grid", &CurrentMeshModel.OffGridDraw);
				ImGui::Checkbox("Mesh Model Normals", &CurrentMeshModel.NormalsDraw);
				if (CurrentMeshModel.NormalsDraw)
				{
					ImGui::InputFloat("Normals Length", &CurrentMeshModel.NormalsLength, 50.0f);

				}
				ImGui::Checkbox("Mesh Model Center Of Each Face", &CurrentMeshModel.CenterOfFaceDraw);
				if (CurrentMeshModel.CenterOfFaceDraw)
				{
					ImGui::InputFloat("Each Face's Normal Length", &CurrentMeshModel.CenterOfFaceLength, 50.0f);
				}
				ImGui::Checkbox("Mesh Model Bounding Box", &CurrentMeshModel.BoundingBoxDraw);
				ImGui::Checkbox("World Bounding Box", &CurrentMeshModel.WorldBoundingBox);

			}
			if (ImGui::CollapsingHeader("Fill Triangle Settings"))
			{
				ImGui::Checkbox("Faces Rectangles Draw", &CurrentMeshModel.DrawFacesRectangles);
				const char* listbox_items[] = { "NO MAPPING" , "NORMAL MAPPING ", "ENVIROMENT MAPPING", "TOON SHADE" };
				static int listbox_item_current = 0;

				if (ImGui::ListBox("Select Texture Mapping Type", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 4))
				{
				}

				if (listbox_item_current == 0)
				{
					CurrentMeshModel.MappingExtra = NoMapping;
				}
				if (listbox_item_current == 1)
				{
					CurrentMeshModel.MappingExtra = NormalMapping;
				}
				if (listbox_item_current == 3)
				{
					CurrentMeshModel.MappingExtra = ToonShading;
					if (scene.GetLightCount())
						ImGui::SliderFloat("Light Intensity Level", &scene.GetActiveLight().LightIntensity, 0, 4);
				}

				ImGui::Checkbox("Mesh Model Fill", &CurrentMeshModel.TriangleFill);
				if (CurrentMeshModel.MappingExtra == 0)
				{
					ImGui::RadioButton("Normal Color", &CurrentMeshModel.TriangleFillMode, 0);
					ImGui::RadioButton("Specific Color", &CurrentMeshModel.TriangleFillMode, 1);
					ImGui::RadioButton("With Light Color Only", &CurrentMeshModel.TriangleFillMode, 2);
					ImGui::RadioButton("With Texture Color Only", &CurrentMeshModel.TriangleFillMode, 3);
					if (CurrentMeshModel.TriangleFillMode == WithTextureNoLight)
					{
						const char* listbox_items[] = { "DEFAULT" , "PLANE MAPPING", "CYLINDER MAPPING", "SPHERE MAPPING" };
						static int listbox_item_current = 0;

						if (ImGui::ListBox("Select Inner Texture Mapping", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 4))
						{
						}

						if (listbox_item_current == 0)
						{
							CurrentMeshModel.MappingMode = Default;
						}
						if (listbox_item_current == 1)
						{
							CurrentMeshModel.MappingMode = PlaneMapping;
						}
						if (listbox_item_current == 2)
						{
							CurrentMeshModel.MappingMode = CylinderMapping;
						}
						if (listbox_item_current == 3)
						{
							CurrentMeshModel.MappingMode = SphereMapping;
						}

						CurrentMeshModel.SetTextureMapping();

					}
					ImGui::RadioButton("With Light Texture Color - BOTH", &CurrentMeshModel.TriangleFillMode, 4);
					if (CurrentMeshModel.TriangleFillMode == WithLightTexture)
					{
						const char* listbox_items[] = { "Default", "Planner Mapping", "Cylinder Mapping", "Sphere Mapping" };
						static int listbox_item_current = 0;

						if (ImGui::ListBox("Select type of Transformation", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 5))
						{
						}
						if (listbox_item_current == 0)
						{
							CurrentMeshModel.MappingMode = Default;
						}
						if (listbox_item_current == 1)
						{
							CurrentMeshModel.MappingMode = PlaneMapping;
						}
						if (listbox_item_current == 2)
						{
							CurrentMeshModel.MappingMode = CylinderMapping;
						}
						if (listbox_item_current == 3)
						{
							CurrentMeshModel.MappingMode = SphereMapping;
						}
						CurrentMeshModel.SetTextureMapping();
					}

				}
			}
			if (ImGui::CollapsingHeader("Lighting of Mesh Model"))
			{
				ImGui::Text("Lighting Properties");
				ImGui::ColorEdit3("Mesh Model Ambient Color", (float*)&CurrentMeshModel.AmbientColor);
				ImGui::ColorEdit3("Mesh Model Diffuse Color", (float*)&CurrentMeshModel.DiffuseColor);
				ImGui::ColorEdit3("Mesh Model Specular Color", (float*)&CurrentMeshModel.SpecularColor);
			}

			if (ImGui::Button("close"))
			{
				MeshModelFeatures = false;
			}
			ImGui::End();
		}
	}

	if (TransformationWindow)
	{
		ImGui::Begin("Transformation Window ");
		if (ImGui::CollapsingHeader("Local Transformation"))
		{
			ImGui::Text("Mesh Model Local Transformation :%s", scene.GetModel(Model_Number).GetModelName());
			if (scene.GetModelCount())
			{
				ImGui::Text("Local Translate");
				MeshModel& Mesh_Model = scene.GetActiveModel();
				vec3 TranslateBar = Mesh_Model.MeshModelTransformation.GetTranslateBar();
				ImGui::InputFloat("Local Translate x", &TranslateBar.x);
				ImGui::InputFloat("Local Translate y", &TranslateBar.y);
				ImGui::InputFloat("Local Translate z", &TranslateBar.z);
				ImGui::SliderFloat3("Translation", (float*)&TranslateBar, -10, 10);
				scene.GetModel(Model_Number).MeshModelTransformation.SetTranslateBar(TranslateBar);

				ImGui::Text("Local Scale");
				vec3 ScaleBar = Mesh_Model.MeshModelTransformation.GetScaleBar();
				ImGui::InputFloat("Local Scale x", &ScaleBar.x);
				ImGui::InputFloat("Local Scale y", &ScaleBar.y);
				ImGui::InputFloat("Local Scale z", &ScaleBar.z);
				ImGui::SliderFloat3("Scale", (float*)&ScaleBar, 0.1, 10);
				scene.GetModel(Model_Number).MeshModelTransformation.SetScaleBar(ScaleBar);

				ImGui::Text("Local Rotation");
				vec3 RotateBar = Mesh_Model.MeshModelTransformation.GetRotateBar();
				ImGui::InputFloat("Local Rotation x", &RotateBar.x);
				ImGui::InputFloat("Local Rotation y", &RotateBar.y);
				ImGui::InputFloat("Local Rotation z", &RotateBar.z);
				ImGui::SliderFloat3("Rotation", (float*)&RotateBar, -360, 360);
				scene.GetModel(Model_Number).MeshModelTransformation.SetRotateBar(RotateBar);

			}

		}
		if (ImGui::CollapsingHeader("World Transformation"))
		{

			ImGui::Text("Mesh Model world Transformation");

			if (scene.GetModelCount())
			{
				ImGui::Text("World Translate");
				static vec3 WTranslateBar = vec3(0.0f, 0.0f, 0.0f);
				ImGui::InputFloat("World Translate x", &WTranslateBar.x);
				ImGui::InputFloat("World Translate y", &WTranslateBar.y);
				ImGui::InputFloat("World Translate z", &WTranslateBar.z);
				ImGui::SliderFloat3("World Translation", (float*)&WTranslateBar, -10, 10);


				ImGui::Text("World Scale");
				static vec3 WScaleBar = vec3(1.0f, 1.0f, 1.0f);
				ImGui::InputFloat("World Scale x", &WScaleBar.x);
				ImGui::InputFloat("World Scale y", &WScaleBar.y);
				ImGui::InputFloat("World Scale z", &WScaleBar.z);
				ImGui::SliderFloat3("World Scale", (float*)&WScaleBar, 0.1, 2);

				ImGui::Text("World Rotation");
			 static vec3 WRotateBar = vec3(0.0f, 0.0f, 0.0f);
				ImGui::InputFloat("World Rotation x", &WRotateBar.x);
				ImGui::InputFloat("World Rotation y", &WRotateBar.y);
				ImGui::InputFloat("World Rotation z", &WRotateBar.z);
				ImGui::SliderFloat3("World Rotation", (float*)&WRotateBar, -360, 360);

				for (int i = 0; i < scene.GetModelCount(); i++)
				{
					MeshModel& CurrentMesh = scene.GetModel(i);
					//mat4x4 RotationX = mat4x4(1.0f), RotationY = mat4x4(1.0f), RotationZ = mat4x4(1.0f);
					/*{
					RotationX[1][1] = cos(radians(WRotateBar.x));
					RotationX[2][1] = -sin(radians(WRotateBar.x));
					RotationX[1][2] = sin(radians(WRotateBar.x));
					RotationX[2][2] = cos(radians(WRotateBar.x));

					RotationY[0][0] = cos(radians(WRotateBar.y));
					RotationY[2][0] = -sin(radians(WRotateBar.y));
					RotationY[0][2] = sin(radians(WRotateBar.y));
					RotationY[2][2] = cos(radians(WRotateBar.y));

					RotationZ[0][0] = cos(radians(WRotateBar.z));
					RotationZ[1][0] = -sin(radians(WRotateBar.z));
					RotationZ[0][1] = sin(radians(WRotateBar.z));
					RotationZ[1][1] = cos(radians(WRotateBar.z));

					scene.GetModel(i).WorldTransformation.RotateMatrix = RotationX * RotationY * RotationZ;

					scene.GetModel(i).WorldTransformation.TranslateMatrix[0][3] = WTranslateBar.x;
					scene.GetModel(i).WorldTransformation.TranslateMatrix[1][3] = WTranslateBar.y;
					scene.GetModel(i).WorldTransformation.TranslateMatrix[2][3] = WTranslateBar.z;

					scene.GetModel(i).WorldTransformation.ScaleMatrix[0][0] = WScaleBar.x;
					scene.GetModel(i).WorldTransformation.ScaleMatrix[1][1] = WScaleBar.y;
					scene.GetModel(i).WorldTransformation.ScaleMatrix[2][2] = WScaleBar.z;
				}*/
					{

						CurrentMesh.WorldTransformation.SetScaleBar(WScaleBar);
						CurrentMesh.WorldTransformation.SetTranslateBar(WTranslateBar);
						CurrentMesh.WorldTransformation.SetRotateBar(WRotateBar);

					}
					{
						/*CurrentMesh.WorldTransformation.SetScaleMatrix();
						CurrentMesh.WorldTransformation.SetTranslateMatrix();
						CurrentMesh.WorldTransformation.SetRotateMatrix();*/
						CurrentMesh.WorldTransformation.SetTransformation();


					}

					//scene.GetModel(i).WorldTransformation.MainMatrix = scene.GetModel(i).WorldTransformation.TranslateMatrix * scene.GetModel(i).WorldTransformation.ScaleMatrix * scene.GetModel(i).WorldTransformation.RotateMatrix;
				}

			}


		}

		if (ImGui::Button("Close"))
			TransformationWindow = false;

		ImGui::End();
	}

	if (Camera_Projection)
	{
		Camera& CurrentCamera = scene.GetActiveCamera();
		ImGui::Begin("Camera Projection");
		if (ImGui::CollapsingHeader("Projection Type"))
		{
			ImGui::RadioButton("Ortho", &CurrentCamera.ProjectionStatus, 0);
			ImGui::RadioButton("Perspective", &CurrentCamera.ProjectionStatus, 1);
			ImGui::RadioButton("Frustum", &CurrentCamera.ProjectionStatus, 2);



			if (CurrentCamera.ProjectionStatus == 0)
			{
				static float Main_Right = 1;
				static float Main_Left = -1;
				static float Main_Up = 1;
				static float Main_Down = -1;

				ImGui::SliderFloat("LEFT", &Main_Left, -10, 10);
				ImGui::SliderFloat("UP", &Main_Up, -10, 10);
				ImGui::SliderFloat("RIGHT", &Main_Right, -10, 10);
				ImGui::SliderFloat("DOWN", &Main_Down, -10, 10);

				CurrentCamera.SetCameraOrtho(Main_Left, Main_Up, Main_Right, Main_Down);

			}
			if (CurrentCamera.ProjectionStatus == 1)
			{
				//Perspective View
				static float Z_Near = -1;
				static float Z_Far = 1;
				static float FOV = CurrentCamera.fov;

				ImGui::SliderFloat("Z NEAR", &Z_Near, -10, 10);
				ImGui::SliderFloat("Z FAR", &Z_Far, -10, 10);
				ImGui::SliderFloat("FOV ", &FOV, -180, 180);

				CurrentCamera.fov = FOV;
				
				CurrentCamera.SetCameraPerspective(CurrentCamera.fov, (float)view_width, (float)view_height, Z_Near, Z_Far);

			}
			if (CurrentCamera.ProjectionStatus == 2)
			{
				static float Main_Right = 1;
				static float Main_Left = -1;
				static float Main_Up = 1;
				static float Main_Down = -1;
				static float Z_Near = -1;
				static float Z_Far = 1;

				ImGui::SliderFloat("LEFT", &Main_Left, -10, 10);
				ImGui::SliderFloat("UP", &Main_Up, -10, 10);
				ImGui::SliderFloat("RIGHT", &Main_Right, -10, 10);
				ImGui::SliderFloat("DOWN", &Main_Down, -10, 10);
				ImGui::SliderFloat("Z NEAR", &Z_Near, -10, 10);
				ImGui::SliderFloat("Z FAR", &Z_Far, -10, 10);


				CurrentCamera.SetCameraFrustum(Main_Left, Main_Up, Main_Right, Main_Down, Z_Near, Z_Far);
			}
		}

		if (ImGui::CollapsingHeader("Look At/Regular Transformations"))
		{
			ImGui::RadioButton("Regular Transformation", &CurrentCamera.ViewStatus, 0);
			ImGui::SameLine();
			ImGui::RadioButton("Look At ", &CurrentCamera.ViewStatus, 1);
			ImGui::NewLine();
			if (CurrentCamera.ViewStatus == 0)
			{
				static int Status = 0;
				ImGui::RadioButton("Local", &Status, 0);
				ImGui::RadioButton("World", &Status, 1);
				if (Status == 0)
				{
					ImGui::Text("Camera Local Translate");

					Camera Camera_Object = scene.GetActiveCamera();
					Camera_Object.ViewStatus = 0;
					vec3 TranslateBar = Camera_Object.LocalTransformation.GetTranslateBar();
					ImGui::InputFloat("Local Translate x", &TranslateBar.x);
					ImGui::InputFloat("Local Translate y", &TranslateBar.y);
					ImGui::InputFloat("Local Translate z", &TranslateBar.z);
					ImGui::SliderFloat3("Translation", (float*)&TranslateBar, -640, 1280);
					scene.GetActiveCamera().LocalTransformation.SetTranslateBar(TranslateBar);

					ImGui::Text("Local Scale");
					vec3 ScaleBar = Camera_Object.LocalTransformation.GetScaleBar();
					ImGui::InputFloat("Local Scale x", &ScaleBar.x);
					ImGui::InputFloat("Local Scale y", &ScaleBar.y);
					ImGui::InputFloat("Local Scale z", &ScaleBar.z);
					ImGui::SliderFloat3("Scale", (float*)&ScaleBar, 0.1, 300);
					scene.GetActiveCamera().LocalTransformation.SetScaleBar(ScaleBar);

					ImGui::Text("Local Rotation");
					vec3 RotateBar = Camera_Object.LocalTransformation.GetRotateBar();
					ImGui::InputFloat("Local Rotation x", &RotateBar.x);
					ImGui::InputFloat("Local Rotation y", &RotateBar.y);
					ImGui::InputFloat("Local Rotation z", &RotateBar.z);
					ImGui::SliderFloat3("Rotation", (float*)&RotateBar, -360, 360);
					scene.GetActiveCamera().LocalTransformation.SetRotateBar(RotateBar);
				}
				if (Status == 1)
				{
					if (scene.GetCameraCount())
					{
						ImGui::Text("World Translate");
						static vec3 WTranslateBar = vec3(0.0f);
						ImGui::InputFloat("World Translate x", &WTranslateBar.x);
						ImGui::InputFloat("World Translate y", &WTranslateBar.y);
						ImGui::InputFloat("World Translate z", &WTranslateBar.z);
						ImGui::SliderFloat3("World Translation", (float*)&WTranslateBar, -100, 100);


						ImGui::Text("World Scale");
					 static vec3 WScaleBar = vec3(1.0f);
						ImGui::InputFloat("World Scale x", &WScaleBar.x);
						ImGui::InputFloat("World Scale y", &WScaleBar.y);
						ImGui::InputFloat("World Scale z", &WScaleBar.z);
						ImGui::SliderFloat3("World Scale", (float*)&WScaleBar, 0.1, 2);

						ImGui::Text("World Rotation");
					 static vec3 WRotateBar = vec3(0.0f);
						ImGui::InputFloat("World Rotation x", &WRotateBar.x);
						ImGui::InputFloat("World Rotation y", &WRotateBar.y);
						ImGui::InputFloat("World Rotation z", &WRotateBar.z);
						ImGui::SliderFloat3("World Rotation", (float*)&WRotateBar, -360, 360);

						for (int i = 0; i < scene.GetCameraCount(); i++)
						{
							mat4x4 RotationX = mat4x4(1.0f), RotationY = mat4x4(1.0f), RotationZ = mat4x4(1.0f);

							RotationX[1][1] = cos(radians(WRotateBar.x));
							RotationX[2][1] = -sin(radians(WRotateBar.x));
							RotationX[1][2] = sin(radians(WRotateBar.x));
							RotationX[2][2] = cos(radians(WRotateBar.x));

							RotationY[0][0] = cos(radians(WRotateBar.y));
							RotationY[2][0] = -sin(radians(WRotateBar.y));
							RotationY[0][2] = sin(radians(WRotateBar.y));
							RotationY[2][2] = cos(radians(WRotateBar.y));

							RotationZ[0][0] = cos(radians(WRotateBar.z));
							RotationZ[1][0] = -sin(radians(WRotateBar.z));
							RotationZ[0][1] = sin(radians(WRotateBar.z));
							RotationZ[1][1] = cos(radians(WRotateBar.z));

							scene.GetCamera(i).WorldTransformation.RotateMatrix = RotationX * RotationY * RotationZ;
							scene.GetCamera(i).WorldTransformation.TranslateMatrix[0][3] = WTranslateBar.x;
							scene.GetCamera(i).WorldTransformation.TranslateMatrix[1][3] = WTranslateBar.y;
							scene.GetCamera(i).WorldTransformation.TranslateMatrix[2][3] = WTranslateBar.z;
							scene.GetCamera(i).WorldTransformation.ScaleMatrix[0][0] = WScaleBar.x;
							scene.GetCamera(i).WorldTransformation.ScaleMatrix[1][1] = WScaleBar.y;
							scene.GetCamera(i).WorldTransformation.ScaleMatrix[2][2] = WScaleBar.z;
							scene.GetCamera(i).WorldTransformation.MainMatrix = scene.GetCamera(i).WorldTransformation.TranslateMatrix * scene.GetCamera(i).WorldTransformation.ScaleMatrix * scene.GetCamera(i).WorldTransformation.RotateMatrix;
						}

					}
				}
			}
			if (CurrentCamera.ViewStatus == 1)
			{
				static vec3 Main_Eye = vec3(0.0f, 0.0f, 0.0f);
				static vec3 Main_AT = vec3(0.0f, 0.0f, -1.0f);
				static vec3 Main_UP = vec3(0.0f, 1.0f, 0.0f);

				ImGui::SliderFloat3("EYE", (float*)&Main_Eye, -10, 10);
				ImGui::SliderFloat3("AT", (float*)&Main_AT, -10, 10);
				ImGui::SliderFloat3("UP", (float*)&Main_UP, -1, 1);
				static float a = 0;
				ImGui::Text("Set (a,a,a) as Camera's Position :");
				ImGui::SameLine();
				if (ImGui::InputFloat("Position", &a, 50.0f))
				{
					Main_Eye = vec3(a, a, a);
				}

				ImGui::NewLine();
				CurrentCamera.eye = Main_Eye, CurrentCamera.at = Main_AT, CurrentCamera.up = Main_UP;
				CurrentCamera.SetCameraLookAt(Main_Eye, Main_AT, Main_UP);
			}

		}

		if (ImGui::Button("close"))
		{
			Camera_Projection = false;
		}

		ImGui::End();

	}

	if (AddLightWindow)
	{
		ImGui::Begin("Add Light Source");
		Light NewLight = Light();
		if (ImGui::CollapsingHeader("Transformation"))
		{
			static int Mode = 0;
			ImGui::RadioButton("Local Transformation", &Mode, 0);
			ImGui::SameLine();
			ImGui::RadioButton("World Transformation", &Mode, 1);
			ImGui::NewLine();

			static vec3 Position = vec3(0.0f,0.0f,0.0f);
			ImGui::InputFloat3("Position", (float*)&Position,0,1);
			NewLight.Light_Position = Position;

			static float Radius = 50;
			ImGui::SliderFloat("Radius", &Radius, 0.0f, 250.0f,"%.0f");
			NewLight.Light_Radius = Radius;

			if (Mode == 0)
			{
				static vec3 TranslateBar = NewLight.Local.GetTranslateBar();
				static vec3 RotateBar = NewLight.Local.GetRotateBar();

				ImGui::DragFloat3("Local Translate Bar", (float*)&TranslateBar, 1.0f, 0.0f, 0.0f);
				ImGui::DragFloat3("Local Rotate Bar", (float*)&RotateBar, 1.0f, 0.0f, 0.0f);

				NewLight.Local.SetTranslateBar(TranslateBar);
				NewLight.Local.SetRotateBar(RotateBar);

			}
			if (Mode == 1)
			{
				static vec3 WorldTranslateBar = vec3(0.0f,0.0f,0.0f);
				static vec3 WorldRotateBar = vec3(0.0f, 0.0f, 0.0f);

				ImGui::DragFloat3("World Translate Bar", (float*)&WorldTranslateBar, 1.0f, 0.0f, 0.0f);
				ImGui::DragFloat3("World Rotate Bar", (float*)&WorldRotateBar, 1.0f, 0.0f, 0.0f);

				NewLight.Local.SetTranslateBar(WorldTranslateBar);
				NewLight.Local.SetRotateBar(WorldRotateBar);
			}

		}
		// Lighting setup removed - focusing on aerodynamics visualization
		/*if (ImGui::CollapsingHeader("Shading Setup"))
		{
			static int Mode = 0;
			ImGui::RadioButton("Flat Shading", &Mode, Flat_Shading);
			ImGui::RadioButton("Gouraud Shading", &Mode, Gouraud_Shading);
			ImGui::RadioButton("Phong Shading", &Mode, Phong_Shading);

			NewLight.Mode = Mode;
		}*/

		if (ImGui::Button("ADD Light Source"))
		{
			std::shared_ptr<Light> Light_PTR = std::make_shared<Light>(NewLight);
			scene.AddLight(Light_PTR);
			AddLightWindow = false;
		}

		if (ImGui::Button("Close"))
			AddLightWindow = false;
		ImGui::End();
	}

	if (LightSettings)
	{
		ImGui::Begin("Lights Settings");
		if (scene.GetLightCount())
		{
			static int index = scene.GetActiveLightIndex();
			if (ImGui::InputInt("Current Light Index", &index, 1, 1, 1))
			{
				if(index < scene.GetLightCount() && index > -1)
					scene.SetActiveLightIndex(index);
			}
			{
				Light& LightObject = scene.GetActiveLight();

				if (ImGui::CollapsingHeader("Transformation"))
				{

					const char* listbox_items[] = { "Center Point & Radius" , "Local Transformation", "World Transformation" };
					static int listbox_item_current = -1;

					if (ImGui::ListBox("Select type of Transformation", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 3))
					{
					}
					ImGui::NewLine();


					if (listbox_item_current == 0)
					{
						static vec3 Position = LightObject.GetPosition();
						ImGui::InputFloat3("Position", (float*)&Position,0,1);
						LightObject.SetPosition(Position);

						static float RadiusLength = LightObject.GetRadiusLength();
						ImGui::SliderFloat("Radius Length", &RadiusLength, 25, 250, 0);
						LightObject.SetRadiusLength(RadiusLength);
					}
					if (listbox_item_current == 1)
					{
						ImGui::Text("Local Translate");
						vec3 TranslateBar = LightObject.Local.GetTranslateBar();
						ImGui::InputFloat("Local Translate x", &TranslateBar.x);
						ImGui::InputFloat("Local Translate y", &TranslateBar.y);
						ImGui::InputFloat("Local Translate z", &TranslateBar.z);
						ImGui::SliderFloat3("Local Translation", (float*)&TranslateBar, -640, 1280);
						LightObject.Local.SetTranslateBar(TranslateBar);

						ImGui::Text("Local Rotation");
						vec3 RotateBar = LightObject.Local.GetRotateBar();
						ImGui::InputFloat("Local Rotation x", &RotateBar.x);
						ImGui::InputFloat("Local Rotation y", &RotateBar.y);
						ImGui::InputFloat("Local Rotation z", &RotateBar.z);
						ImGui::SliderFloat3("Local Rotation", (float*)&RotateBar, -360, 360);
						LightObject.Local.SetRotateBar(RotateBar);
					}
					if (listbox_item_current == 2)
					{

						ImGui::Text("World Translate");
						vec3 WTranslateBar = LightObject.World.GetTranslateBar();
						ImGui::InputFloat("World Translate x", &WTranslateBar.x);
						ImGui::InputFloat("World Translate y", &WTranslateBar.y);
						ImGui::InputFloat("World Translate z", &WTranslateBar.z);
						ImGui::SliderFloat3("World Translation", (float*)&WTranslateBar, -640, 1280);
						LightObject.World.SetTranslateBar(WTranslateBar);

						ImGui::Text("World Rotation");
						vec3 WRotateBar = LightObject.World.GetRotateBar();
						ImGui::InputFloat("World Rotation x", &WRotateBar.x);
						ImGui::InputFloat("World Rotation y", &WRotateBar.y);
						ImGui::InputFloat("World Rotation z", &WRotateBar.z);
						ImGui::SliderFloat3("World Rotation", (float*)&WRotateBar, -360, 360);
						LightObject.World.SetRotateBar(WRotateBar);
					}
				}		
				
				// Lighting setup removed - focusing on aerodynamics visualization
			}
			
		}
		else
		{
			ImGui::Text("No Lights Has Been Detected!");
		}
		if (ImGui::Button("Close"))
		{
			LightSettings = false;
		}
		ImGui::End();
	}	

	if (show_demo_window)
	{
		ImGui::ShowDemoWindow(&show_demo_window);
	}

}

void DrawAerodynamicsMenu(ImGuiIO& io, Scene& scene)
{
	if (!showAerodynamicsWindow) {
		return;
	}

	ImGui::Begin("Aerodynamics Simulation", &showAerodynamicsWindow);

	if (!scene.GetModelCount()) {
		ImGui::Text("No mesh models loaded. Please load a model first.");
		if (ImGui::Button("Close")) {
			showAerodynamicsWindow = false;
		}
		ImGui::End();
		return;
	}

	// System Status
	if (ImGui::CollapsingHeader("System Status", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (aerodynamicsSystem) {
			auto stats = aerodynamicsSystem->getStatistics();
			ImGui::Text("Status: %s", stats.isRunning ? "Running" : "Stopped");
			ImGui::Text("Grid Resolution: %d³", stats.gridResolution);
			ImGui::Text("Simulation Time: %.2f s", stats.simulationTime);
			
			// Performance metrics
			ImGui::Separator();
			ImGui::Text("Performance:");
			ImGui::Text("Frame Rate: %.1f FPS", stats.frameRate);
			ImGui::Text("Total Voxels: %d", stats.totalVoxels);
			ImGui::Text("Active Voxels: %d (%.1f%%)", stats.activeVoxels, 
					   stats.totalVoxels > 0 ? (float)stats.activeVoxels * 100.0f / stats.totalVoxels : 0.0f);
			
			ImGui::Separator();
			ImGui::Text("Physics:");
			ImGui::Text("Average Velocity: %.3f m/s", stats.averageVelocity);
			ImGui::Text("Max Velocity: %.3f m/s", stats.maxVelocity);
			ImGui::Text("Average Pressure: %.3f Pa", stats.averagePressure);
			ImGui::Text("Drag Force: (%.3f, %.3f, %.3f) N", stats.dragForce.x, stats.dragForce.y, stats.dragForce.z);
			ImGui::Text("Lift Force: (%.3f, %.3f, %.3f) N", stats.liftForce.x, stats.liftForce.y, stats.liftForce.z);
		} else {
			ImGui::Text("System: Not Initialized");
		}
	}

	// System Controls
	if (ImGui::CollapsingHeader("System Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (!aerodynamicsSystem) {
			if (ImGui::Button("Initialize System")) {
				try {
					aerodynamicsSystem = std::make_shared<AerodynamicsSystem>();
					if (!aerodynamicsSystem->initialize(scene.GetActiveModelPointer())) {
						aerodynamicsSystem.reset();
						ImGui::OpenPopup("Error");
					} else {
						// Initialize post-processing for the newly created aerodynamics system
						InitializeAerodynamicsPostProcessing();
					}
				} catch (const std::exception& e) {
					aerodynamicsSystem.reset();
					ImGui::OpenPopup("Error");
				}
			}
		} else {
			auto stats = aerodynamicsSystem->getStatistics();
			
			if (!stats.isRunning) {
				if (ImGui::Button("Start Simulation")) {
					aerodynamicsSystem->startSimulation();
				}
			} else {
				if (ImGui::Button("Stop Simulation")) {
					aerodynamicsSystem->stopSimulation();
				}
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Reset Simulation")) {
				aerodynamicsSystem->resetSimulation();
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Reinitialize")) {
				aerodynamicsSystem.reset();
			}
		}
	}

	// Simulation Parameters
	if (aerodynamicsSystem && ImGui::CollapsingHeader("Simulation Parameters")) {
		static SimulationParameters params = aerodynamicsSystem->getParameters();
		bool parametersChanged = false;
		
		// Performance-oriented grid resolution with warnings
		ImGui::Text("Performance Settings:");
		if (ImGui::SliderInt("Grid Resolution", &params.gridResolution, 16, 64)) {
			parametersChanged = true;
		}
		
		// Color-coded performance warnings
		if (params.gridResolution <= 32) {
			ImGui::SameLine(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "(Good Performance)");
		} else if (params.gridResolution <= 48) {
			ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 1, 0, 1), "(Moderate Performance)");
		} else {
			ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0, 0, 1), "(May be Slow)");
		}
		
		// Estimate voxel count
		int estimatedVoxels = params.gridResolution * params.gridResolution * params.gridResolution;
		ImGui::Text("Estimated Voxels: ~%dk", estimatedVoxels / 1000);
		
		ImGui::Separator();
		ImGui::Text("Physics Settings:");
		if (ImGui::SliderFloat("Time Step", &params.timeStep, 0.001f, 0.1f, "%.4f")) {
			parametersChanged = true;
		}
		if (ImGui::SliderFloat("Viscosity", &params.viscosity, 0.001f, 0.1f, "%.4f")) {
			parametersChanged = true;
		}
		if (ImGui::SliderFloat("Density", &params.density, 0.1f, 10.0f, "%.2f")) {
			parametersChanged = true;
		}
		
		ImGui::Text("Wind Velocity:");
		if (ImGui::SliderFloat3("Wind (m/s)", (float*)&params.windVelocity, -50.0f, 50.0f)) {
			parametersChanged = true;
		}
		
		ImGui::Text("Gravity:");
		if (ImGui::SliderFloat3("Gravity (m/s²)", (float*)&params.gravity, -20.0f, 20.0f)) {
			parametersChanged = true;
		}
		
		if (parametersChanged) {
			aerodynamicsSystem->updateParameters(params);
		}
	}

	// Visualization Controls
	if (aerodynamicsSystem && ImGui::CollapsingHeader("Visualization")) {
		static VisualizationParameters vizParams = aerodynamicsSystem->getVisualizationParameters();
		bool vizChanged = false;
		
		if (ImGui::Checkbox("Velocity Vectors", &vizParams.showVelocityVectors)) {
			vizChanged = true;
		}
		if (vizParams.showVelocityVectors) {
			ImGui::SameLine();
			if (ImGui::SliderFloat("Scale##velocity", &vizParams.velocityScale, 0.01f, 1.0f)) {
				vizChanged = true;
			}
		}
		
		if (ImGui::Checkbox("Pressure Field", &vizParams.showPressureField)) {
			vizChanged = true;
		}
		
		if (ImGui::Checkbox("Streamlines", &vizParams.showStreamlines)) {
			vizChanged = true;
		}
		if (vizParams.showStreamlines) {
			if (ImGui::SliderInt("Count##streamlines", &vizParams.streamlineCount, 10, 200)) {
				vizChanged = true;
			}
			if (ImGui::SliderInt("Max Length##streamlines", &vizParams.maxStreamlineLength, 50, 500)) {
				vizChanged = true;
			}
		}
		
		if (ImGui::Checkbox("Particles", &vizParams.showParticles)) {
			vizChanged = true;
		}
		if (vizParams.showParticles) {
			ImGui::SameLine();
			if (ImGui::SliderInt("Count##particles", &vizParams.particleCount, 100, 5000)) {
				vizChanged = true;
			}
		}
				if (ImGui::Checkbox("Grid Wireframe", &vizParams.showGrid)) {
			vizChanged = true;
		}
		
		// Apply visualization parameter changes
		if (vizChanged) {
			aerodynamicsSystem->updateVisualizationParameters(vizParams);
		}
		
		// Post-processing controls for Gaussian blur
		if (ImGui::CollapsingHeader("Post-Processing")) {
			auto visualizer = aerodynamicsSystem->getVisualizer();
			if (visualizer) {
				ImGui::Text("Gaussian Blur Settings:");
				
				// Pressure field blur controls
				bool pressureBlurEnabled = visualizer->IsPressureBlurEnabled();
				if (ImGui::Checkbox("Pressure Field Blur", &pressureBlurEnabled)) {
					visualizer->EnablePressureBlur(pressureBlurEnabled);
				}
				if (pressureBlurEnabled) {
					ImGui::Indent();
					ImGui::Text("Smooths pressure visualization for cleaner data display");
					ImGui::Unindent();
				}
				
				// Velocity field blur controls  
				bool velocityBlurEnabled = visualizer->IsVelocityBlurEnabled();
				if (ImGui::Checkbox("Velocity Field Blur", &velocityBlurEnabled)) {
					visualizer->EnableVelocityBlur(velocityBlurEnabled);
				}
				if (velocityBlurEnabled) {
					ImGui::Indent();
					ImGui::Text("Smooths velocity vectors for enhanced flow visualization");
					ImGui::Unindent();
				}
				
				// Blur parameters (only show if at least one blur is enabled)
				if (pressureBlurEnabled || velocityBlurEnabled) {
					ImGui::Separator();
					ImGui::Text("Blur Parameters:");
					
					// Get current blur settings from the visualizer
					static float blurRadius = 1.5f;
					static float blurStrength = 0.4f;
					static int blurIterations = 2;
					
					bool blurParamsChanged = false;
					if (ImGui::SliderFloat("Blur Radius", &blurRadius, 0.1f, 5.0f, "%.2f")) {
						blurParamsChanged = true;
					}
					ImGui::SameLine(); 
					if (ImGui::Button("?##radius")) {
						ImGui::SetTooltip("Controls the spread of the blur effect. Higher values create more diffuse smoothing.");
					}
					
					if (ImGui::SliderFloat("Blur Strength", &blurStrength, 0.1f, 1.0f, "%.2f")) {
						blurParamsChanged = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("?##strength")) {
						ImGui::SetTooltip("Controls the intensity of the blur effect. Higher values create stronger smoothing.");
					}
					
					if (ImGui::SliderInt("Blur Iterations", &blurIterations, 1, 5)) {
						blurParamsChanged = true;
					}
					ImGui::SameLine();
					if (ImGui::Button("?##iterations")) {
						ImGui::SetTooltip("Number of blur passes. More iterations create smoother results but reduce performance.");
					}
					
					// Apply blur parameters if changed
					if (blurParamsChanged) {
						visualizer->SetBlurParameters(blurRadius, blurStrength, blurIterations);
					}
					
					// Performance warning for high settings
					if (blurIterations > 3 || blurRadius > 3.0f) {
						ImGui::TextColored(ImVec4(1, 1, 0, 1), "⚠ High blur settings may impact performance");
					}
					
					// Quick presets
					ImGui::Separator();
					ImGui::Text("Quick Presets:");
					if (ImGui::Button("Subtle")) {
						blurRadius = 1.0f;
						blurStrength = 0.2f;
						blurIterations = 1;
						visualizer->SetBlurParameters(blurRadius, blurStrength, blurIterations);
					}
					ImGui::SameLine();
					if (ImGui::Button("Moderate")) {
						blurRadius = 1.5f;
						blurStrength = 0.4f;
						blurIterations = 2;
						visualizer->SetBlurParameters(blurRadius, blurStrength, blurIterations);
					}
					ImGui::SameLine();
					if (ImGui::Button("Strong")) {
						blurRadius = 2.5f;
						blurStrength = 0.7f;
						blurIterations = 3;
						visualizer->SetBlurParameters(blurRadius, blurStrength, blurIterations);
					}
				}
			} else {
				ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Post-processing unavailable - visualizer not initialized");
			}
		}
	}

	// Error popup
	if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Failed to initialize aerodynamics system!");
		ImGui::Text("Make sure a valid mesh model is loaded.");
		if (ImGui::Button("OK")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (ImGui::Button("Close")) {
		showAerodynamicsWindow = false;
	}

	ImGui::End();
}

void PrintMatrix(mat4x4 input)
{
	for (int i = 0; i < 4; i++)
	{
		ImGui::Text("%f %f %f %f", input[i].x, input[i].y, input[i].z, input[i].w);
	}
	ImGui::NewLine();
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// Update global viewport dimensions
	view_width = width;
	view_height = height;
	
	// Update OpenGL viewport
	glViewport(0, 0, width, height);
	
	// Update post-processing for aerodynamics system if initialized
	if (aerodynamicsSystem) {
		auto visualizer = aerodynamicsSystem->getVisualizer();
		if (visualizer) {
			visualizer->ResizePostProcessing(width, height);
		}
	}
	
	std::cout << "Viewport resized to: " << width << "x" << height << std::endl;
}

void InitializeAerodynamicsPostProcessing()
{
	// Initialize post-processing for aerodynamics system if it exists
	if (aerodynamicsSystem && view_width > 0 && view_height > 0) {
		auto visualizer = aerodynamicsSystem->getVisualizer();
		if (visualizer) {
			visualizer->InitializePostProcessing(view_width, view_height);
			std::cout << "Aerodynamics post-processing initialized with viewport: " 
			          << view_width << "x" << view_height << std::endl;
		}
	}
}