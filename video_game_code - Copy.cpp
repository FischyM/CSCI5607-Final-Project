// Mathew Fischbach, CSCI 5607
// Code adapted from Stephen Guy


const char* INSTRUCTIONS = "Press W and S to move forward and backwards\n\
Press A and D to rotate left and right.\n\
Find the teapot to win!\n\
You may find doors blocking your way. Find the key to unlock these doors.\n\
Press E to drop a key on an open floor\n\
Press F to change between fullscreen and windowed.\n\
Press Esc to exit the game.\n";

//Mac OS build: g++ video_game_code.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL2 -framework OpenGL -o FinalProject
//Linux build:  g++ video_game_code.cpp -x c glad/glad.c -g -lSDL2 -lSDL2main -lGL -ldl -I/usr/include/SDL2/ -o FinalProject

// For Visual Studios
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "glad/glad.h"  // Include order can matter here
#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#include <cstdio>

#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <map>
#include <vector>
#include <algorithm>
using namespace std;

int screenWidth = 800;
int screenHeight = 600;
float timePast = 0;
float dt = 0;
bool goal_found = false;
float char_radius = 0.125;

// srand(time(NULL));
float rand01() {
	return rand() / (float)RAND_MAX;
}

// door:key colors
glm::vec3 colorA(0.0f, 1.0f, 0.2f);
glm::vec3 colorB(1.0f, 0.5f, 0.3f);
glm::vec3 colorC(0.0f, 0.2f, 1.0f);
glm::vec3 colorD(0.8f, 0.2f, 0.8f);
glm::vec3 colorE(0.1f, 0.1f, 0.0f);

// set up camera attributes
glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, 0.0f);  // Cam Position
glm::vec3 cam_dir = glm::vec3(-1.0f, 0.0f, -1.0f);  // Look at point
glm::vec3 cam_up = glm::vec3(0.0f, 1.0f, 0.0f);  // Up
float cam_angle = glm::atan(cam_dir.z, cam_dir.x);
float cam_speed = 2.0f;

char activeKey = '0';

bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;

struct Material {
	char newmtl[20];
	float Ns;  // phong specularity exponent
	glm::vec3 Ka;  // ambient light
	glm::vec3 Kd;  // diffuse light
	glm::vec3 Ks;  // specular light
	glm::vec3 Ke;  // emissive light
	//float Ni;  // index of refraction
	//float d;  // 1.0 for "d" is the default and means fully opaque, as does a value of 0.0 for "Tr"
	//float illum;  // illumination model type, maybe need this?
	void debug() {
		printf("Material\nName: %s\nKa: (%f,%f,%f)\nKd: (%f,%f,%f)\nKs: (%f,%f,%f)\nKe: (%f,%f,%f)\nNs: %f\n",
			newmtl, Ka.x, Ka.y, Ka.z, Kd.x, Kd.y, Kd.z, Ks.x, Ks.y, Ks.z, Ke.x, Ke.y, Ke.z, Ns);
	}
};

struct ShaderMaterial {
	float Ns;  // phong specularity exponent
	glm::vec3 Ka;  // ambient light
	glm::vec3 Kd;  // diffuse light
	glm::vec3 Ks;  // specular light
	glm::vec3 Ke;  // emissive light
	ShaderMaterial(Material mat) {
		Ns = mat.Ns;
		Ka = mat.Ka;
		Kd = mat.Kd;
		Ks = mat.Ks;
		Ke = mat.Ke;
	}
	ShaderMaterial();
};

vector<Material> mat_list;

struct PointLights {
	glm::vec3 pos[36];
	glm::vec3 color[36];
	int size;
	void debug() {
		printf("total point lights: %d\n", size);
		for (int i = 0; i < size; i++) {
			printf("Point Light %d, pos: (%f,%f,%f), color: (%f,%f,%f)\n",
				i + 1, pos[i].x, pos[i].y, pos[i].z, color[i].x, color[i].y, color[i].z);
		}
	}
};

PointLights point_lights;

struct MapFile{
	int width = 0;
	int height = 0;
	char* data = NULL;
};

map <char, bool> doorOpen =  { {'A',false}, {'B',false}, {'C',false}, {'D',false}, {'E',false} };
map <char, char> doorToKey = { {'A','a'},   {'B','b'},   {'C','c'},   {'D','d'},   {'E','e'} };

void setCamDirFromAngle(float cam_angle) {
	cam_dir.z = glm::sin(cam_angle);
	cam_dir.x = glm::cos(cam_angle);
}

bool isDoor(char type) {
	if (type == 'A' || type == 'B' || type == 'C' || type == 'D' || type == 'E')
		return true;
	else
		return false;
}

bool isKey(char type) {
	if (type == 'a' || type == 'b' || type == 'c' || type == 'd' || type == 'e')
		return true;
	else
		return false;
}

bool isWall(char type) {
	return type == 'W';
}

void drawGeometry(int shaderProgram, vector<int> modelNumVerts, vector<int> modelStarts, MapFile map_data);

static char* readShaderSource(const char* shaderFile);

void loadMapFile(const char* file_name, MapFile& map_data);

float* loadModelOBJwithMTL(const char* file_name, int& numLines, const char* file_nameMTL, vector<Material>& mat_list);

void dropKey(float vx, float vz, MapFile map_data);

bool isWalkableAndEvents(float newX, float newZ, MapFile map_data);

void wallSlide(float newX, float newZ, MapFile map_data);

void loadModelMTL(const char* file_name, vector<Material>& mat_list);

void SetMaterial(int shaderProgram, vector<Material> mat_list, int ind);

void SendMaterialsToShader(int shaderProgram, vector<Material> mat_list);

void SetLights(int shaderProgram, PointLights point_lights);

int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screenWidth, screenHeight, SDL_WINDOW_OPENGL);

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);

	//Mouse motion
	SDL_SetRelativeMouseMode(SDL_TRUE);

	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}


	// load different material files
	// light emmitting material - 0
	loadModelMTL("models/light.mtl", mat_list);  // 1st, lights
	
	//Here we will load three different model files 
	const int n = 9;
	//Load Model 1 - cube
	int numLines = 0;
	float* modelCube = loadModelOBJwithMTL("models/cube-tri.obj", numLines, "models/cube-tri.mtl", mat_list);
	int numVertsCube = numLines / n;
	printf("%d, %d\n", numLines, numVertsCube);

	//Load Model 2 - sword
	numLines = 0;
	float* modelSword = loadModelOBJwithMTL("models/sword-tri.obj", numLines, "models/sword-tri.mtl", mat_list);
	int numVertsSword = numLines / n;
	printf("%d, %d\n", numLines, numVertsSword);

	//Load Model 3 - floor
	numLines = 0;
	float* modelFloor = loadModelOBJwithMTL("models/ModularFloor-tri.obj", numLines, "models/ModularFloor-tri.mtl", mat_list);
	int numVertsFloor = numLines / n;
	printf("%d, %d\n", numLines, numVertsFloor);

	// load model 4 - key and it's material
	numLines = 0;
	float* modelKey1 = loadModelOBJwithMTL("models/key1-tri.obj", numLines, "models/key1-tri.mtl", mat_list);
	int numVertsKey1 = numLines / n;
	printf("%d, %d\n", numLines, numVertsKey1);

	// load model 5 - potion and it's material
	numLines = 0;
	float* modelPotion = loadModelOBJwithMTL("models/potion-tri.obj", numLines, "models/potion-tri.mtl", mat_list);
	int numVertsPotion = numLines / n;
	printf("%d, %d\n", numLines, numVertsPotion);

	
	//SJG: I load each model in a different array, then concatenate everything in one big array
	// This structure works, but there is room for improvement here. Eg., you should store the start
	// and end of each model a data structure or array somewhere.
	//Concatenate model arrays
	
	float* modelData = new float[(numVertsCube + numVertsSword + numVertsFloor + numVertsKey1 + numVertsPotion) * n];
	copy(modelCube,   modelCube + numVertsCube * n,     modelData);
	copy(modelSword,  modelSword + numVertsSword * n,   modelData + numVertsCube * n);
	copy(modelFloor,  modelFloor + numVertsFloor * n,   modelData + numVertsCube * n + numVertsSword * n);
	copy(modelKey1,   modelKey1 + numVertsKey1 * n,     modelData + numVertsCube * n + numVertsSword * n + numVertsFloor * n);
	copy(modelPotion, modelPotion + numVertsPotion * n, modelData + numVertsCube * n + numVertsSword * n + numVertsFloor * n + numVertsKey1 * n);

	int totalNumVerts = numVertsCube + numVertsSword + numVertsFloor + numVertsKey1 + numVertsPotion;
	int startVertCube = 0;  //The cube is the first model in the VBO
	int startVertSword = numVertsCube; //The sword starts right after the cube
	int startVertFloor = numVertsCube + numVertsSword; //The floor starts right after the sword
	int startVertKey = numVertsCube + numVertsSword + numVertsFloor; //The key starts right after the floor
	int startPotion = numVertsCube + numVertsSword + numVertsFloor + numVertsKey1;
	
	// use these for DrawGeometry, its too cumbersome to add all of them
	vector<int> modelNumVerts = { numVertsCube, numVertsSword, numVertsFloor, numVertsKey1, numVertsPotion };
	vector<int> modelStarts = { startVertCube, startVertSword, startVertFloor, startVertKey, startPotion };
	


	//// Allocate Texture 0 (Wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("textures/wood.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex0;
    glGenTextures(1, &tex0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    //Load the texture into memory
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w,surface->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_FreeSurface(surface);
    //// End Allocate Texture ///////


	//// Allocate Texture 1 (Brick) ///////
	SDL_Surface* surface1 = SDL_LoadBMP("textures/brick.bmp");
	if (surface==NULL){ //If it failed, print the error
        printf("Error: \"%s\"\n",SDL_GetError()); return 1;
    }
    GLuint tex1;
    glGenTextures(1, &tex1);
    
    //Load the texture into memory
    glActiveTexture(GL_TEXTURE1);
    
    glBindTexture(GL_TEXTURE_2D, tex1);
    //What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //How to filter
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface1->w,surface1->h, 0, GL_BGR,GL_UNSIGNED_BYTE,surface1->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); //Mip maps the texture
    
    SDL_FreeSurface(surface1);
	//// End Allocate Texture ///////
	




	// TODO: This is where the shader files begin.
	// https://sites.google.com/site/gsucomputergraphics/educational/how-to-implement-lighting
	//Build a Vertex Array Object (VAO) to store mapping of shader attributes to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time
	// TODO: This could be a line for errors, added integer to the vbo for mat index
	glBufferData(GL_ARRAY_BUFFER, static_cast<unsigned long long>(totalNumVerts) * (8 * sizeof(float) + sizeof(int)), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used
	
	int texturedShader = InitShader("shaders/textured-Vertex.glsl", "shaders/textured-Fragment.glsl");	
	
	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib,  3, GL_FLOAT, GL_FALSE, (8 * sizeof(float) + sizeof(int)), 0);
	//Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);

	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, (8 * sizeof(float) + sizeof(int)), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(normAttrib);

	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, (8 * sizeof(float) + sizeof(int)), (void*)(3 * sizeof(float)));

	GLint matAttrib = glGetAttribLocation(texturedShader, "inMatIndex");
	glVertexAttribPointer(matAttrib, 1, GL_INT, GL_FALSE, (8 * sizeof(float) + sizeof(int)), (void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(matAttrib);

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one	

	glEnable(GL_DEPTH_TEST);

	printf("%s\n", INSTRUCTIONS);


	// get the map file data
	MapFile map_data = MapFile();
	loadMapFile("maps/complicated.txt", map_data);  // test_with_doors complicated


	// set up multiple point lights
	int count = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			point_lights.pos[count] = glm::vec3(3+i*10, 2, 3+j*10);  // remember, y is up
			point_lights.color[count] = glm::vec3(1, 1, 1);
			count++;
		}
	}
	point_lights.size = count;
	point_lights.debug();



	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;
	// add some flags
	bool is_w = false;
	bool is_a = false;
	bool is_s = false;
	bool is_d = false;
	bool is_q = false;
	bool is_e = false;

	//movement
	int accel = 1;
	float slowFactor = 0.75;

	// mouse motion variables
	static int xpos = screenWidth / 2; // = 400 to center the cursor in the window
	static int ypos = screenHeight / 2; // = 300 to center the cursor in the window

	//jump feature
	float jumpStartTime = SDL_GetTicks() / 1000.f;
	//printf("jump start time is %f\n", jumpStartTime);
	float MaxInAirTime = 0.8; //Leave ground to back to ground, total 1 second.
	float HalfMaxInAirTime = MaxInAirTime / 2;
	float jumpMaxHight = 1;
	float jumpHight = 0;
	bool inAir = false;

	//printf("number of materials right now: %d\n", mat_list.size());
	//for (int i = 0; i < mat_list.size(); i++) {
	//	mat_list[i].debug();
	//}

	while (!quit) {
		if (goal_found) {
			printf("\n\n\n*******************************\n\nCongrats! You found the teapot!\n\n*******************************\n\n\n");
			quit = true;
		}

		while (SDL_PollEvent(&windowEvent)) {  //inspect all events in the queue
			const Uint8* state = SDL_GetKeyboardState(NULL);


			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) { //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
			}
			if(windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_SPACE && inAir==false) { // if "space" is pressed and character is not in air (character on ground)
				jumpStartTime = SDL_GetTicks() / 1000.f;
				printf("jump start time is %f\n", jumpStartTime);
				inAir = true;
			}


			// check for acceleration modifier
			if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]) accel = 3;
			else accel = 1;

			// if movement keys are pressed
			if (windowEvent.type == SDL_KEYDOWN) {
				if (windowEvent.key.keysym.sym == SDLK_w) is_w = true;
				if (windowEvent.key.keysym.sym == SDLK_s) is_s = true;
				if (windowEvent.key.keysym.sym == SDLK_a) is_a = true;
				if (windowEvent.key.keysym.sym == SDLK_d) is_d = true;
				if (windowEvent.key.keysym.sym == SDLK_q) is_q = true;
				if (windowEvent.key.keysym.sym == SDLK_e) is_e = true;
			}
			// if movement keys are released
			if (windowEvent.type == SDL_KEYUP) {
				if (windowEvent.key.keysym.sym == SDLK_w) is_w = false;
				if (windowEvent.key.keysym.sym == SDLK_s) is_s = false;
				if (windowEvent.key.keysym.sym == SDLK_a) is_a = false;
				if (windowEvent.key.keysym.sym == SDLK_d) is_d = false;
				if (windowEvent.key.keysym.sym == SDLK_q) is_q = false;
				if (windowEvent.key.keysym.sym == SDLK_e) is_e = false;
				
			}

			//If mouse motion event happened
			if (windowEvent.type == SDL_MOUSEMOTION)
			{
				//Get mouse position
				xpos += windowEvent.motion.xrel;
				ypos += windowEvent.motion.yrel;
				//printf("Mouse x= %d\n", xpos);
				//printf("Mouse y= %d\n", ypos);

			}

		}

		float v_x = 0.0;
		float v_z = 0.0;
		float w = 0.0;
		// perform movements 
		if (is_w) {  // If "w key" is pressed 
			v_x = v_x + cam_speed * cam_dir.x * accel;
			v_z = v_z + cam_speed * cam_dir.z * accel;
		}
		if (is_s) {  // If "s key" is pressed
			v_x = v_x - cam_speed * cam_dir.x * accel;
			v_z = v_z - cam_speed * cam_dir.z * accel;
		}
		if (is_a) {  // If "a key" is pressed
			v_x = v_x + cam_speed * cam_dir.z * accel * slowFactor;
			v_z = v_z - cam_speed * cam_dir.x * accel * slowFactor;
		}
		if (is_d) {  // If "d key" is pressed
			v_x = v_x - cam_speed * cam_dir.z * accel * slowFactor;
			v_z = v_z + cam_speed * cam_dir.x * accel * slowFactor;
		}
		cam_angle = double(xpos) / 1000;
		cam_dir.y = double(-ypos) / 1000;
		setCamDirFromAngle(cam_angle);

		


		// check for collision
		float new_x = cam_pos.x + v_x * dt;
		float new_z = cam_pos.z + v_z * dt;
		// return true if movement does not collide with objects
		if (isWalkableAndEvents(new_x, new_z, map_data)) {
			// change camera position if we should move
			cam_pos.x = new_x;
			cam_pos.z = new_z;
		}
		else {
			wallSlide(new_x, new_z, map_data);
		}

		// check if e was pressed to drop key
		if (is_e) {
			dropKey(new_x, new_z, map_data);
		}

		// Clear the screen to default color
		glClearColor(.2f, 0.4f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(texturedShader);

		float time = SDL_GetTicks() / 1000.f;
		dt = time - timePast;
		timePast = time;

		//change camera postion if character is inAir (jump)
		float jumpPassTime = time - jumpStartTime;
		if (jumpPassTime >= MaxInAirTime) {
			inAir = false; 
			cam_pos.y = 0;
		}
		else { 
			float tempX = 0;
			if (jumpPassTime < HalfMaxInAirTime) {
				tempX = jumpPassTime;
			}
			else {
				tempX = MaxInAirTime - jumpPassTime;
			}
			jumpHight = jumpMaxHight * tempX / HalfMaxInAirTime;
			cam_pos.y = jumpHight;
		}



		glm::mat4 view = glm::lookAt(cam_pos, cam_pos + cam_dir, cam_up);

		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 proj = glm::perspective(3.14f / 4, screenWidth / (float)screenHeight, 0.1f, 40.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		glBindVertexArray(vao);
		drawGeometry(texturedShader, modelNumVerts, modelStarts, map_data);

		SDL_GL_SwapWindow(window); //Double buffering
	}

	//Clean Up
	delete[] modelCube;
	delete[] modelSword;
	delete[] modelFloor;
	delete[] modelKey1;
	delete[] modelPotion;
	delete[] modelData;
	delete map_data.data;
	glDeleteProgram(texturedShader);
	glDeleteBuffers(1, vbo);
	glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

// all rendering code goes here
void drawGeometry(int shaderProgram, vector<int> modelNumVerts, vector<int> modelStarts, MapFile map_data){
	
    GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");


	// set point lights
	SetLights(shaderProgram, point_lights);

	//SendMaterialsToShader(shaderProgram, mat_list);

	static bool cam_start = true;
	//************
	// Draw map file models
	// go through each map tile
	//*************
	// indexes for models
	// modelCube;   0
	// modelSword;  1
	// modelFloor;  2
	// modelKey1;   3
	// modelPotion; 4
	for (int j = 0; j < map_data.height; j++) { 
		for (int i = 0; i < map_data.width; i++) {
			char map_type = map_data.data[j * map_data.width + i];
			bool draw_floor = false;
			// create walls
			if (map_type == 'W') {  
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

				//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
				glUniform1i(uniTexID, 1);
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, modelStarts[0], modelNumVerts[0]); //(Primitive Type, Start Vertex, Num Verticies)
			}
			// set camera to S position
			else if (map_type == 'S') {  
				if (cam_start) {
					cam_pos.x = (float)i;
					cam_pos.z = (float)j;
					cam_start = false;
				}
				draw_floor = true;
			}
			// create goal, currently is a sword
			else if (map_type == 'G') {  
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, sin(timePast) * 0.18, j));
				model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
				model = glm::rotate(model, timePast * 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));

				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

				//Set which texture to use (-1 = no texture)
				glUniform1i(uniTexID, -1);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, modelStarts[1], modelNumVerts[1]); //(Primitive Type, Start Vertex, Num Verticies)

				draw_floor = true;
			}
			// draw floor
			else if (map_type == 'O') {  
				draw_floor = true;
			}
			// Doors
			else if (map_type == 'A' || map_type == 'B' || map_type == 'C' || map_type == 'D' || map_type == 'E') {
				// set color
				//if (map_type == 'A') SetMaterial(shaderProgram, mat_list, 1);
				//else if (map_type == 'B') SetMaterial(shaderProgram, mat_list, 2);
				//else if (map_type == 'C') SetMaterial(shaderProgram, mat_list, 3);
				//else if (map_type == 'D') SetMaterial(shaderProgram, mat_list, 4);
				//else SetMaterial(shaderProgram, mat_list, 5);


				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

				//Set which texture to use (-1 = no texture ... bound to GL_TEXTURE1)
				glUniform1i(uniTexID, -1);
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, modelStarts[0], modelNumVerts[0]); //(Primitive Type, Start Vertex, Num Verticies)
			}
			// Keys
			else if (map_type == 'a' || map_type == 'b' || map_type == 'c' || map_type == 'd' || map_type == 'e') {
				// set color
				if (map_type == 'a') SetMaterial(shaderProgram, mat_list, 1);
				else if (map_type == 'b') SetMaterial(shaderProgram, mat_list, 2);
				else if (map_type == 'c') SetMaterial(shaderProgram, mat_list, 3);
				else if (map_type == 'd') SetMaterial(shaderProgram, mat_list, 4);
				else SetMaterial(shaderProgram, mat_list, 5);
		
				
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, sin(timePast) * 0.18, j));
				model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
				model = glm::rotate(model, timePast * 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));

				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

				//Set which texture to use (-1 = no texture, material instead)
				glUniform1i(uniTexID, -1);

				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, modelStarts[3], modelNumVerts[3]); //(Primitive Type, Start Vertex, Num Verticies)

				draw_floor = true;
			}

			// draw the floor
			if (draw_floor) {
				SetMaterial(shaderProgram, mat_list, 12);

				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, -0.625, j));
				model = glm::scale(model, glm::vec3(0.5f));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
				//Set which texture to use (0 = wood)
				glUniform1i(uniTexID, -1);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, modelStarts[2], modelNumVerts[2]); //(Primitive Type, Start Vertex, Num Verticies
			}
		}
	}

	// draw lights
	for (int i = 0; i < point_lights.size; i++) {
		// for each light, draw them as a square
		glm::mat4 model = glm::mat4(1);
		model = glm::translate(model, point_lights.pos[i]);
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));

		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
		glUniform1i(uniTexID, -1);  //Set which texture to use (-1 = no texture)
		SetMaterial(shaderProgram, mat_list, 0);
		glDrawArrays(GL_TRIANGLES, modelStarts[0], modelNumVerts[0]);
	}

	// changes due to events
	// render inventoried key
	if (activeKey != '0') {
		// set color
		if (activeKey == 'a') SetMaterial(shaderProgram, mat_list, 1);
		else if (activeKey == 'b') SetMaterial(shaderProgram, mat_list, 2);
		else if (activeKey == 'c') SetMaterial(shaderProgram, mat_list, 3);
		else if (activeKey == 'd') SetMaterial(shaderProgram, mat_list, 4);
		else SetMaterial(shaderProgram, mat_list, 5);


		// use key that is
		glm::mat4 model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(cam_pos.x + cam_dir.x/2, -0.1, cam_pos.z + cam_dir.z/2));
		model = glm::scale(model, glm::vec3(.3f, .3f, .3f));
		model = glm::rotate(model, -cam_angle, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));

		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
		//Set which texture to use (-1 = no texture)
		glUniform1i(uniTexID, -1);
		//Draw an instance of the model (at the position & orientation specified by the model matrix above)
		glDrawArrays(GL_TRIANGLES, modelStarts[3], modelNumVerts[3]); //(Primitive Type, Start Vertex, Num Verticies)
	}
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	long length;
	char* buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move positi6on indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

	// allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

	// append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName) {
	GLuint vertex_shader, fragment_shader;
	GLchar* vs_text, * fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char* vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders

	// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Load Fragment Shader
	const char* ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);

	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}

// load in a file to draw maps with
void loadMapFile(const char* file_name, MapFile& map_data) {
	// open file and read first line for width and height
	ifstream map_file(file_name);
	if (!(map_file >> map_data.width >> map_data.height)) {
		cout << "Error! Map file first line not read.\n";
	}

	// create data array
	map_data.data = new char[map_data.height * map_data.width];

	// fill data array
	for (int j = 0; j < map_data.height; j++) {
		for (int i = 0; i < map_data.width; i++) {
			map_file >> map_data.data[j * map_data.width + i];
		}
	}
}

// Can the character walk to the tile?
bool isWalkableAndEvents(float newX, float newZ, MapFile map_data) {
	// calc player offset x and z
	bool verbose = false;
	float x = newX + 0.5f;
	float z = newZ + 0.5f;
	vector<int> bounds = { -1, 1, 2 };
	for (auto dx : bounds) {
		for (auto dz : bounds) {
			int w = ceil(x + char_radius * dx);
			int h = ceil(z + char_radius * dz);
			if (w < 1 || h < 1 || w > map_data.width || h > map_data.height) {
				if (verbose) printf("OOB w %d, h %d, width %d, heigh %.d\n", w, h, map_data.width, map_data.height);
				return false;
			}
			int ind = (h - 1) * map_data.width + (w - 1);
			char map_tile = map_data.data[ind];
			//printf("x/w {%.4f}/{%d}    z/h {%.4f}/{%d}    map ind/type {%d}/{%c}\n", x, w, z, h, ind, map_tile);
			if (isWall(map_tile)) {  // is wall
				if (verbose) printf("wall w %d, h %d, width %d, heigh %.d\n", w, h, map_data.width, map_data.height);
				return false;
			}
			if (isDoor(map_tile)) {  // if it's a door 
				if (doorToKey[map_tile] == activeKey) {  //and you have the right key
					if (verbose) printf("door with key w %d, h %d, width %d, heigh %.d\n", w, h, map_data.width, map_data.height);
					// remove key and remove door, now you can move
					activeKey = '0';
					map_data.data[ind] = 'O';
					return true;
				}
				else {  // it is still locked and you cannot move
					if (verbose) printf("door no key w %d, h %d, width %d, heigh %.d\n", w, h, map_data.width, map_data.height);
					return false;
				}
			}
			// check if it's a key tile and no active key
			if (isKey(map_tile)) {
				if (verbose) printf("grab key w %d, h %d, width %d, heigh %.d\n", w, h, map_data.width, map_data.height);
				// no active key, pick it up and remove from map, can now move
				if (activeKey == '0') {
					activeKey = map_tile;
					map_data.data[ind] = 'O';
				}
				// either situation, no key and pick it up or if a key is already present, you can move
				return true;
			}
			// we are at the goal
			if (map_tile == 'G') {
				// TODO: add in something fancy here
				if (verbose) printf("goal w %d, h %d, width %d, heigh %.d\n", w, h, map_data.width, map_data.height);
				goal_found = true;
				return true;
			}
		}
	}
	if (verbose) printf("you are clear to move\n");
	return true;
}

// load .obj type models
float* loadModelOBJwithMTL(const char* file_nameOBJ, int& numLines, const char* file_nameMTL, vector<Material>& mat_list) {
	// load materials
	loadModelMTL(file_nameMTL, mat_list);

	// Using the OBJ models from Stephen Guy (https://drive.google.com/drive/u/0/folders/1aRQUoDFDjEUdB-7Ol0cz9Bftv-kCkog-)
	// it appears that these OBJ models use square faces and not triangles. 
	// Therefor, I went into blender and converted all faces into triangles
	FILE* fp = fopen(file_nameOBJ, "r");

	vector<glm::vec3> vertex_arr;
	vector<glm::vec2> texture_arr;
	vector<glm::vec3> normal_arr;
	vector<int> vertex_ind;
	vector<int> texture_ind;
	vector<int> normal_ind;
	vector<int> material_ind;
	int num_tri = 0;

	if (fp == NULL) {
		printf("Can't open file '%s'\n", file_nameOBJ);
		return nullptr;
	}
	bool has_vt = false;
	char line[1024];
	int curr_mat_ind = 0;
	while (fgets(line, 1024, fp)) {
		if (line[0] == '#') {  // skip commented lines
			continue;
		}
		char command[100];
		int word_count = sscanf(line, "%s ", command);  // read first word in the line
		if (word_count < 1) {  // empty line
			continue;
		}
		string commandStr = command;
		if (commandStr == "v") {  // vertex
			glm::vec3 vertex;
			sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
			vertex_arr.push_back(vertex);
		}
		else if (commandStr == "vt") {  // vertex texture coordinate
			if (!has_vt) {
				has_vt = true;
			}
			glm::vec2 texture;
			sscanf(line, "vt %f %f", &texture.x, &texture.y);
			texture_arr.push_back(texture);
		}
		else if (commandStr == "vn") {  // vertex normal
			glm::vec3 normal;
			sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
			normal_arr.push_back(normal);
		}
		else if (commandStr == "f") {  // triangle face
			if (has_vt) {
				int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;
				sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				vertex_ind.push_back(v1); texture_ind.push_back(vt1); normal_ind.push_back(vn1); material_ind.push_back(curr_mat_ind);
				vertex_ind.push_back(v2); texture_ind.push_back(vt2); normal_ind.push_back(vn2); material_ind.push_back(curr_mat_ind);
				vertex_ind.push_back(v3); texture_ind.push_back(vt3); normal_ind.push_back(vn3); material_ind.push_back(curr_mat_ind);
				
			}
			else {
				int v1, vn1, v2, vn2, v3, vn3;
				sscanf(line, "f %d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
				vertex_ind.push_back(v1); normal_ind.push_back(vn1); material_ind.push_back(curr_mat_ind);
				vertex_ind.push_back(v2); normal_ind.push_back(vn2); material_ind.push_back(curr_mat_ind);
				vertex_ind.push_back(v3); normal_ind.push_back(vn3); material_ind.push_back(curr_mat_ind);
			}
			num_tri++;
		}
		else if (commandStr == "usemtl") {
			char mtl[20];
			sscanf(line, "usemtl %s", mtl);
			// find material properties and use that too in VAO/VBO TODO
			for (int i = 0; i < mat_list.size(); i++) {
				if (strcmp(mat_list[i].newmtl, mtl) == 0) {
					curr_mat_ind = i;
					//cout << i << endl;
					//mat_list[i].debug();
				}
			}
		}
	}

	fclose(fp);
	numLines = vertex_ind.size() * 9;  // 3 floats for x,y,z of vertex, 2 for u,v texture coordinates, 3 for vertex normal x,y,z, 1 mat index
	float* model1 = new float[numLines];

	//printf("num_tri %d, lines %d, size of vertexInd %d, textureInd %d, normalInd %d, materialInd %d\n", num_tri, numLines, vertex_ind.size(), texture_ind.size(), normal_ind.size(), material_ind.size());
	//printf("size of vertexArr %d, size of textureArr %d, size of normalArr %d\n", vertex_arr.size(), texture_arr.size(), normal_arr.size());
	//
	//cout << endl;
	//for (int i = 0; i < material_ind.size(); i++) {
	//	int j;
	//	for (j = 0; j < i; j++)
	//		if (material_ind[i] == material_ind[j])
	//			break;
	//	if (i == j)
	//		cout << material_ind[i] << " ";
	//}
	//cout << endl;

	for (int i = 0; i < vertex_ind.size(); i++) {
		// vertex x,y,z
		model1[(i * 9)] = vertex_arr[vertex_ind[i] - 1].x;
		model1[(i * 9) + 1] = vertex_arr[vertex_ind[i] - 1].y;
		model1[(i * 9) + 2] = vertex_arr[vertex_ind[i] - 1].z;
		// texture map u,v
		if (has_vt) {
			model1[(i * 9) + 3] = texture_arr[texture_ind[i] - 1].x;
			model1[(i * 9) + 4] = texture_arr[texture_ind[i] - 1].y;
		}
		else {
			model1[(i * 9) + 3] = 0;
			model1[(i * 9) + 4] = 0;
		}
		// vertex normal x,y,z
		model1[(i * 9) + 5] = normal_arr[normal_ind[i] - 1].x;
		model1[(i * 9) + 6] = normal_arr[normal_ind[i] - 1].y;
		model1[(i * 9) + 7] = normal_arr[normal_ind[i] - 1].z;
		// material index
		model1[(i * 9) + 8] = material_ind[i];
	}

	printf("nice, successful obj loaded\n");
	return model1;
}

void loadModelMTL(const char* file_name, vector<Material>& mat_list) {
	FILE* fp = fopen(file_name, "r");
	if (fp == NULL) {
		perror("Can't open file");
	}
	Material mat;
	char line[1024];
	while (fgets(line, 1024, fp)) {
		if (line[0] == '#') {  // skip commented lines
			continue;
		}
		char command[100];
		int word_count = sscanf(line, "%s ", command);  // read first word in the line
		if (word_count < 1) {  // empty line
			continue;
		}
		
		std::string commandStr = command;
		if (commandStr == "newmtl") {  // beginning of new material
			mat = Material();  // so reset mat to a new Material. It will populate below a
			char newmtl[20];
			sscanf(line, "newmtl %s", newmtl);
			strcpy(mat.newmtl, newmtl);
		}
		else if (commandStr == "Ns") {
			float ns = -1.0;
			sscanf(line, "Ns %f", &ns);
			mat.Ns = ns;
		}
		else if (commandStr == "Ka") {
			//glm::vec3 vertex;
			//sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
			//vertex_arr.push_back(vertex);
			glm::vec3 Ka = glm::vec3(-1.0,-1.0,-1.0);
			sscanf(line, "Ka %f %f %f", &Ka.x, &Ka.y, &Ka.z);
			mat.Ka = Ka;
		}
		else if (commandStr == "Kd") {
			glm::vec3 Kd = glm::vec3(-1.0, -1.0, -1.0);
			sscanf(line, "Kd %f %f %f", &Kd.x, &Kd.y, &Kd.z);
			mat.Kd = Kd;
		}
		else if (commandStr == "Ks") {
			glm::vec3 Ks = glm::vec3(-1.0, -1.0, -1.0);
			sscanf(line, "Ks %f %f %f", &Ks.x, &Ks.y, &Ks.z);
			mat.Ks = Ks;
		}
		else if (commandStr == "Ke") {
			glm::vec3 Ke = glm::vec3(-1.0, -1.0, -1.0);
			sscanf(line, "Ke %f %f %f", &Ke.x, &Ke.y, &Ke.z);
			mat.Ke = Ke;
		}
		else if (commandStr == "illum") {  // this is the last item in a material, so add material to list
			mat_list.push_back(mat);
		}
	}
	fclose(fp);
}

// check how to, or if, to drop the key
void dropKey(float vx, float vz, MapFile map_data) {
	// calc player offset x and z
	float x = vx + 0.5f;
	float z = vz + 0.5f;
	// find indexing for map data
	int w = ceil(x);
	int h = ceil(z);
	int ind = (h - 1) * map_data.width + (w - 1);
	char map_tile = map_data.data[ind];
	// only drop key if on an open square and we have a key
	if (map_tile == 'O' && activeKey != '0') {
		int h_offset = 0;
		int w_offset = 0;
		if (cam_dir.x > 0.5) {
			w_offset = 1;
		}
		else if (cam_dir.x < -0.5) {
			w_offset = -1;
		}
		else {
			w_offset = 0;
		}

		if (cam_dir.z > 0.5) {
			h_offset = 1;
		}
		else if (cam_dir.z < -0.5) {
			h_offset = -1;
		}
		else {
			h_offset = 0;
		}

		// set map tile to key that is the next one in front of the character
		ind = (h - 1 + h_offset) * map_data.width + (w - 1 + w_offset);
		// keep in bounds of  map_data
		if (ind >= map_data.width * map_data.height) ind = map_data.width * map_data.height - 1;
		if (ind < 0) ind = 0;
		if (map_data.data[ind] == 'O') {
			map_data.data[ind] = activeKey;
			// set active key to none
			activeKey = '0';
		}
	}
}

// move even thought I am hitting an impassable object
void wallSlide(float newX, float newZ, MapFile map_data) {
	bool verbose = false;
	float x = newX + 0.5f;
	float z = newZ + 0.5f;
	//int ind = (ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1);
	//// current tile that we are on. It is a moveable tile because we never move onto something that is impassable
	//// as determined with velocity collision calculations
	//char curr_map = map_data.data[ind];
	//char right_map = map_data.data[ind - 1];  // hitting cube on the right side of it is ind-1
	//char left_map = map_data.data[ind + 1];  // hitting cube on the left side of it is ind+1
	//char top_map = map_data.data[ind + map_data.width];  // hitting cube on the top side of it is ind-map_data.width
	//char bottom_map = map_data.data[ind - map_data.width];  // hitting cube on the bottom side of it is ind+map_data.width

	char curr_map = map_data.data[(int)((ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1))];
	char right_map = map_data.data[(int)((ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1)) - 1];  // hitting cube on the right side of it is ind-1
	char left_map = map_data.data[(int)((ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1)) + 1];  // hitting cube on the left side of it is ind+1
	char top_map = map_data.data[(int)((ceil(z + char_radius)) * map_data.width + (ceil(x + char_radius) - 1))];  // hitting cube on the top side of it is ind-map_data.width
	char bottom_map = map_data.data[(int)((ceil(z + char_radius) - 2) * map_data.width + (ceil(x + char_radius) - 1))];  // hitting cube on the bottom side of it is ind+map_data.width

	//string all(1, curr_map);
	//all.append(1, right_map); all.append(1, left_map); all.append(1, top_map); all.append(1, bottom_map);
	//int count = 0;
	//for (auto c : all) {
	//	if (isDoor(c) || isWall(c)) count++;
	//}

	if (verbose) printf("r %c   l %c   t %c   b %c   ", right_map, left_map, top_map, bottom_map);
	// don't move if you are on a blocked tile and or if there are 2 blocked things
	if (!(isDoor(curr_map) || isWall(curr_map))) {
		// if right side and we are hitting a wall or closed door
		if ((isDoor(right_map) || isWall(right_map))) {
			if (verbose) printf("hitting right side of cube\n");
			newX = cam_pos.x;  // essentially put velocity in x direction to 0
			if (isWalkableAndEvents(newX, newZ, map_data)) {
				cam_pos.z = newZ;
			}
		}
		// if left
		else if ((isDoor(left_map) || isWall(left_map))) {
			if (verbose) printf("hitting left side of cube\n");
			newX = cam_pos.x;  // essentially put velocity in x didrection to 0
			if (isWalkableAndEvents(newX, newZ, map_data)) {
				cam_pos.z = newZ;
			}
		}
		// if top side
		else if ((isDoor(top_map) || isWall(top_map))) {
			if (verbose) printf("hitting top side of cube\n");
			newZ = cam_pos.z;  // essentially put velocity in z direction to 0
			if (isWalkableAndEvents(newX, newZ, map_data)) {
				cam_pos.x = newX;
			}
		}
		// if bottom
		else if ((isDoor(bottom_map) || isWall(bottom_map))) {
			if (verbose) printf("hitting bottom side of cube\n");
			newZ = cam_pos.z;  // essentially put velocity in z direction to 0
			if (isWalkableAndEvents(newX, newZ, map_data)) {
				cam_pos.x = newX;
			}
		}
		if (right_map == 'O' && left_map == 'O' && top_map == 'O' && bottom_map == 'O') {
			if (verbose) printf("no obstruction, keep it going\n");
			cam_pos.x = newX;
			cam_pos.z = newZ;
		}
	}
}

void SetMaterial(int shaderProgram, vector<Material> mat_list, int ind) {
	// TODO: have a uniform bool that tells the shader if to use this or not
	
	Material mat = mat_list.at(ind);

	GLint uniKa = glGetUniformLocation(shaderProgram, "mat.Ka");
	glUniform3fv(uniKa, 1, glm::value_ptr(mat.Ka));

	GLint uniKd = glGetUniformLocation(shaderProgram, "mat.Kd");
	glUniform3fv(uniKd, 1, glm::value_ptr(mat.Kd));

	GLint uniKs = glGetUniformLocation(shaderProgram, "mat.Ks");
	glUniform3fv(uniKs, 1, glm::value_ptr(mat.Ks));

	GLint uniKe = glGetUniformLocation(shaderProgram, "mat.Ke");
	glUniform3fv(uniKe, 1, glm::value_ptr(mat.Ke));

	GLint uniNs = glGetUniformLocation(shaderProgram, "mat.Ns");
	glUniform1f(uniNs, mat.Ns);
}

void SendMaterialsToShader(int shaderProgram, vector<Material> mat_list) {
	//for (int i = 0; i < 13; i++) {
	//	Material mat = mat_list[i];
	//	char buf[32];

	//	sprintf(buf, "inMaterials[%d].Ka", i);
	//	GLint uniKa = glGetUniformLocation(shaderProgram, buf);
	//	glUniform3fv(uniKa, 1, glm::value_ptr(mat.Ka));

	//	sprintf(buf, "inMaterials[%d].Kd", i);
	//	GLint uniKd = glGetUniformLocation(shaderProgram, buf);
	//	glUniform3fv(uniKd, 1, glm::value_ptr(mat.Kd));

	//	sprintf(buf, "inMaterials[%d].Ks", i);
	//	GLint uniKs = glGetUniformLocation(shaderProgram, buf);
	//	glUniform3fv(uniKs, 1, glm::value_ptr(mat.Ks));

	//	sprintf(buf, "inMaterials[%d].Ke", i);
	//	GLint uniKe = glGetUniformLocation(shaderProgram, buf);
	//	glUniform3fv(uniKe, 1, glm::value_ptr(mat.Ke));
	//	glUniform3fv(uniKe, 1, glm::value_ptr(mat.Ke));

	//	sprintf(buf, "inMaterials[%d].Ns", i);
	//	GLint uniNs = glGetUniformLocation(shaderProgram, buf);
	//	glUniform1f(uniNs, mat.Ns);
	//}
	const int size = 13;
	glm::vec3 inMatsKa[size];
	glm::vec3 inMatsKs[size];
	glm::vec3 inMatsKd[size];
	glm::vec3 inMatsKe[size];
	float inMatsNs[size];

	for (int i = 0; i < size; i++) {
		Material mat = mat_list[i];
		inMatsKa[i] = mat.Ka;
		inMatsKs[i] = mat.Ks;
		inMatsKd[i] = mat.Kd;
		inMatsKe[i] = mat.Ke;
		inMatsNs[i] = mat.Ns;
	}


	GLint uniKa = glGetUniformLocation(shaderProgram, "inMatsKa");
	glUniform3fv(uniKa, size, glm::value_ptr(inMatsKa[0]));

	GLint uniKs = glGetUniformLocation(shaderProgram, "inMatsKs");
	glUniform3fv(uniKs, size, glm::value_ptr(inMatsKs[0]) );

	GLint uniKd = glGetUniformLocation(shaderProgram, "inMatsKd");
	glUniform3fv(uniKd, size, glm::value_ptr(inMatsKd[0]) );

	GLint uniKe = glGetUniformLocation(shaderProgram, "inMatsKe");
	glUniform3fv(uniKe, size, glm::value_ptr(inMatsKe[0]) );

	GLint uniNs = glGetUniformLocation(shaderProgram, "inMatsNs");
	glUniform1fv(uniNs, size, inMatsNs );


}

void SetLights(int shaderProgram, PointLights point_lights) {

	GLint uniPos = glGetUniformLocation(shaderProgram, "inPointLightsPOS");
	glUniform3fv(uniPos, point_lights.size, glm::value_ptr(point_lights.pos[0]));

	GLint uniColor = glGetUniformLocation(shaderProgram, "inPointLightsCOLOR");
	glUniform3fv(uniColor, point_lights.size, glm::value_ptr(point_lights.color[0]));
}
