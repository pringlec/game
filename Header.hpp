#pragma once

#if(_Win32)
#include <Windows.h>
#endif
// Use GLEW for access to OpenGL 
#define GLEW_STATIC
#include "Libs\glew-2.0.0-win32\glew-2.0.0\include\GL\glew.h"

// SDL for windowing and input capture
#include "Libs\SDL2-2.0.5\include\SDL.h"

// Needed becuase the linker is fucking stupid and cannot find main.cpp
#undef main

#include "Libs\SDL2-2.0.5\include\SDL_opengl.h"

// GLM for mathematics
#include "Libs\glm-0.9.8.4\glm\glm\glm.hpp"
#include "Libs\glm-0.9.8.4\glm\glm\gtc\matrix_transform.hpp"

#include <vector>
#include <string>
#include <map>

#include <functional>

#include "Libs\Phys\src\btBulletDynamicsCommon.h"

#include "Object.hpp"

struct Light {
	glm::vec4 position;
	glm::vec4 diffuseColour;
	glm::vec4 ambientColour;
	glm::vec4 specularColour;
};

class KeyHandler {
public:
	KeyHandler(std::function<void(float)> f) : action(f) {}
	void operator() (float d) { action(d); }
	void run(float d) { action(d); }
private:
	std::function<void(float)> action;
};

class Game
{
public:
	void initialise();
	void run();
	void shutdown();


protected:

	virtual void readConfigFile();
	virtual void bindKeyboard();

	virtual void loadAssets();

	virtual void update(SDL_Keycode aKey, float delta);
	virtual void render();

	std::vector<VisibleObject*> mVisibleWorld;

	SDL_Window* window;
	SDL_GLContext context;

	int screenWidth, screenHeight;

	// Model, view and projection matrix uniform locations
	GLuint ModelTransformLoc, ViewTransformLoc, ProjectionTransformLoc;

	glm::mat4 view;
	glm::mat4 projection;

	Light theLight;

	//GLuint shaderProgram;
	std::vector<ShaderProgram*> mShaders;
	ShaderProgram * defaultShader;

	glm::vec3 getVisibleWorldCentroid();
	void centerVisibleWorld();

private:
	// Data members for use in processing config file
	std::string configFile;
	std::vector<std::string> assetFiles;

	// Assume that mObjectNames[i]  is associated with shaders mVertexShaders[i] and mFragmentShaders[i]
	// Some objects may reuse shaders (eg the default shader) so there can be repetitions in the names
	// stored in the shader name vectors. It is assumed that object names are unique.
	std::vector<std::string> mObjectNames;
	std::vector<std::string> mVertexShaderNames;
	std::vector<std::string> mFragmentShaderNames;

	std::map<std::string, std::string> keyBindings;
	std::map<std::string, KeyHandler*> commandHandler;

	float angle = 0.1f; // amount to rotate view.
	glm::mat4 viewRotMatrix;

	// Visible world objects;
	glm::vec3 cubePos;

	// Physics stuff
	bool isPhysicsPaused = true;
	glm::mat4 phys2VisTransform;
	bool correctForBullet = true;

	btDiscreteDynamicsWorld* thePhysicsWorld;
	btRigidBody* thePhysicsCube;
	btVector3 physCubeStartPos;
	btQuaternion cubeStartOrientation;

	btRigidBody* thePhysicsRamp;
	btRigidBody* thePhysicsWall;
};