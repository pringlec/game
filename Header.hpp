#pragma once
// Needed becuase the linker is fucking stupid and cannot find main.cpp
#include <Windows.h>

// Use GLEW for access to OpenGL 
#define GLEW_STATIC
#include <GL/glew.h>

// SDL for windowing and input capture
#include <SDL.h>
#include <SDL_opengl.h>

// GLM for mathematics
#include "glm\glm\glm.hpp"
#include "glm\glm\gtc\matrix_transform.hpp"

#include <vector>
#include <string>
#include <map>

#include <functional>

#include "Shader.hpp"

struct Material {
	glm::vec4 ambientReflectivity;
	glm::vec4 diffuseReflectivity;
	glm::vec4 specularRelectivity;
	float shininess;
};

struct Mesh {
	void load(std::string meshSource);
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;
	std::vector<GLushort> elements;
	Material material;
};

struct Light {
	glm::vec4 position;
	glm::vec4 diffuseColour;
	glm::vec4 ambientColour;
	glm::vec4 specularColour;
};

class GameObject {
public:
	GameObject(std::string name, std::string meshSource)
		: mName(name), mMeshSource(meshSource), mIsVisible(true) {};

	void setMeshSource(std::string meshSource) { mMeshSource = meshSource; }
	void loadObject();
	void render();
	bool isVisible() { return mIsVisible; }

	void setMaterial(glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, float shininess)
	{
		mMesh.material.ambientReflectivity = ambient;
		mMesh.material.diffuseReflectivity = diffuse;
		mMesh.material.specularRelectivity = specular;
		mMesh.material.shininess = shininess;
	}

	void setAmbientReflectivity(glm::vec4 ambientReflectivity) { mMesh.material.ambientReflectivity = ambientReflectivity; }
	glm::vec4 getAmbientReflectivity() { return mMesh.material.ambientReflectivity; }
	void setdiffusiveReflectivity(glm::vec4 diffusiveReflectivity) { mMesh.material.diffuseReflectivity = diffusiveReflectivity; }
	glm::vec4 getDiffusiveReflectivity() { return mMesh.material.diffuseReflectivity; }
	void setspecularReflectivity(glm::vec4 specularReflectivity) { mMesh.material.specularRelectivity = specularReflectivity; }
	glm::vec4 getSpecularReflectivity() { return mMesh.material.specularRelectivity; }
	void setShininess(float shininess) { mMesh.material.shininess = shininess; }
	float getShininess() { return mMesh.material.shininess; }

	void setBuffers(GLuint vertexBufferID, GLuint normalBufferID, GLuint elementBufferID)
	{
		mVertexBufferID = vertexBufferID; mNormalBufferID = normalBufferID; mElementBufferID = elementBufferID;
	}

	glm::mat4 getModelTransform() { return mModelTransform; }
	void setModelTransform(glm::mat4 tm) { mModelTransform = tm; }

	GLuint getVertexBufferID() { return mVertexBufferID; }
	GLuint getNormalBufferID() { return mNormalBufferID; }
	GLuint getElementBufferID() { return mElementBufferID; }

	void move(glm::vec3 d);
private:
	std::string mName;
	bool mIsVisible;
	std::string mMeshSource;

	glm::mat4 mModelTransform;

	Mesh mMesh;
	GLuint mVertexBufferID, mNormalBufferID, mElementBufferID;
};

class KeyHandler {
public:
	KeyHandler(std::function<void(void)> f) : action(f) {}
	void operator() () { action(); }
	void run() { action(); }
private:
	std::function<void(void)> action;
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

	virtual void update(SDL_Keycode aKey);
	virtual void render();

	virtual void setCurrentTarget(GameObject obj) {
		mCurrentTarget = &obj;
	}

	virtual GameObject* getCurrentTarget() { return mCurrentTarget; }

	std::vector<GameObject> mGameWorld;

	SDL_Window* window;
	SDL_GLContext context;

	int screenWidth, screenHeight;

	// Model, view and projection matrix uniform locations
	GLuint ModelTransformLoc, ViewTransformLoc, ProjectionTransformLoc;

	glm::mat4 view;
	glm::mat4 projection;

	Light theLight;

	//GLuint shaderProgram;
	ShaderProgram * defaultShader;


private:
	std::string configFile;

	std::map<std::string, std::string> keyBindings;
	//	std::map<std::string, SDL_Keycode> keyCode;
	std::map<std::string, KeyHandler*> commandHandler;

	GameObject* mCurrentTarget;
	float speed;

};