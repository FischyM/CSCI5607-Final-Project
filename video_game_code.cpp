// Mathew Fischbach, CSCI 5607
// Code adapted from Stephen Guy


const char* INSTRUCTIONS = "Press W and S to move forward and backwards\n\
Press A and D to rotate left and right.\n\
Find the teapot to win!\n\
You may find doors blocking your way. Find the key to unlock these doors.\n\
Press E to drop a key on an open floor\n\
Press F to change between fullscreen and windowed.\n\
Press Esc to exit the game.\n";

//Mac OS build: g++ multiObjectTest.cpp -x c glad/glad.c -g -F/Library/Frameworks -framework SDL2 -framework OpenGL -o MultiObjTest
//Linux build:  g++ multiObjectTest.cpp -x c glad/glad.c -g -lSDL2 -lSDL2main -lGL -ldl -I/usr/include/SDL2/ -o MultiObjTest

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
glm::vec3 cam_dir = glm::vec3(-1.0f, 0.0f,-1.0f);  // Look at point
glm::vec3 cam_up = glm::vec3(0.0f, 1.0f, 0.0f);  // Up
float cam_angle = glm::atan(cam_dir.z, cam_dir.x);
float cam_speed = 2.0f;

void setCamDirFromAngle(float cam_angle) {
	cam_dir.z = glm::sin(cam_angle);
	cam_dir.x = glm::cos(cam_angle);
}


bool DEBUG_ON = true;
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
bool fullscreen = false;

struct MapFile{
	int width = 0;
	int height = 0;
	char* data = NULL;
};

map <char, bool> doorOpen = { {'A',false}, {'B',false}, {'C',false}, {'D',false}, {'E',false} };
map <char, char> doorToKey = { {'A','a'}, {'B','b'}, {'C','c'}, {'D','d'}, {'E','e'} };

bool isDoor(char type) {
	if (type=='A' || type == 'B' || type == 'C' || type == 'D' || type == 'E')
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

char activeKey = '0';


void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts,
					int model3_start, int model3_numVerts, int model4_start, int model4_numVerts, MapFile map_data);

static char* readShaderSource(const char* shaderFile);

void readMapFile(const char* file_name, MapFile& map_data);

float* loadModelOBJ(const char* file_name, int& numLines);

void dropKey(float vx, float vz, MapFile map_data);

bool isWalkableAndEvents(float newX, float newZ, MapFile map_data);

void wallSlide(float newX, float newZ, MapFile map_data);

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
	
	//Load OpenGL extentions with GLAD
	if (gladLoadGLLoader(SDL_GL_GetProcAddress)){
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}
	
	//Here we will load three different model files 
	
	//Load Model 1 - cube
	ifstream modelFile;
	modelFile.open("models/cube.txt");
	int numLines = 0;
	modelFile >> numLines;
	float* model1 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model1[i];
	}
	printf("%d\n",numLines);
	int numVertsCube = numLines/8;
	modelFile.close();
	
	//Load Model 2 - teapot
	modelFile.open("models/teapot.txt");
	numLines = 0;
	modelFile >> numLines;
	float* model2 = new float[numLines];
	for (int i = 0; i < numLines; i++){
		modelFile >> model2[i];
	}
	printf("%d\n",numLines);
	int numVertsTeapot = numLines/8;
	modelFile.close();

	//Load Model 3 - floor
	modelFile.open("models/floor.txt");
	numLines = 0;
	modelFile >> numLines;
	float* model3 = new float[numLines];
	for (int i = 0; i < numLines; i++) {
		modelFile >> model3[i];
	}
	printf("%d\n", numLines);
	int numVertsFloor = numLines / 8;
	modelFile.close();

	// load model 4 - key
	numLines = 0;
	float* model4 = loadModelOBJ("models/key.obj", numLines);
	int numVertsKey = numLines / 8;
	printf("%d, %d\n", numLines, numVertsKey);
	
	//SJG: I load each model in a different array, then concatenate everything in one big array
	// This structure works, but there is room for improvement here. Eg., you should store the start
	// and end of each model a data structure or array somewhere.
	//Concatenate model arrays
	float* modelData = new float[(numVertsCube + numVertsTeapot + numVertsFloor + numVertsKey)*8];
	copy(model1, model1+numVertsCube*8,   modelData);
	copy(model2, model2+numVertsTeapot*8, modelData+numVertsCube*8);
	copy(model3, model3+numVertsFloor*8,  modelData+numVertsCube*8+numVertsTeapot*8);
	copy(model4, model4+numVertsKey*8,    modelData+numVertsCube*8+numVertsTeapot*8+numVertsFloor*8);
	int totalNumVerts = numVertsCube + numVertsTeapot + numVertsFloor + numVertsKey;
	int startVertCube = 0;  //The cube is the first model in the VBO
	int startVertTeapot = numVertsCube; //The teapot starts right after the cube
	int startVertFloor =  numVertsCube + numVertsTeapot; //The floor starts right after the teapot
	int startVertKey =    numVertsCube + numVertsTeapot + numVertsFloor; //The floor starts right after the floor
	
	//// Allocate Texture 0 (Wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("wood.bmp");
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
	SDL_Surface* surface1 = SDL_LoadBMP("brick.bmp");
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
	
	//Build a Vertex Array Object (VAO) to store mapping of shader attributse to VBO
	GLuint vao;
	glGenVertexArrays(1, &vao); //Create a VAO
	glBindVertexArray(vao); //Bind the above created VAO to the current context

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo[1];
	glGenBuffers(1, vbo);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); //Set the vbo as the active array buffer (Only one buffer can be active at a time
	glBufferData(GL_ARRAY_BUFFER, static_cast<unsigned long long>(totalNumVerts)*8*sizeof(float), modelData, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used
	
	int texturedShader = InitShader("textured-Vertex.glsl", "textured-Fragment.glsl");	
	
	//Tell OpenGL how to set fragment shader input 
	GLint posAttrib = glGetAttribLocation(texturedShader, "position");
	glVertexAttribPointer(posAttrib,  3, GL_FLOAT, GL_FALSE, 8*sizeof(float), 0);
	  //Attribute, vals/attrib., type, isNormalized, stride, offset
	glEnableVertexAttribArray(posAttrib);
	
	//GLint colAttrib = glGetAttribLocation(texturedShader, "inColor");
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
	//glEnableVertexAttribArray(colAttrib);
	
	GLint normAttrib = glGetAttribLocation(texturedShader, "inNormal");
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(5*sizeof(float)));
	glEnableVertexAttribArray(normAttrib);
	
	GLint texAttrib = glGetAttribLocation(texturedShader, "inTexcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));

	GLint uniView = glGetUniformLocation(texturedShader, "view");
	GLint uniProj = glGetUniformLocation(texturedShader, "proj");

	glBindVertexArray(0); //Unbind the VAO in case we want to create a new one	
                       
	glEnable(GL_DEPTH_TEST);  

	printf("%s\n",INSTRUCTIONS);


	// get the map file data
	MapFile map_data = MapFile();
	readMapFile("maps/complicated.txt", map_data);  // test_with_doors complicated

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
	int accel = 1;
	

	while (!quit){
		if (goal_found) {
			printf("\n\n\n*******************************\n\nCongrats! You found the teapot!\n\n*******************************\n\n\n");
			quit = true;
		}

		while (SDL_PollEvent(&windowEvent)){  //inspect all events in the queue
			const Uint8* state = SDL_GetKeyboardState(NULL);


			if (windowEvent.type == SDL_QUIT) quit = true;
			//List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
			//Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f){ //If "f" is pressed
				fullscreen = !fullscreen;
				SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Toggle fullscreen 
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
		if (is_a) {  // If "q key" is pressed
			w = w - cam_speed;
		}
		if (is_d) {  // If "e key" is pressed
			w = w + cam_speed;
		}
		cam_angle = cam_angle + w * dt;
		setCamDirFromAngle(cam_angle);
		


		// check for collision
		float new_x = cam_pos.x + v_x * dt;
		float new_z = cam_pos.z + v_z * dt;
		// return true if movement does not collide with objects
		if ( isWalkableAndEvents(new_x, new_z, map_data) ) {
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

		glm::mat4 view = glm::lookAt(cam_pos, cam_pos+cam_dir, cam_up);

		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 proj = glm::perspective(3.14f/4, screenWidth / (float) screenHeight, 0.1f, 40.0f); //FOV, aspect, near, far
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(texturedShader, "tex0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);
		glUniform1i(glGetUniformLocation(texturedShader, "tex1"), 1);

		glBindVertexArray(vao);
		drawGeometry(texturedShader, startVertCube, numVertsCube, startVertTeapot, numVertsTeapot, startVertFloor, numVertsFloor,
					startVertKey, numVertsKey, map_data);

		SDL_GL_SwapWindow(window); //Double buffering
	}
	
	//Clean Up
	delete [] model1;
	delete [] model2;
	delete [] model3;
	delete [] model4;
	delete map_data.data;
	glDeleteProgram(texturedShader);
    glDeleteBuffers(1, vbo);
    glDeleteVertexArrays(1, &vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();
	return 0;
}

// all rendering code goes here
void drawGeometry(int shaderProgram, int model1_start, int model1_numVerts, int model2_start, int model2_numVerts,
					int model3_start, int model3_numVerts, int model4_start, int model4_numVerts, MapFile map_data){
	
	GLint uniColor = glGetUniformLocation(shaderProgram, "inColor");
	glm::vec3 colVec(1,1,1);
	glUniform3fv(uniColor, 1, glm::value_ptr(colVec));
      
    GLint uniTexID = glGetUniformLocation(shaderProgram, "texID");
	GLint uniModel = glGetUniformLocation(shaderProgram, "model");

	static bool cam_start = true;
	//************
	// Draw map file models
	// go through each map tile
	//*************
	// model 1 is a cube
	// model 2 is a teapot
	// model 3 is a square for floor
	for (int j = 0; j < map_data.height; j++) { 
		for (int i = 0; i < map_data.width; i++) {
			char map_type = map_data.data[j * map_data.width + i];
			if (map_type == 'W') {  // create walls
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

				//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
				glUniform1i(uniTexID, 1);
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)

			}
			else if (map_type == 'S') {  // set camera to S position
				if (cam_start) {
					cam_pos.x = (float)i;
					cam_pos.z = (float)j;
					cam_start = false;
				}

				// draw the floor
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
				//Set which texture to use (0 = wood)
				glUniform1i(uniTexID, 0);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model3_start, model3_numVerts); //(Primitive Type, Start Vertex, Num Verticies
			}
			else if (map_type == 'G') {  // create goal
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, sin(timePast) * 0.18, j));
				model = glm::scale(model, glm::vec3(.5f, .5f, .5f));
				model = glm::rotate(model, timePast * 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
				model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));
				
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

				//Set which texture to use (-1 = no texture)
				glUniform1i(uniTexID, -1);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model2_start, model2_numVerts); //(Primitive Type, Start Vertex, Num Verticies)


				// draw the floor
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
				//Set which texture to use (0 = wood)
				glUniform1i(uniTexID, 0);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model3_start, model3_numVerts); //(Primitive Type, Start Vertex, Num Verticies
			}
			else if (map_type == 'O') {  // draw floor
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
				//Set which texture to use (0 = wood)
				glUniform1i(uniTexID, 0);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model3_start, model3_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
			}
			// Doors
			else if (map_type == 'A' || map_type == 'B' || map_type == 'C' || map_type == 'D' || map_type == 'E') {
				// set color
				if (map_type == 'A') colVec = colorA;
				else if (map_type == 'B') colVec = colorB;
				else if (map_type == 'C') colVec = colorC;
				else if (map_type == 'D') colVec = colorD;
				else colVec = colorE;
				glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

				glm::mat4 model = glm::mat4(1); 
				model = glm::translate(model, glm::vec3(i, 0, j));

				//Set which texture to use (-1 = no texture ... bound to GL_TEXTURE1)
				glUniform1i(uniTexID, -1);
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)

				// draw the floor
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
				//Set which texture to use (0 = wood)
				glUniform1i(uniTexID, 0);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model3_start, model3_numVerts); //(Primitive Type, Start Vertex, Num Verticies
			}
			// Keys
			else if (map_type == 'a' || map_type == 'b' || map_type == 'c' || map_type == 'd' || map_type == 'e') {
				// set color
				if (map_type == 'a') colVec = colorA;
				else if (map_type == 'b') colVec = colorB;
				else if (map_type == 'c') colVec = colorC;
				else if (map_type == 'd') colVec = colorD;
				else colVec = colorE;
				glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

				// use teapot that is colored as the key
				glm::mat4 model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, -0.1 + sin(timePast) * 0.18, j));
				model = glm::scale(model, glm::vec3(.1f, .1f, .1f));
				model = glm::rotate(model, timePast * 3.14f / 2, glm::vec3(0.0f, 1.0f, 0.0f));
				//model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));

				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

				//Set which texture to use (-1 = no texture)
				glUniform1i(uniTexID, -1);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model4_start, model4_numVerts); //(Primitive Type, Start Vertex, Num Verticies)


				// draw the floor
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(i, 0, j));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader
				//Set which texture to use (0 = wood)
				glUniform1i(uniTexID, 0);
				//Draw an instance of the model (at the position & orientation specified by the model matrix above)
				glDrawArrays(GL_TRIANGLES, model3_start, model3_numVerts); //(Primitive Type, Start Vertex, Num Verticies

			}

			//// Creating implicit perimeter
			//if (i == 0) {  // on left edge, need left wall
			//	glm::mat4 model = glm::mat4(1);
			//	model = glm::translate(model, glm::vec3(-1, 0, j));
			//	model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));
			//	model = glm::scale(model, glm::vec3(1.00f, 1.00f, 1.00f));

			//	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
			//	glUniform1i(uniTexID, 1);
			//	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			//	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
			//	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
			//}
			//if (j == 0) {  // on the top, need top wall
			//	glm::mat4 model = glm::mat4(1);
			//	model = glm::translate(model, glm::vec3(i, 0, -1));
			//	model = glm::scale(model, glm::vec3(1.00f, 1.00f, 1.00f));

			//	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
			//	glUniform1i(uniTexID, 1);
			//	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			//	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
			//	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
			//}
			//if (i == map_data.width - 1) {  // on right edge, need right wall
			//	glm::mat4 model = glm::mat4(1);
			//	model = glm::translate(model, glm::vec3(i+1, 0, j));
			//	model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));
			//	model = glm::scale(model, glm::vec3(1.00f, 1.00f, 1.00f));

			//	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
			//	glUniform1i(uniTexID, 1);
			//	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			//	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
			//	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
			//}
			//if (j == map_data.height - 1) {  // on the bottom, need bottom wall
			//	glm::mat4 model = glm::mat4(1);
			//	model = glm::translate(model, glm::vec3(i, 0, j+1));
			//	model = glm::scale(model, glm::vec3(1.00f, 1.00f, 1.00f));
			//	

			//	//Set which texture to use (1 = brick texture ... bound to GL_TEXTURE1)
			//	glUniform1i(uniTexID, 1);
			//	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
			//	//Draw an instance of the model (at the position & orientation specified by the model matrix above)
			//	glDrawArrays(GL_TRIANGLES, model1_start, model1_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
			//}
		}
	}
	// changes due to events
	// render inventoried key
	if (activeKey != '0') {
		// set color
		if (activeKey == 'a') colVec = colorA;
		else if (activeKey == 'b') colVec = colorB;
		else if (activeKey == 'c') colVec = colorC; 
		else if (activeKey == 'd') colVec = colorD;
		else colVec = colorE;
		glUniform3fv(uniColor, 1, glm::value_ptr(colVec));

		// use key that is
		glm::mat4 model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(cam_pos.x + cam_dir.x/2, -0.1, cam_pos.z + cam_dir.z/2));
		model = glm::scale(model, glm::vec3(.03f, .03f, .03f));
		model = glm::rotate(model, -cam_angle, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, -3.14f / 2, glm::vec3(1.0f, 0.0f, 0.0f));

		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model)); //pass model matrix to shader

		//Set which texture to use (-1 = no texture)
		glUniform1i(uniTexID, -1);
		//Draw an instance of the model (at the position & orientation specified by the model matrix above)
		glDrawArrays(GL_TRIANGLES, model4_start, model4_numVerts); //(Primitive Type, Start Vertex, Num Verticies)
	}
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile){
	FILE *fp;
	long length;
	char *buffer;

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
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName){
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
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
void readMapFile(const char* file_name, MapFile& map_data) {
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
float* loadModelOBJ(const char* file_name, int& numLines) {
	// file_name = "models/key.obj";
	FILE* fp = fopen(file_name, "r");

	vector<glm::vec3> vertex_arr;
	vector<glm::vec2> texture_arr;
	vector<glm::vec3> normal_arr;
	vector<int> vertex_ind;
	vector<int> texture_ind;
	vector<int> normal_ind;
	int num_tri = 0;

	if (fp == NULL) {
		printf("Can't open file '%s'\n", file_name);
		return nullptr;
	}

	long length;
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
		if (commandStr == "v") {  // vertex
			glm::vec3 vertex;
			sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z);
			vertex_arr.push_back(vertex);
		}
		else if (commandStr == "vt") {  // vertex texture coordinate
			glm::vec2 texture;
			sscanf(line, "vt %f %f", &texture.x, &texture.y);
			texture_arr.push_back(texture);
		}
		else if (commandStr == "vn") {  // vertex normal
			glm::vec3 normal;
			sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
			normal_arr.push_back(normal);
		}
		else if (commandStr == "f") {  // triangle face, dictates ordering of vertices...i  think?
			int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;
			sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
			vertex_ind.push_back(v1); texture_ind.push_back(vt1); normal_ind.push_back(vn1);
			vertex_ind.push_back(v2); texture_ind.push_back(vt2); normal_ind.push_back(vn2);
			vertex_ind.push_back(v3); texture_ind.push_back(vt3); normal_ind.push_back(vn3);
			num_tri++;
		}
		
	}
	fclose(fp);
	numLines = vertex_ind.size() * 8;  // 3 floats for x,y,z of vertex, 2 for u,v texture coordinates, 3 for vertex normal x,y,z
	float* model1 = new float[numLines];

	//printf("num_tri %d, lines %d, size of vertexInd %d, textureInd %d, normalInd %d\n", num_tri, numLines, vertex_ind.size(), texture_ind.size(), normal_ind.size());
	//printf("size of vertexArr %d, size of textureArr %d, size of normalArr %d\n", vertex_arr.size(), texture_arr.size(), normal_arr.size());
	//printf("max index in indexInd %d\n", *max_element(vertex_ind.begin(), vertex_ind.end()));
	//printf("max index in textureInd %d\n", *max_element(texture_ind.begin(), texture_ind.end()));
	//printf("max index in normalInd %d\n", *max_element(normal_ind.begin(), normal_ind.end()));

	// for each vertex
	for (int i = 0; i < vertex_ind.size(); i++) {
		// vertex x,y,z
		model1[(i * 8)] =     vertex_arr[vertex_ind[i]-1].x;
		model1[(i * 8) + 1] = vertex_arr[vertex_ind[i]-1].y;
		model1[(i * 8) + 2] = vertex_arr[vertex_ind[i]-1].z;
		// texture map u,v
		model1[(i * 8) + 3] = texture_arr[texture_ind[i]-1].x;
		model1[(i * 8) + 4] = texture_arr[texture_ind[i]-1].y;
		// vertex normal x,y,z
		model1[(i * 8) + 5] = normal_arr[normal_ind[i]-1].x;
		model1[(i * 8) + 6] = normal_arr[normal_ind[i]-1].y;
		model1[(i * 8) + 7] = normal_arr[normal_ind[i]-1].z;
	}
	printf("nice, successful obj loaded\n");
	return model1;
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
	bool verbose = true;
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

	char curr_map = map_data.data[(int)( (ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1) )];
	char right_map = map_data.data[(int)( (ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1 ))-1];  // hitting cube on the right side of it is ind-1
	char left_map = map_data.data[(int)( (ceil(z + char_radius) - 1) * map_data.width + (ceil(x + char_radius) - 1 ))+1];  // hitting cube on the left side of it is ind+1
	char top_map = map_data.data[(int)( (ceil(z + char_radius)) * map_data.width + (ceil(x + char_radius) - 1) )];  // hitting cube on the top side of it is ind-map_data.width
	char bottom_map = map_data.data[(int)( (ceil(z + char_radius) - 2) * map_data.width + (ceil(x + char_radius) - 1) )];  // hitting cube on the bottom side of it is ind+map_data.width

	//string all(1, curr_map);
	//all.append(1, right_map); all.append(1, left_map); all.append(1, top_map); all.append(1, bottom_map);
	//int count = 0;
	//for (auto c : all) {
	//	if (isDoor(c) || isWall(c)) count++;
	//}

	if (verbose) printf("r %c   l %c   t %c   b %c   ", right_map, left_map, top_map, bottom_map);
	// don't move if you are on a blocked tile and or if there are 2 blocked things
	if ( !(isDoor(curr_map) || isWall(curr_map)) ) {
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
