#include "Header.hpp"

// iostream to access cout
#include <iostream>

#include <fstream>
#include <sstream>

#include "Libs\glm-0.9.8.4\glm\glm\gtc\type_ptr.hpp"

using namespace std;

// *********** Mesh ***********

void Mesh::load(string meshSource)
{
	ifstream in(meshSource, ios::in);
	if (!in)
	{
		cerr << "Cannot open " << meshSource << endl; exit(1);
	}

	// The processing of the OBJ file given here is not very efficient but for smallish models
	// it's not too bad, and gets the job done in a easily understandable way.
	// Also note that there is no error handling -- it is assumed that the OBJ file is well formed
	// and that all vertex lines contain only three numbers (ie the mesh as been triangulated).

	// An OBJ file will typically have a MTL (for materials file) associated with it.
	// If we find one we will store it's location in the following variable then process the MTL file
	// after the OBJ file.
	string mtlFile = "";

	string line;
	while (getline(in, line))
	{   // See: http://www.cplusplus.com/reference/string/string/substr/
		if (line.substr(0, 2) == "v ") // process vertex lines
		{
			istringstream s(line.substr(2));
			glm::vec4 v;
			s >> v.x;
			s >> v.y;
			s >> v.z;
			v.w = 1.0f;  // Complete the vec4.
			vertices.push_back(v);
		}
		else if (line.substr(0, 2) == "f ") // process face lines
		{
			// The original of this clause assumed that face lines were of the form
			// f 1 4 9
			// ie an 'f' followed by 3 integers.
			// We want to be able to process face lines of the form
			// f 1//3 4//2 9//3
			// (which includes normal indices) or
			// f 1/2/3 4/7/2 9/4/6
			// (which include texture indices).

			// Approach is to read each group into a separate string and then extract the indices.

			istringstream s(line.substr(2));
			string s1, s2, s3;
			s >> s1 >> s2 >> s3;

			// For each of the substrings s1,s2,s3 we need to split '10/2/3' into '10', '2', '3', or '1//37' into '1', '','37' etc

			GLushort a, b, c;

			// The std::string method find returns the position of the first occurence of the string it is searching for (or
			// the length of the searched string if it cannot find any occurrences).
			// See: http://www.cplusplus.com/reference/string/string/find/
			// We can use the find method to split up the strings as follows

			// This code assumes that entries on face lines will be like 1/2/3 or 2//4 -- what happens if entries are not one of these types?
			int sl1pos = s1.find('/', 0);
			string i1 = s1.substr(0, sl1pos);
			int sl2pos = s1.find('/', sl1pos + 1); // Start searching for next slash one position beyond first.
			string i2 = s1.substr(sl1pos + 1, sl2pos - (sl1pos + 1));
			string i3 = s1.substr(sl2pos + 1); // Everything from second slash to end of string (this is assumed to be an integer)

											   // Currently we will only make use of the first index found 
			a = stoi(i1); // See: http://www.cplusplus.com/reference/string/stoi/

						  // The splitting code now needs to be repeated for s2 and s3.
						  // There is an obvious refactoring to make here -- move this splitting code to a separate function!

			sl1pos = s2.find('/', 0);
			i1 = s2.substr(0, sl1pos);
			sl2pos = s2.find('/', sl1pos + 1); // Start searching for next slash one position beyond first.
			i2 = s2.substr(sl1pos + 1, sl2pos - (sl1pos + 1));
			i3 = s2.substr(sl2pos + 1); // Everything from second slash to end of string (this is assumed to be an integer)

			b = stoi(i1);


			sl1pos = s3.find('/', 0);
			i1 = s3.substr(0, sl1pos);
			sl2pos = s3.find('/', sl1pos + 1); // Start searching for next slash one position beyond first.
			i2 = s3.substr(sl1pos + 1, sl2pos - (sl1pos + 1));
			i3 = s3.substr(sl2pos + 1); // Everything from second slash to end of string (this is assumed to be an integer)

			c = stoi(i1);

			a--;  // Indices in OBJ files are 1-based, whilst C++ arrays are 0-based.
			b--;
			c--;
			elements.push_back(a);
			elements.push_back(b);
			elements.push_back(c);
		}
		else if (line[0] == '#') // this is a comment line
		{
			/* ignoring this line */
		}
		else if (line.substr(0, 3) == "vn ") // will want to process normal lines (those beginning "vn ") at sometime
		{
			/* ignoring this line */
		}
		else if (line.substr(0, 7) == "mtllib ")
		{
			istringstream s(line.substr(7));
			s >> mtlFile;
			mtlFile = "assets/" + mtlFile;
			cout << "Found mtllib: " << mtlFile << endl;
		}
		else
		{
			/* ignoring this line */
		}
	}

	normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
	for (int i = 0; i < elements.size(); i += 3)
	{
		GLushort ia = elements[i];
		GLushort ib = elements[i + 1];
		GLushort ic = elements[i + 2];
		glm::vec3 normal = glm::normalize(glm::cross(
			glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
			glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
		normals[ia] = normals[ib] = normals[ic] = normal;
	}

	glm::vec4 ambient;
	glm::vec4 diffusive;
	glm::vec4 specular;
	float shininess;
	float alpha = 1.0;

	if (mtlFile != "")
	{
		// Process MTL file
		cout << "Processing MTL file." << endl;
		ifstream mtlIn(mtlFile, ios::in);
		if (!mtlIn)
		{
			cerr << "Cannot open " << mtlFile << endl;
			ambient = glm::vec4(0.329412f, 0.223529f, 0.027451f, 1.0f);
			diffusive = glm::vec4(0.780392f, 0.568627f, 0.113725f, 1.0f);
			specular = glm::vec4(0.992157f, 0.941176f, 0.807843f, 1.0f);
			shininess = 27.8;
		}
		else
		{
			string mtlLine;
			while (getline(mtlIn, mtlLine))
			{
				cout << mtlLine << endl;
				if (mtlLine.substr(0, 3) == "Ka ")
				{
					// Process ambient reflectivity settings
					istringstream s(mtlLine.substr(3));
					s >> ambient.r;
					s >> ambient.g;
					s >> ambient.b;
					ambient.a = alpha;  // Complete the vec4.
				}
				else if (mtlLine.substr(0, 3) == "Kd ")
				{
					// Process diffusive reflectivity settings
					istringstream s(mtlLine.substr(3));
					s >> diffusive.r;
					s >> diffusive.g;
					s >> diffusive.b;
					diffusive.a = alpha;  // Complete the vec4.
				}
				else if (mtlLine.substr(0, 3) == "Ks ")
				{
					// Process specular reflectivity settings
					istringstream s(mtlLine.substr(3));
					s >> specular.r;
					s >> specular.g;
					s >> specular.b;
					specular.a = alpha;  // Complete the vec4.
				}
				else if (mtlLine.substr(0, 3) == "Ns ")
				{
					// Process shininess setting
					istringstream s(mtlLine.substr(3));
					s >> shininess;
				}
				else if (mtlLine.substr(0, 2) == "d ")
				{
					// Process transparency setting
					istringstream s(mtlLine.substr(2));
					s >> alpha;
				}
				else
				{
					// Ignore this line at the moment
				}
			}

			ambient.a = alpha;
			diffusive.a = alpha;
			specular.a = alpha;
		}
	}
	else
	{
		ambient = glm::vec4(0.329412f, 0.223529f, 0.027451f, 1.0f);
		diffusive = glm::vec4(0.780392f, 0.568627f, 0.113725f, 1.0f);
		specular = glm::vec4(0.992157f, 0.941176f, 0.807843f, 1.0f);
		shininess = 27.8;
	}

	material.ambientReflectivity = ambient;
	material.diffuseReflectivity = diffusive;
	material.specularRelectivity = specular;
	material.shininess = shininess;
}

// ************* KeyHandler *********************


// ************* GameObject *********************

void GameObject::loadObject()
{
	// get mesh
	mMesh.load(mMeshSource);

	// set up buffers to hold mesh data
	// Error handling missing
	glGenBuffers(1, &mVertexBufferID);
	glGenBuffers(1, &mNormalBufferID);
	glGenBuffers(1, &mElementBufferID);

	glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferID);
	int dataSize = mMesh.vertices.size() * sizeof(mMesh.vertices[0]);
	glBufferData(GL_ARRAY_BUFFER, dataSize, mMesh.vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, mNormalBufferID);
	int ndataSize = mMesh.normals.size() * sizeof(mMesh.normals[0]);
	glBufferData(GL_ARRAY_BUFFER, ndataSize, mMesh.normals.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferID);
	int indicesSize = mMesh.elements.size() * sizeof(mMesh.elements[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, mMesh.elements.data(), GL_STATIC_DRAW);
}

void GameObject::render()
{
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, mNormalBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBufferID);
	glDrawElements(GL_TRIANGLES, mMesh.elements.size(), GL_UNSIGNED_SHORT, 0);
}

void GameObject::move(glm::vec3 d)
{
	// d is a delta so move moves object from p to p+d
	glm::mat4 transMat = glm::translate(glm::mat4(), d);
	mModelTransform = transMat * mModelTransform;
}

// ************* Game ***************

void Game::initialise()
{
	// Initialise SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	screenWidth = 500;
	screenHeight = 500;

	// Set up an OpenGL window and context
	std::cout << "About to create OpenGL window and context\n" << std::endl;
	window = SDL_CreateWindow("OpenGL", 1000, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	context = SDL_GL_CreateContext(window);

	// Initialise GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		std::cout << "GLEW Error" << glewGetString(GLEW_VERSION) << std::endl;
	}

	// Starting speed
	speed = 0.05f;

	// Default config file
	// Should allow this to be set from the command line
	configFile = "assets/config.txt";

	// Bind keyboard/mouse/gamepad
	bindKeyboard();

	// Read configuration file
	readConfigFile();

	// Load assets


	// Set up OpenGL buffers
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Load game objects
	mGameWorld.push_back(GameObject("Suzanne", "assets/suzanne2.obj"));
	mGameWorld.push_back(GameObject("Torus", "assets/torus.obj"));
	mGameWorld.push_back(GameObject("Grid", "assets/grid.obj"));

	for (int i = 0; i < mGameWorld.size(); ++i) {
		mGameWorld[i].loadObject();
		//should read in these values from a file
		mGameWorld[i].setMaterial(glm::vec4(0.329412f, 0.223529f, 0.027451f, 1.0f),
			glm::vec4(0.780392f, 0.568627f, 0.113725f, 1.0f),
			glm::vec4(0.992157f, 0.941176f, 0.807843f, 1.0f),
			27.8);
		mGameWorld[i].setModelTransform(glm::translate(glm::mat4(), glm::vec3(0.0, 0.0, -4.0)));
	}

	mCurrentTarget = &mGameWorld[0];

	mGameWorld[1].move(glm::vec3(1.0, 0.0, 0.0));
	// Set up a light for the world
	// Arbitrary light position and colour values
	theLight.position = glm::vec4(-5.0f, 5.0f, -4.0f, 1.0f);
	theLight.ambientColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	theLight.diffuseColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	theLight.specularColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Default view and projection matrices
	view = glm::lookAt(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));
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

}

void Game::run()
{
	SDL_Event windowEvent;
	while (true)
	{
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
		update(theKey);
		// Render the game world.
		render();

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
		if (configType == "Asset") { // Currently ignored if present.
									 //string fname;
									 //str >> fname;
									 //int lastSlash = fname.find_last_of("/\\");
									 //assetFiles.push_back(
									 //	pair<string, string>(fname.substr(0, lastSlash), fname.substr(lastSlash + 1)));
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
	KeyHandler *moveUp = new KeyHandler([this]() {mCurrentTarget->move(speed*glm::vec3(0.0f, 1.0f, 0.0f)); });
	commandHandler["up"] = moveUp;

	KeyHandler *moveDown = new KeyHandler([this]() {mCurrentTarget->move(speed*glm::vec3(0.0f, -1.0f, 0.0f)); });
	commandHandler["down"] = moveDown;

	KeyHandler *moveLeft = new KeyHandler([this]() {mCurrentTarget->move(speed*glm::vec3(-1.0f, 0.0f, 0.0f)); });
	commandHandler["left"] = moveLeft;

	KeyHandler *moveRight = new KeyHandler([this]() {mCurrentTarget->move(speed*glm::vec3(1.0f, 0.0f, 0.0f)); });
	commandHandler["right"] = moveRight;

	KeyHandler *selectObject1 = new KeyHandler([this]() {cout << "Select target 1" << endl; mCurrentTarget = &mGameWorld[0]; });
	commandHandler["selectObject1"] = selectObject1;

	KeyHandler *selectObject2 = new KeyHandler([this]() {
		cout << "Select target 2" << endl;
		mCurrentTarget = &mGameWorld[1]; });
	commandHandler["selectObject2"] = selectObject2;

	KeyHandler *selectObject3 = new KeyHandler([this]() {
		cout << "Select target 3" << endl;
		mCurrentTarget = &mGameWorld[2]; });
	commandHandler["selectObject3"] = selectObject3;

	KeyHandler *zoomOut = new KeyHandler([this]() {
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, -0.5f));
	});
	commandHandler["zoomOut"] = zoomOut;

	KeyHandler *zoomIn = new KeyHandler([this]() {
		view = glm::translate(view, glm::vec3(0.0f, 0.0f, 0.5f));
	});
	commandHandler["zoomIn"] = zoomIn;
}

void Game::update(SDL_Keycode aKey)
{
	if (aKey != SDLK_UNKNOWN) {
		string kname = SDL_GetKeyName(aKey);
		// check to see if kname is actually bound to a command
		// If it is we assume that an action has been associated with the command and run it
		if (keyBindings.find(kname) != keyBindings.end()) {
			string command = keyBindings[kname];
			(*commandHandler[command])();
			// commandHandler[command]->run();
		}
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

	for (int i = 0; i < mGameWorld.size(); ++i) {
		if (mGameWorld[i].isVisible()) {
			// Need to set up shader attributes and uniforms before rendering object.
			// Methods of defaultShader object will throw a runtime_error exception if
			// there is an error detected in the shader processing.
			// This code should therefore be wrapped in a try..catch block

			// Associate vertex shader inputs with vertex attributes

			glEnableVertexAttribArray(defaultShader->attribute("vPosition"));
			glBindBuffer(GL_ARRAY_BUFFER, mGameWorld[i].getVertexBufferID());
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(defaultShader->attribute("vNormal"));
			glBindBuffer(GL_ARRAY_BUFFER, mGameWorld[i].getNormalBufferID());
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// Set up uniforms for materials values for this object
			glm::vec4 ambientContrib = theLight.ambientColour * mGameWorld[i].getAmbientReflectivity();
			glm::vec4 diffuseContrib = theLight.diffuseColour * mGameWorld[i].getDiffusiveReflectivity();
			glm::vec4 specularContrib = theLight.specularColour * mGameWorld[i].getSpecularReflectivity();
			float shininess = mGameWorld[i].getShininess();

			glUniform4fv(defaultShader->uniform("ambientContrib"), 1, glm::value_ptr(ambientContrib));
			glUniform4fv(defaultShader->uniform("diffuseContrib"), 1, glm::value_ptr(diffuseContrib));
			glUniform4fv(defaultShader->uniform("specularContrib"), 1, glm::value_ptr(specularContrib));
			glUniform4fv(defaultShader->uniform("lightPosition"), 1, glm::value_ptr(theLight.position));
			glUniform1f(defaultShader->uniform("shininess"), shininess);

			// Object model transform
			glm::mat4 mt = mGameWorld[i].getModelTransform();
			glUniformMatrix4fv(defaultShader->uniform("M"), 1, GL_FALSE, glm::value_ptr(mt));
			mGameWorld[i].render();
		}
	}

	SDL_GL_SwapWindow(window);
}