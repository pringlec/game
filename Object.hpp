#pragma once
// Needed becuase the linker is fucking stupid and cannot find main.cpp
#include <Windows.h>

// Use GLEW for access to OpenGL 
#define GLEW_STATIC
#include "Libs\glew-2.0.0-win32\glew-2.0.0\include\GL\glew.h"

// SDL for windowing and input capture
#include "Libs\SDL2-2.0.5\include\SDL.h"
#undef main
#include "Libs\SDL2-2.0.5\include\SDL_opengl.h"

// GLM for mathematics
#include "Libs\glm-0.9.8.4\glm\glm\glm.hpp"
#include "Libs\glm-0.9.8.4\glm\glm\gtc\matrix_transform.hpp"

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
	std::string textureFile = "";     // Assuming at most one texture file
	std::string vertexShader = "";
	std::string fragmentShader = "";
};

struct Mesh {
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;
	std::vector<GLushort> elements;
};


// struct for bounding box info
struct AABB {
	AABB(glm::vec3 cd = glm::vec3(0.0, 0.0, 0.0), float wX = 0.0, float wY = 0.0, float wZ = 0.0)
		: centre(cd), halfWidthX(wX), halfWidthY(wY), halfWidthZ(wZ) {}
	glm::vec3 centre;
	float halfWidthX;
	float halfWidthY;
	float halfWidthZ;
};


class VisibleObject {
public:
	VisibleObject(std::string name, Mesh* mesh, Material* material)
		: mName(name), mMesh(mesh), mMaterial(material), mIsVisible(true) {
		compileShaders();
	};

	void sendObjectToGPU();
	void render();
	bool hasShaders() { return (mShader != nullptr); }
	void useShader(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& correction = glm::mat4());
	bool isVisible() { return mIsVisible; }

	void setMesh(Mesh* mesh) { mMesh = mesh; }

	void setMaterial(glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, float shininess)
	{
		mMaterial->ambientReflectivity = ambient;
		mMaterial->diffuseReflectivity = diffuse;
		mMaterial->specularRelectivity = specular;
		mMaterial->shininess = shininess;
	}

	void setTextureFile(std::string fname) { mMaterial->textureFile = fname; }
	std::string getTextureFile() { return mMaterial->textureFile; }

	std::string getName() { return mName; }

	void setAmbientReflectivity(glm::vec4 ambientReflectivity) { mMaterial->ambientReflectivity = ambientReflectivity; }
	glm::vec4 getAmbientReflectivity() { return mMaterial->ambientReflectivity; }
	void setdiffusiveReflectivity(glm::vec4 diffusiveReflectivity) { mMaterial->diffuseReflectivity = diffusiveReflectivity; }
	glm::vec4 getDiffusiveReflectivity() { return mMaterial->diffuseReflectivity; }
	void setspecularReflectivity(glm::vec4 specularReflectivity) { mMaterial->specularRelectivity = specularReflectivity; }
	glm::vec4 getSpecularReflectivity() { return mMaterial->specularRelectivity; }
	void setShininess(float shininess) { mMaterial->shininess = shininess; }
	float getShininess() { return mMaterial->shininess; }

	void setBuffers(GLuint vertexBufferID, GLuint normalBufferID, GLuint elementBufferID,
		GLuint texCoordBufferID, GLuint textureBufferID = 0)
	{
		mVertexBufferID = vertexBufferID; mNormalBufferID = normalBufferID;
		mElementBufferID = elementBufferID; mTexCoordBufferID = texCoordBufferID;
		mTextureBufferID = textureBufferID;
	}

	glm::vec3 getMeshCentroid();
	float getBoundingSphereRadius();
	AABB getBoundingBox();

	void translateMesh(glm::vec3 disp);

	glm::mat4 getModelTransform() { return mModelTransform; }
	void setModelTransform(glm::mat4 tm) { mModelTransform = tm; }


	glm::mat4 getModelMovement() { return mMovement; }
	void setModelMovement(glm::mat4 m) { mMovement = m; }

	GLuint getVertexBufferID() { return mVertexBufferID; }
	GLuint getNormalBufferID() { return mNormalBufferID; }
	GLuint getElementBufferID() { return mElementBufferID; }
	GLuint getTexCoordBufferID() { return mTexCoordBufferID; }
	GLuint getTextureBufferID() { return mTextureBufferID; }


	void move(glm::vec3 d);
private:
	std::string mName;
	bool mIsVisible;
	std::string mMeshSource;

	glm::mat4 mModelTransform;
	glm::mat4 mMovement;

	Mesh *mMesh;
	Material *mMaterial;
	ShaderProgram* mShader = nullptr;
	void compileShaders();

	GLuint mVertexBufferID, mNormalBufferID, mElementBufferID, mTexCoordBufferID, mTextureBufferID;
};
