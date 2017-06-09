#include "Header.hpp"

//#include "Libs\assimp\include\assimp\Importer.hpp" // C++ importer interface
#include <assimp\Importer.hpp>
//#include "Libs\assimp\include\assimp\scene.h" // Output data structure
#include <assimp\scene.h>
//#include "Libs\assimp\include\assimp\postprocess.h" // Post processing flags
#include <assimp\postprocess.h>
// iostream to access cout
#include <iostream>
#include <fstream>
#include <sstream>

//#include "Libs\glm-0.9.8.4\glm\glm\gtc\type_ptr.hpp"
#include <gtc\type_ptr.hpp>

//#include "BulletWorldImporter\btBulletWorldImporter.h"
#include <BulletWorldImporter\btBulletWorldImporter.h>

#include <algorithm>

using namespace std;


// ************* Game ***************

void Game::initialise()
{
	// Initialise SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	screenWidth = 1000;
	screenHeight = 750;

	// Set up an OpenGL window and context
	std::cout << "About to create OpenGL window and context\n" << std::endl;
	window = SDL_CreateWindow("OpenGL", 500, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	context = SDL_GL_CreateContext(window);

	// Initialise GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		std::cout << "GLEW Error" << glewGetString(GLEW_VERSION) << std::endl;
	}

	// Default config file
	// Should allow this to be set from the command line
	configFile = "assets/config.txt";

	// Read configuration file
	readConfigFile();

	// Load assets
	loadAssets();

	// Bind keyboard/mouse/gamepad
	bindKeyboard();

	// Massage game objects if required

	// Set up OpenGL buffers for mesh objects
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Send game objects to GPU
	for (unsigned int i = 0; i < mVisibleWorld.size(); ++i) {
		mVisibleWorld[i]->sendObjectToGPU();
	}

	// Set up a light for the world
	// Arbitrary light position and colour values
	theLight.position = glm::vec4(-5.0f, 5.0f, -4.0f, 1.0f);
	theLight.ambientColour = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	theLight.diffuseColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	theLight.specularColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Default view and projection matrices
	view = glm::lookAt(glm::vec3(0.0, 10.0, 20.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));
	projection = glm::perspective(45.0f, 1.0f*screenWidth / screenHeight, 1.0f, 100.0f);

	// Default shaders

	try {
		defaultShader = new ShaderProgram();
		defaultShader->initFromFiles("assets/default.vert", "assets/default.frag");
		defaultShader->addAttribute("vPosition");
		defaultShader->addAttribute("vNormal");
		defaultShader->addUniform("P");
		defaultShader->addUniform("M");
		defaultShader->addUniform("V");
		defaultShader->addUniform("lightPosition");
		defaultShader->addUniform("ambientContrib");
		defaultShader->addUniform("diffuseContrib");
		defaultShader->addUniform("specularContrib");
		defaultShader->addUniform("shininess");

		defaultShader->use();
	}
	catch (const runtime_error& error) {
		cerr << "Error in shader processing!" << endl;
		cerr << error.what();
		// Program now continues but will abort when it hits the code to associate 
		// attribute/uniforms with appropriate program data as that code
		// has not been wrapped in a try..catch block.
		// Might be better to terminate program at this point.
	}

	glEnable(GL_DEPTH_TEST);

	// Initialise physics world
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new	btCollisionDispatcher(collisionConfiguration);
	btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
	thePhysicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache,
		solver, collisionConfiguration);
	//thePhysicsWorld->setGravity(btVector3(0, -10, 0));

	isPhysicsPaused = true;

	// Associate visible world and physics world

	btBulletWorldImporter* fileLoader = new btBulletWorldImporter(thePhysicsWorld);

	//optionally enable the verbose mode to provide debugging information during file loading (a lot of data is generated, so this option is very slow)

	fileLoader->setVerboseMode(true);
	fileLoader->loadFile("assets/arena.bullet");

	int numRigidBodies = fileLoader->getNumRigidBodies();
	for (int i = 0; i < numRigidBodies; i++) {
		btCollisionObject* coll = fileLoader->getRigidBodyByIndex(i);
		btRigidBody* body = btRigidBody::upcast(coll);
		string bodyName = fileLoader->getNameForPointer(body);
		if (bodyName == "Cube") {
			thePhysicsCube = body;
			btTransform cubeStartTransform = thePhysicsCube->getWorldTransform();

			physCubeStartPos = thePhysicsCube->getCenterOfMassPosition();
			cubeStartOrientation = thePhysicsCube->getOrientation();

			btDefaultMotionState *ms = new btDefaultMotionState(cubeStartTransform);
			thePhysicsCube->setMotionState(ms);
			//thePhysicsCube->setRestitution(0.25);
			//thePhysicsCube->setFriction(10.0);
			//thePhysicsCube->setRollingFriction(0.1);
			//thePhysicsCube->setDamping(0.04, 0.10);
		}
		if (bodyName == "Ramp") {
			thePhysicsRamp = body;
			//thePhysicsRamp->setRestitution(0.25);
			//thePhysicsRamp->setFriction(1.5);
		}
		if (bodyName == "Wall") {
			thePhysicsWall = body;
			//thePhysicsWall->setRestitution(0.85);
		}
	}

	thePhysicsWorld->setGravity(btVector3(0.0, 0.0, -9.8));

	// Physics world and OBJ file as exported from Blender has Z-up and Y-forward, need to translate to
	// OpenGL world with -Z-forward and Y-up.
	// This transform will do the trick.

	phys2VisTransform = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

}

void Game::run()
{
	SDL_Event windowEvent;
	float lastTime = SDL_GetTicks() / 1000.0f;
	while (true)
	{
		float current = SDL_GetTicks() / 1000.0f;
		float delta = current - lastTime;
		SDL_Keycode theKey = SDLK_UNKNOWN;

		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP &&
				windowEvent.key.keysym.sym == SDLK_ESCAPE) break;

			if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				std::cout << "Window resized." << std::endl;
				screenWidth = windowEvent.window.data1;
				screenHeight = windowEvent.window.data2;
				glViewport(0, 0, screenWidth, screenHeight);
			}

			if (windowEvent.type == SDL_KEYDOWN) {
				theKey = windowEvent.key.keysym.sym;
			}
		}

		// Update game in response to user input
		update(theKey, delta);

		// Render the game world.
		render();

		lastTime = current;
	}
}

void Game::shutdown()
{
	// Shutdown
	SDL_GL_DeleteContext(context);
	SDL_Quit();
}

void Game::readConfigFile()
{
	string line;
	string key;
	ifstream ifs(configFile, ifstream::in);

	while (ifs.good() && !ifs.eof() && getline(ifs, line))
	{
		string configType = "";
		stringstream str(line);
		str >> configType;
		if (configType == "#") // a comment line
			continue;
		if (configType == "Asset") { // An asset line gives the name (with path) of file containing
									 // assets (ie 3D object meshes)
			string fname;
			str >> fname;
			size_t lastSlash = fname.find_last_of("/\\");
			assetFiles.push_back(fname);
		}
		else if (configType == "Object") {
			string objName;
			string vertShader;
			string fragShader;
			str >> objName >> vertShader >> fragShader;
			mObjectNames.push_back(objName);
			mVertexShaderNames.push_back(vertShader);
			mFragmentShaderNames.push_back(fragShader);
		}
		else if (configType == "Key") {
			string key;
			string binding;
			str >> key >> binding;
			keyBindings[key] = binding;
		}
	}
}

void Game::bindKeyboard()
{
	KeyHandler *forward = new KeyHandler([this](float d) {
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, 0.5f));
	});
	commandHandler["forward"] = forward;

	KeyHandler *back = new KeyHandler([this](float d) {
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, -0.5f));
	});
	commandHandler["back"] = back;

	KeyHandler *togglePhysics = new KeyHandler([this](float d) {
		isPhysicsPaused = !isPhysicsPaused;
	});
	commandHandler["togglePhysics"] = togglePhysics;

	KeyHandler *correctBullet = new KeyHandler([this](float d) {
		correctForBullet = !correctForBullet;
	});
	commandHandler["correctBullet"] = correctBullet;

	KeyHandler *rotateLeft = new KeyHandler([this](float d) {
		view = glm::rotate(view, (-1.0f)*angle, glm::vec3(0.0f, 1.0f, 0.0f));
	});
	commandHandler["rotateLeft"] = rotateLeft;

	KeyHandler *rotateRight = new KeyHandler([this](float d) {
		view = glm::rotate(view, angle, glm::vec3(0.0f, 1.0f, 0.0f));
	});
	commandHandler["rotateRight"] = rotateRight;

}

void Game::loadAssets()
{
	// process each asset file listed in assetFiles
	Assimp::Importer importer;
	for (int i = 0; i < assetFiles.size(); ++i) {
		// Each asset file is read by an Assimp importer
		// No post processing requested -- assuming objects exported from 3D modeller as required for game
		const aiScene* scene = importer.ReadFile(assetFiles[i], 0);
		if (!scene) {
			fprintf(stderr, importer.GetErrorString());
		}

		// Each asset file can contain the meshes and material/texture descriptions for multiple objects.
		// I am assuming that asset files are Wavefront OBJ files (but Assimp can handle a great many
		// other formats)
		// The way to handle this is to process the aiNode tree -- but as OBJ files cannot describe
		// hierarchical relationships between objects this tree is just one layer deep.
		// We can thus process the scene as follows (this would not work for a scene containing
		// a hierarchy of meshes).

		aiNode* rn = scene->mRootNode;

#ifdef _DEBUG
		cout << "Mesh source: " << assetFiles[i] << endl;
		cout << "  Root node: " << rn->mName.data << endl;
		cout << "  Num children: " << rn->mNumChildren << endl;
		for (unsigned int k = 0; k < rn->mNumChildren; ++k) {
			const aiNode* cn = rn->mChildren[k];
			cout << "    C" << i << " Name: " << cn->mName.data << endl;
			cout << "       Num children: " << cn->mNumChildren << endl;
			cout << "       Num meshes: " << cn->mNumMeshes << endl;
			cout << "       Mesh index: " << cn->mMeshes[0] << endl;
			int matIndex = scene->mMeshes[cn->mMeshes[0]]->mMaterialIndex;
			cout << "       Mesh material index: " << matIndex << endl;
			aiString matname;
			scene->mMaterials[matIndex]->Get(AI_MATKEY_NAME, matname);
			cout << "       Material name: " << matname.data << endl;
			aiColor3D colorD(0.f, 0.f, 0.f);
			scene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, colorD);
			cout << "       Material diffusive colour: " << colorD.r << "," << colorD.g << "," << colorD.b << endl;
			aiColor3D colorA(0.f, 0.f, 0.f);
			scene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_AMBIENT, colorA);
			cout << "       Material ambient colour: " << colorA.r << "," << colorA.g << "," << colorA.b << endl;
			aiColor3D colorS(0.f, 0.f, 0.f);
			scene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_SPECULAR, colorS);
			cout << "       Material specular colour: " << colorS.r << "," << colorS.g << "," << colorS.b << endl;
			float shininess = 0.0;
			scene->mMaterials[matIndex]->Get(AI_MATKEY_SHININESS, shininess);
			// For some reason the Get function returns a value for shininess that is 4 times too large!
			cout << "       Material specular shininess: " << (shininess / 4.0) << endl;
		}
#endif

		for (unsigned int k = 0; k < rn->mNumChildren; ++k) {
			const aiNode* cn = rn->mChildren[k];
			// Construct a VisibleObject from the child.
			// I will give the VisibleObject the name of the child,
			// extract the mesh and material definitions (if they exist)
			string name = cn->mName.data;
			// Assume there is only one mesh associated with object (note that some models
			// can have more than one mesh!)
			unsigned int meshIndex = cn->mMeshes[0];
			aiMesh* theSceneMesh = scene->mMeshes[meshIndex];

			Mesh * objMesh = new Mesh();

			// Get vertices for mesh
			objMesh->vertices.reserve(theSceneMesh->mNumVertices);
			for (unsigned int j = 0; j < theSceneMesh->mNumVertices; j++) {

				aiVector3D pos = theSceneMesh->mVertices[j];
				objMesh->vertices.push_back(glm::vec4(pos.x, pos.y, pos.z, 1.0f));
			}

			// Get normals for mesh
			if (theSceneMesh->HasNormals()) {
				objMesh->normals.reserve(theSceneMesh->mNumVertices);
				for (unsigned int j = 0; j < theSceneMesh->mNumVertices; j++) {
					aiVector3D n = theSceneMesh->mNormals[j];
					objMesh->normals.push_back(glm::vec3(n.x, n.y, n.z));
				}
			}

			// Fill face indices
			if (theSceneMesh->HasFaces()) {
				objMesh->elements.reserve(3 * theSceneMesh->mNumFaces);
				for (unsigned int j = 0; j < theSceneMesh->mNumFaces; j++) {
					// Assume the model has only triangles.
					objMesh->elements.push_back(theSceneMesh->mFaces[j].mIndices[0]);
					objMesh->elements.push_back(theSceneMesh->mFaces[j].mIndices[1]);
					objMesh->elements.push_back(theSceneMesh->mFaces[j].mIndices[2]);
				}
			}

			// Fill vertices texture coordinates
			if (theSceneMesh->mTextureCoords[0] != NULL) {
				objMesh->texcoords.reserve(theSceneMesh->mNumVertices);
				for (unsigned int j = 0; j < theSceneMesh->mNumVertices; j++) {
					aiVector3D  UVW = theSceneMesh->mTextureCoords[0][j]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
					objMesh->texcoords.push_back(glm::vec2(UVW.x, UVW.y));
				}
			}

			Material* objMaterial = new Material();

			// Now get material associated with mesh
			int matIndex = scene->mMeshes[meshIndex]->mMaterialIndex;

			aiColor3D colorA(0.f, 0.f, 0.f);
			scene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_AMBIENT, colorA);
			objMaterial->ambientReflectivity = glm::vec4(colorA.r, colorA.g, colorA.b, 1.0f);

			aiColor3D colorD(0.f, 0.f, 0.f);
			scene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, colorD);
			objMaterial->diffuseReflectivity = glm::vec4(colorD.r, colorD.g, colorD.b, 1.0f);

			aiColor3D colorS(0.f, 0.f, 0.f);
			scene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_SPECULAR, colorS);
			objMaterial->specularRelectivity = glm::vec4(colorS.r, colorS.g, colorS.b, 1.0f);
			float shininess = 0.0;
			scene->mMaterials[matIndex]->Get(AI_MATKEY_SHININESS, shininess);
			// For some reason the Get function returns a value for shininess that is 4 times too large!
			objMaterial->shininess = shininess / 4.0f;

			// get texture file if any

			if (scene->mMaterials[matIndex]->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
				aiString texName;
				scene->mMaterials[matIndex]->GetTexture(aiTextureType_DIFFUSE, i, &texName);
				// Assume texture file lies in the same place as OBJ file
				size_t pos = assetFiles[i].find_last_of("/");
				string texFileName = texName.C_Str();
				if (pos != string::npos) {
					string path = assetFiles[i].substr(0, pos);
					texFileName = path + "/" + texFileName;
				}
				objMaterial->textureFile = texFileName;
			}

			// Note shaders to use (empty string -- use default shader)
			// Check that name of object is in the list of object names 
			// if so set vertex and fragment shader names from the stored names
			// Otherwise these are left as initialised to the empty string .
			auto it = find(mObjectNames.begin(), mObjectNames.end(), name);
			if (it != mObjectNames.end()) {
				auto index = distance(mObjectNames.begin(), it);
				objMaterial->vertexShader = mVertexShaderNames[index];
				objMaterial->fragmentShader = mFragmentShaderNames[index];
			}


			// Finally add object to visible world
			mVisibleWorld.push_back(new VisibleObject(name, objMesh, objMaterial));
		}
	}
}

void Game::update(SDL_Keycode aKey, float delta)
{
	if (aKey != SDLK_UNKNOWN) {
		string kname = SDL_GetKeyName(aKey);
		// check to see if kname is actually bound to a command
		// If it is we assume that an action has been associated with the command and run it
		if (keyBindings.find(kname) != keyBindings.end()) {
			string command = keyBindings[kname];
			(*commandHandler[command])(delta);
			// commandHandler[command]->run(delta);
		}
	}

	// Update physics world
	if (!isPhysicsPaused) {
		thePhysicsWorld->stepSimulation(delta, 10);
	}
}

void Game::render()
{
	// Display model
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Associate view matrix with shader uniform V
	glUniformMatrix4fv(defaultShader->uniform("V"), 1, GL_FALSE, glm::value_ptr(view));
	// Associate projection matrix with shader uniform P
	glUniformMatrix4fv(defaultShader->uniform("P"), 1, GL_FALSE, glm::value_ptr(projection));

	for (int i = 0; i < mVisibleWorld.size(); ++i) {
		if (mVisibleWorld[i]->isVisible()) {
			// Only the Cube moves in this demo
			if (mVisibleWorld[i]->getName() == "Cube") {
				btTransform cubeTransform;
				thePhysicsCube->getMotionState()->getWorldTransform(cubeTransform);
				btVector3 physCubePos = cubeTransform.getOrigin();
				// Need info about rotation/orientation of cube as well
				btQuaternion cubeOrientation = cubeTransform.getRotation();
				// Find quaternion to take cube to new orientation
				btQuaternion transQuat = cubeOrientation * cubeStartOrientation.inverse();
				btVector3 rotAxis = transQuat.getAxis();
				btScalar rot = transQuat.getAngle();

				// Translate 3D cube to origin           
				// Apply new orientation
				// Translate to new position
				// This is done in reverse order of course

				glm::mat4 cubeTrans;
				cubeTrans = glm::translate(glm::mat4(), glm::vec3(physCubePos.getX(), physCubePos.getY(), physCubePos.getZ()));
				cubeTrans = glm::rotate(cubeTrans, rot, glm::vec3(rotAxis.getX(), rotAxis.getY(), rotAxis.getZ()));
				cubeTrans = glm::translate(cubeTrans, glm::vec3((-1.0f)*physCubeStartPos.getX(), (-1.0f)*physCubeStartPos.getY(), (-1.0f)*physCubeStartPos.getZ()));
				mVisibleWorld[i]->setModelMovement(cubeTrans);

			}

			if (!mVisibleWorld[i]->hasShaders()) {

				// Use default shaders
				defaultShader->use();

				// Need to set up shader attributes and uniforms before rendering object.
				// Methods of defaultShader object will throw a runtime_error exception if
				// there is an error detected in the shader processing.
				// This code should therefore be wrapped in a try..catch block

				// Associate vertex shader inputs with vertex attributes

				glEnableVertexAttribArray(defaultShader->attribute("vPosition"));
				glBindBuffer(GL_ARRAY_BUFFER, mVisibleWorld[i]->getVertexBufferID());
				glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

				glEnableVertexAttribArray(defaultShader->attribute("vNormal"));
				glBindBuffer(GL_ARRAY_BUFFER, mVisibleWorld[i]->getNormalBufferID());
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

				// Set up uniforms for materials values for this object
				glm::vec4 ambientContrib = theLight.ambientColour * mVisibleWorld[i]->getAmbientReflectivity();
				glm::vec4 diffuseContrib = theLight.diffuseColour * mVisibleWorld[i]->getDiffusiveReflectivity();
				glm::vec4 specularContrib = theLight.specularColour * mVisibleWorld[i]->getSpecularReflectivity();
				float shininess = mVisibleWorld[i]->getShininess();

				glUniform4fv(defaultShader->uniform("ambientContrib"), 1, glm::value_ptr(ambientContrib));
				glUniform4fv(defaultShader->uniform("diffuseContrib"), 1, glm::value_ptr(diffuseContrib));
				glUniform4fv(defaultShader->uniform("specularContrib"), 1, glm::value_ptr(specularContrib));
				glUniform4fv(defaultShader->uniform("lightPosition"), 1, glm::value_ptr(theLight.position));
				glUniform1f(defaultShader->uniform("shininess"), shininess);

				// Object model transform
				glm::mat4 mtb = mVisibleWorld[i]->getModelTransform();
				glm::mat4 mtm = mVisibleWorld[i]->getModelMovement();
				glm::mat4 mt = mtm * mtb;
				if (correctForBullet) mt = phys2VisTransform * mt;
				glUniformMatrix4fv(defaultShader->uniform("M"), 1, GL_FALSE, glm::value_ptr(mt));
			}
			else {
				if (correctForBullet) {
					mVisibleWorld[i]->useShader(view, projection, phys2VisTransform);
				}
				else {
					mVisibleWorld[i]->useShader(view, projection);
				}
			}

			mVisibleWorld[i]->render();
		}
	}

	SDL_GL_SwapWindow(window);
}

glm::vec3 Game::getVisibleWorldCentroid()
{
	size_t nObjects = mVisibleWorld.size();
	glm::vec3 c(0.0f, 0.0f, 0.0f);
	for (size_t i = 0; i < nObjects; ++i) {
		c += mVisibleWorld[i]->getMeshCentroid();
	}
	return c / (float)nObjects;
}

void Game::centerVisibleWorld()
{
	glm::vec3 cd = (-1.0f)*getVisibleWorldCentroid();
	size_t nObjects = mVisibleWorld.size();
	for (size_t i = 0; i < nObjects; ++i) {
		mVisibleWorld[i]->translateMesh(cd);
	}
}

