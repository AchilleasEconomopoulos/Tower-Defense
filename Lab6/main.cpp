#include "SDL2/SDL.h"
#include <iostream>
#include <chrono>
#include "GLEW\glew.h"
#include "Renderer.h"
#include <string>
#include <thread>         // std::this_thread::sleep_for

using namespace std;

//Screen attributes
SDL_Window * window;

//OpenGL context 
SDL_GLContext gContext;
const int SCREEN_WIDTH = 1380;	//800;	//640;
const int SCREEN_HEIGHT = 1024;	//600;	//480;

//Event handler
SDL_Event event;

Renderer * renderer = nullptr;

void func()
{
	system("pause");
}

// initialize SDL and OpenGL
bool init()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		return false;
	}

	// use Double Buffering
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0)
		cout << "Error: No double buffering" << endl;

	// set OpenGL Version (3.3)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// Create Window
	window = SDL_CreateWindow("Tower Defence", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == NULL)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return false;
	}

	//Create OpenGL context
	gContext = SDL_GL_CreateContext(window);
	if (gContext == NULL)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	// Disable Vsync
	if (SDL_GL_SetSwapInterval(0) == -1)
		printf("Warning: Unable to disable VSync! SDL Error: %s\n", SDL_GetError());

	// Initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("Error loading GLEW\n");
		return false;
	}
	// some versions of glew may cause an opengl error in initialization
	glGetError();

	renderer = new Renderer();
	bool engine_initialized = renderer->Init(SCREEN_WIDTH, SCREEN_HEIGHT);

	//atexit(func);
	
	return engine_initialized;
}


void clean_up()
{
	delete renderer;

	SDL_GL_DeleteContext(gContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int main(int argc, char *argv[])
{

	srand(static_cast <unsigned> (time(0)));

	//Initialize
	if (init() == false)
	{
		system("pause");
		return EXIT_FAILURE;
	}

	//Quit flag
	bool quit = false;
	bool mouse_button_pressed = false;
	bool key_pressed = false;
	bool key2 = false;
	glm::vec2 prev_mouse_position(0);

	bool oneWave = false;
	bool twoWaves = false;

	float m_continous_time = 0;
	float lastWave = 0.0;
	float lastTower = 0.0;
	float lastRemovalGiven = 0.0;

	int waveInterval = 2;
	int towerInterval = 10;
	int removalInterval = 7;

	int totalWaves = 12;
	int currentWave = 1;


	auto simulation_start = chrono::steady_clock::now();

	// Wait for user exit
	while (quit == false)
	{
		// While there are events to handle
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				quit = true;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				// Key down events
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
				if (event.key.keysym.sym == SDLK_u) renderer->ReloadShaders();
				else if (event.key.keysym.sym == SDLK_w )
				{
					renderer->CameraMoveForward(true);
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					renderer->CameraMoveBackWard(true);
				}	
				else if (event.key.keysym.sym == SDLK_a)
				{
					renderer->CameraMoveLeft(true);
				}
				else if (event.key.keysym.sym == SDLK_d)
				{
					renderer->CameraMoveRight(true);
				}
				else if (event.key.keysym.sym == SDLK_UP)
				{
					if (!key_pressed) {
						renderer->gpMoveForward(true);
						key_pressed = true;
						renderer->currentAction(Renderer::TILE::SELECT);
					}
					else {
						renderer->gpMoveForward(false);
					}
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					if (!key_pressed) {
						renderer->gpMoveBackWard(true);
						key_pressed = true;
						renderer->currentAction(Renderer::TILE::SELECT);

					}
					else {
						renderer->gpMoveBackWard(false);
					}
				}
				else if (event.key.keysym.sym == SDLK_LEFT)
				{
					if (!key_pressed) {
						renderer->gpMoveLeft(true);
						key_pressed = true;
						renderer->currentAction(Renderer::TILE::SELECT);
					}
					else {
						renderer->gpMoveLeft(false);
					}
				}
				else if (event.key.keysym.sym == SDLK_RIGHT)
				{
					if (!key_pressed) {
						renderer->gpMoveRight(true);
						key_pressed = true;
						renderer->currentAction(Renderer::TILE::SELECT);
					}
					else {
						renderer->gpMoveRight(false);
					}
				}
				else if (event.key.keysym.sym == SDLK_t)
				{
					if (!key2) {
						key2 = true;
						renderer->placeTower();
					}
						
				}
				else if (event.key.keysym.sym == SDLK_r)
				{
					if (!key2) {
						key2 = true;
						renderer->removeTower();
					}
						
				}

			}
			else if (event.type == SDL_KEYUP)
			{
				if (event.key.keysym.sym == SDLK_w)
				{
					renderer->CameraMoveForward(false);
				}
				else if (event.key.keysym.sym == SDLK_s)
				{
					renderer->CameraMoveBackWard(false);
				}
				else if (event.key.keysym.sym == SDLK_a)
				{
					renderer->CameraMoveLeft(false);
				}
				else if (event.key.keysym.sym == SDLK_d)
				{
					renderer->CameraMoveRight(false);
				}
				if (event.key.keysym.sym == SDLK_UP)
				{
					key_pressed = false;
				}
				else if (event.key.keysym.sym == SDLK_DOWN)
				{
					key_pressed = false;
				}
				else if (event.key.keysym.sym == SDLK_LEFT)
				{
					key_pressed = false;
				}
				else if (event.key.keysym.sym == SDLK_RIGHT)
				{
					key_pressed = false;
				}

				if (event.key.keysym.sym == SDLK_t)
				{
					key2 = false;

				}else if (event.key.keysym.sym == SDLK_r)
				{
					key2 = false;
				}
			}
			else if (event.type == SDL_MOUSEMOTION)
			{
				int x = event.motion.x;
				int y = event.motion.y;
				if (mouse_button_pressed)
				{
					renderer->CameraLook(glm::vec2(x, y) - prev_mouse_position);
					prev_mouse_position = glm::vec2(x, y);
				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
			{
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					int x = event.button.x;
					int y = event.button.y;
					mouse_button_pressed = (event.type == SDL_MOUSEBUTTONDOWN);
					prev_mouse_position = glm::vec2(x, y);
				}
			}
			else if (event.type == SDL_MOUSEWHEEL)
			{
				int x = event.wheel.x;
				int y = event.wheel.y;
			}
			else if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					renderer->ResizeBuffers(event.window.data1, event.window.data2);
				}
			}
		}

		// Compute the ellapsed time
		auto simulation_end = chrono::steady_clock::now();
		float dt = chrono::duration <float>(simulation_end - simulation_start).count(); // in seconds
		simulation_start = chrono::steady_clock::now();

		m_continous_time += dt;

		if (!(currentWave > totalWaves)) {
			if (lastWave == 0.0 || m_continous_time - lastWave > waveInterval) {
				renderer->addPirateWave(currentWave*2);
				lastWave = m_continous_time;
				currentWave++;
			}
		}
		
		if(!((currentWave > totalWaves) && renderer->isBoardEmpty())){
			if (m_continous_time - lastTower > towerInterval) {
				renderer->giveTower();
				lastTower = m_continous_time;
			}

			if (m_continous_time - lastRemovalGiven > removalInterval) {
				renderer->addRemoval();
				lastRemovalGiven = m_continous_time;
			}

			// Update
			renderer->Update(dt);

			// Draw
			renderer->Render();

			quit = renderer->getGameOver();
		}
		else {
			quit = true;
		}
		
		//Update screen (swap buffer for double buffering)
		SDL_GL_SwapWindow(window);
	}

	//Clean up
	clean_up();

	return 0;
}
