#include "Renderer.h"
#include "GeometryNode.h"
#include "Tools.h"
#include <algorithm>
#include "ShaderProgram.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "OBJLoader.h"
#include <iostream>

// RENDERER
Renderer::Renderer()
{	
	selection = TILE::SELECT;
	m_vbo_fbo_vertices = 0;
	m_vao_fbo = 0;

	m_fbo = 0;
	m_fbo_texture = 0;


	m_terrain = nullptr;
	m_road = nullptr;
	m_treasure_chest = nullptr;
	m_green_plane = nullptr;
	m_red_plane = nullptr;

	m_rendering_mode = RENDERING_MODE::TRIANGLES;	
	m_continous_time = 0.0;
	m_camera_position = glm::vec3(0.720552, 18.1377, -11.3135);
	m_camera_target_position = glm::vec3(4.005, 12.634, -5.66336);
	m_camera_up_vector = glm::vec3(0, 1, 0);


	m_selection_position = glm::vec3(0, 0, 0);

	m_pirate_render = std::vector<bool>(1);


	InitializeArrays();

	

	boardIsEmpty = true;
	gameOver = false;
	available_towers = 3;
	removals_remaining = 0;

}

Renderer::~Renderer()
{

	glDeleteTextures(1, &m_fbo_texture);
	glDeleteFramebuffers(1, &m_fbo);

	glDeleteVertexArrays(1, &m_vao_fbo);
	glDeleteBuffers(1, &m_vbo_fbo_vertices);


	delete m_terrain;
	delete m_road;
	delete m_treasure_chest;
	delete m_green_plane;
	delete m_red_plane;
	delete m_tower;
	delete m_cannonball;
	delete m_pirate_body;
	delete m_pirate_rarm;
	delete m_pirate_lfoot;
	delete m_pirate_rfoot;


	
}

bool Renderer::Init(int SCREEN_WIDTH, int SCREEN_HEIGHT)
{
	this->m_screen_width = SCREEN_WIDTH;
	this->m_screen_height = SCREEN_HEIGHT;

	// Initialize OpenGL functions

	//Set clear color
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	//This enables depth and stencil testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);
	// glClearDepth sets the value the depth buffer is set at, when we clear it

	// Enable back face culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// open the viewport
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); //we set up our viewport

	bool techniques_initialization = InitRenderingTechniques();
	bool buffers_initialization = InitIntermediateShaderBuffers();
	bool items_initialization = InitCommonItems();
	bool lights_sources_initialization = InitLightSources();
	bool meshes_initialization = InitGeometricMeshes();

	//If there was any errors
	if (Tools::CheckGLError() != GL_NO_ERROR)
	{
		printf("Exiting with error at Renderer::Init\n");
		return false;
	}

	//If everything initialized
	return techniques_initialization && items_initialization && buffers_initialization;
}

void Renderer::Update(float dt)
{

	if (m_pirate_spawntimes.size() == 0)
		boardIsEmpty = true;

	float movement_speed = 10.0f;
	glm::vec3 direction = glm::normalize(m_camera_target_position - m_camera_position);

	m_camera_position += m_camera_movement.x *  movement_speed * direction * 0.005f;
	m_camera_target_position += m_camera_movement.x * movement_speed * direction * 0.005f;

	glm::vec3 right = glm::normalize(glm::cross(direction, m_camera_up_vector));
	m_camera_position += m_camera_movement.y *  movement_speed * right * 0.005f;
	m_camera_target_position += m_camera_movement.y * movement_speed * right * 0.005f;

	glm::mat4 rotation = glm::mat4(1.0f);
	float angular_speed = glm::pi<float>() * 0.0025f;

	rotation *= glm::rotate(glm::mat4(1.0), m_camera_look_angle_destination.y * angular_speed, right);
	rotation *= glm::rotate(glm::mat4(1.0), -m_camera_look_angle_destination.x  * angular_speed, m_camera_up_vector);
	m_camera_look_angle_destination = glm::vec2(0);

	direction = rotation * glm::vec4(direction, 0);
	float dist = glm::distance(m_camera_position, m_camera_target_position);
	m_camera_target_position = m_camera_position + direction * dist;

	m_view_matrix = glm::lookAt(m_camera_position, m_camera_target_position, m_camera_up_vector);

	m_continous_time += dt;

	m_terrain_transformation_matrix = glm::scale(glm::mat4(1.f), glm::vec3(20, 1, 20));
	m_terrain_transformation_matrix *= glm::translate(glm::mat4(1.f), glm::vec3(1, -2.5, 1));
	m_terrain_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_terrain_transformation_matrix))));

	for (int i = 0; i < 30; i++) {

	m_road_transformation_matrix[i] = glm::scale(glm::mat4(1.f), glm::vec3(2, 1, 2));
	m_road_transformation_matrix[i] *= glm::translate(glm::mat4(1.f), glm::vec3(2* m_tile_positions[i].x+1, -2.49, 2*m_tile_positions[i].y+1));
	m_road_transformation_normal_matrix[i] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_road_transformation_matrix[i]))));
	}

	if (m_treasure_chest_exists[0]) {
		m_treasure_chest_transformation_matrix[0] = glm::translate(glm::mat4(1.f), m_treasure_chest_positions[0]);
		m_treasure_chest_transformation_matrix[0] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
		m_treasure_chest_transformation_matrix[0] *= glm::translate(glm::mat4(1.f), glm::vec3(0.1760, -0.0226, 8.0619));
		m_treasure_chest_transformation_normal_matrix[0] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_treasure_chest_transformation_matrix[0]))));
	}

	if(m_treasure_chest_exists[1]) {
		m_treasure_chest_transformation_matrix[1] = glm::translate(glm::mat4(1.f), m_treasure_chest_positions[1]);
		m_treasure_chest_transformation_matrix[1] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
		m_treasure_chest_transformation_matrix[1] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
		m_treasure_chest_transformation_matrix[1] *= glm::translate(glm::mat4(1.f), glm::vec3(0.1760, -0.0226, 8.0619));
		m_treasure_chest_transformation_normal_matrix[1] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_treasure_chest_transformation_matrix[1]))));
	}

	if (m_treasure_chest_exists[2]) {
		m_treasure_chest_transformation_matrix[2] = glm::translate(glm::mat4(1.f), m_treasure_chest_positions[2]);
		m_treasure_chest_transformation_matrix[2] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
		m_treasure_chest_transformation_matrix[2] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
		m_treasure_chest_transformation_matrix[2] *= glm::translate(glm::mat4(1.f), glm::vec3(0.1760, -0.0226, 8.0619));
		m_treasure_chest_transformation_normal_matrix[2] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_treasure_chest_transformation_matrix[2]))));
	}
	

	glm::mat4 standard_tower = glm::scale(glm::mat4(1.f), glm::vec3(0.4))
							 * glm::translate(glm::mat4(1.f), glm::vec3(2.6035, 0.0626, 2.6373));

	m_tower_transformation_matrix = glm::translate(glm::mat4(1.f), m_selection_position + glm::vec3(1, -2.48, 1)) * standard_tower;

	m_tower_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_tower_transformation_matrix))));


	int pirateCount = m_pirate_body_transformation_matrix.size();
	movePirates(pirateCount);

	if (!boardIsEmpty) {
		for (int i = 0; i < m_last_shots.size(); i++) {
			if (m_last_shots[i] == 0.0)
				shootCannonballs(i);
		}
		updateCannonballs();
	}
	
	
	m_green_plane_transformation_matrix = glm::translate(glm::mat4(1.f), m_selection_position) * glm::scale(glm::mat4(1.f), glm::vec3(2, 1, 2));
	m_green_plane_transformation_matrix *= glm::translate(glm::mat4(1.f), glm::vec3(1, -2.48, 1));
	m_green_plane_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_green_plane_transformation_matrix))));

	m_red_plane_transformation_matrix = glm::translate(glm::mat4(1.f), m_selection_position) * glm::scale(glm::mat4(1.f), glm::vec3(2, 1, 2));
	m_red_plane_transformation_matrix *= glm::translate(glm::mat4(1.f), glm::vec3(1, -2.48, 1));
	m_red_plane_transformation_normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_red_plane_transformation_matrix))));


}

bool Renderer::InitCommonItems()
{
	glGenVertexArrays(1, &m_vao_fbo);
	glBindVertexArray(m_vao_fbo);

	GLfloat fbo_vertices[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1,
	};

	glGenBuffers(1, &m_vbo_fbo_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_fbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fbo_vertices), fbo_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);

	return true;


	return true;
}

bool Renderer::InitRenderingTechniques()
{
	bool initialized = true;


	//Basic Geometry Rendering
	std::string vertex_shader_path = "../Data/Shaders/basic_rendering.vert";
	std::string fragment_shader_path = "../Data/Shaders/basic_rendering.frag";
	m_basic_geometry_rendering_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_basic_geometry_rendering_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	initialized = m_basic_geometry_rendering_program.CreateProgram();
	m_basic_geometry_rendering_program.LoadUniform("uniform_projection_matrix");
	m_basic_geometry_rendering_program.LoadUniform("uniform_view_matrix");
	m_basic_geometry_rendering_program.LoadUniform("uniform_model_matrix");
	m_basic_geometry_rendering_program.LoadUniform("uniform_texture");
	m_basic_geometry_rendering_program.LoadUniform("uniform_color");



	//Geometry Rendering with Illumination + Shadows
	vertex_shader_path = "../Data/Shaders/basic_shadowed_rendering.vert";
	fragment_shader_path = "../Data/Shaders/basic_shadowed_rendering.frag";
	m_shadowed_geometry_rendering_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_shadowed_geometry_rendering_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	initialized = m_shadowed_geometry_rendering_program.CreateProgram();
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_projection_matrix");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_view_matrix");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_model_matrix");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_normal_matrix");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_diffuse");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_specular");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_shininess");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_has_texture");
	m_shadowed_geometry_rendering_program.LoadUniform("diffuse_texture");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_camera_position");
	// Light Source Uniforms
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_projection_matrix");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_view_matrix");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_position");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_direction");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_color");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_umbra");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_light_penumbra");
	m_shadowed_geometry_rendering_program.LoadUniform("uniform_cast_shadows");
	m_shadowed_geometry_rendering_program.LoadUniform("shadowmap_texture");

	// Post Processing Program
	vertex_shader_path = "../Data/Shaders/postproc.vert";
	fragment_shader_path = "../Data/Shaders/postproc.frag";
	m_postprocess_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_postprocess_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	initialized = initialized && m_postprocess_program.CreateProgram();
	m_postprocess_program.LoadUniform("uniform_texture");
	m_postprocess_program.LoadUniform("uniform_time");
	m_postprocess_program.LoadUniform("uniform_depth");
	m_postprocess_program.LoadUniform("uniform_projection_inverse_matrix");

	// Shadow mapping Program
	vertex_shader_path = "../Data/Shaders/shadow_map_rendering.vert";
	fragment_shader_path = "../Data/Shaders/shadow_map_rendering.frag";
	m_spot_light_shadow_map_program.LoadVertexShaderFromFile(vertex_shader_path.c_str());
	m_spot_light_shadow_map_program.LoadFragmentShaderFromFile(fragment_shader_path.c_str());
	initialized = initialized && m_spot_light_shadow_map_program.CreateProgram();
	m_spot_light_shadow_map_program.LoadUniform("uniform_projection_matrix");
	m_spot_light_shadow_map_program.LoadUniform("uniform_view_matrix");
	m_spot_light_shadow_map_program.LoadUniform("uniform_model_matrix");


	return initialized;
}

bool Renderer::ReloadShaders()
{
	bool reloaded = true;
	// rendering techniques
	reloaded = reloaded && m_shadowed_geometry_rendering_program.ReloadProgram();
	reloaded = reloaded && m_postprocess_program.ReloadProgram();
	reloaded = reloaded && m_spot_light_shadow_map_program.ReloadProgram();

	return reloaded;
}

bool Renderer::InitIntermediateShaderBuffers()
{
	// generate texture handles 
	glGenTextures(1, &m_fbo_texture);
	glGenTextures(1, &m_fbo_depth_texture);

	// framebuffer to link everything together
	glGenFramebuffers(1, &m_fbo);

	return ResizeBuffers(m_screen_width, m_screen_height);
}

bool Renderer::ResizeBuffers(int width, int height)
{
	m_screen_width = width;
	m_screen_height = height;

	// texture
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_screen_width, m_screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// depth texture
	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_screen_width, m_screen_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// framebuffer to link to everything together
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_fbo_depth_texture, 0);

	GLenum status = Tools::CheckFramebufferStatus(m_fbo);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_projection_matrix = glm::perspective(glm::radians(60.f), width / (float)height, 0.1f, 1500.0f);
	m_view_matrix = glm::lookAt(m_camera_position, m_camera_target_position, m_camera_up_vector);

	return true;
}

bool Renderer::InitLightSources()
{
	m_spotlight_node.SetPosition(glm::vec3(16, 30, 16));
	m_spotlight_node.SetTarget(glm::vec3(16.4, 0, 16));

	m_spotlight_node.SetColor(200.0f * glm::vec3(140, 140, 140) / 255.f);
	m_spotlight_node.SetConeSize(73, 80);
	m_spotlight_node.CastShadow(true);

	return true;
}

bool Renderer::InitGeometricMeshes()
{
	bool initialized = true;
	OBJLoader loader;
	// load geometric object 1
	auto mesh = loader.load("../Data/Terrain/terrain.obj");

	if (mesh != nullptr)
	{
		m_terrain = new GeometryNode();
		m_terrain->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;

	mesh = loader.load("../Data/Terrain/road.obj");
	if (mesh != nullptr)
	{
		m_road = new GeometryNode();
		m_road->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;

	mesh = loader.load("../Data/Treasure/treasure_chest.obj");

	if (mesh != nullptr)
	{
		m_treasure_chest = new GeometryNode;
		m_treasure_chest->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;


	mesh = loader.load("../Data/Various/plane_green.obj");

	if (mesh != nullptr)
	{
		m_green_plane = new GeometryNode();
		m_green_plane->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;

	mesh = loader.load("../Data/Various/plane_red.obj");

	if (mesh != nullptr)
	{
		m_red_plane = new GeometryNode();
		m_red_plane->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;

	mesh = loader.load("../Data/MedievalTower/tower.obj");

	if (mesh != nullptr)
	{
		m_tower = new GeometryNode();
		m_tower->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;

	mesh = loader.load("../Data/Various/cannonball.obj");

	if (mesh != nullptr)
	{
		m_cannonball = new GeometryNode();
		m_cannonball->Init(mesh);
	}
	else
		initialized = false;
	delete mesh;

	mesh = loader.load("../Data/Pirate/pirate_body.obj");
	if (mesh != nullptr)
	{
		m_pirate_body = new GeometryNode();
		m_pirate_body->Init(mesh);
	}
	else
		initialized = false;

	mesh = loader.load("../Data/Pirate/pirate_arm.obj");
	if (mesh != nullptr)
	{
		m_pirate_rarm = new GeometryNode();
		m_pirate_rarm->Init(mesh);
	}
	else
		initialized = false;

	mesh = loader.load("../Data/Pirate/pirate_right_foot.obj");
	if (mesh != nullptr)
	{
		m_pirate_rfoot = new GeometryNode();
		m_pirate_rfoot->Init(mesh);
	}
	else
		initialized = false;

	mesh = loader.load("../Data/Pirate/pirate_left_foot.obj");
	if (mesh != nullptr)
	{
		m_pirate_lfoot = new GeometryNode();
		m_pirate_lfoot->Init(mesh);
	}
	else
		initialized = false;

	return initialized;
}

void Renderer::SetRenderingMode(RENDERING_MODE mode)
{
	m_rendering_mode = mode;
}

void Renderer::Render()
{
	RenderShadowMaps();

	// Draw the geometry
	RenderGeometry();

	RenderToOutFB();

	GLenum error = Tools::CheckGLError();
	if (error != GL_NO_ERROR)
	{
		printf("Reanderer:Draw GL Error\n");
		system("pause");
	}
}

void Renderer::RenderShadowMaps()
{
	// if the light source casts shadows
	if (m_spotlight_node.GetCastShadowsStatus())
	{
		int m_depth_texture_resolution = m_spotlight_node.GetShadowMapResolution();

		// bind the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_spotlight_node.GetShadowMapFBO());
		glViewport(0, 0, m_depth_texture_resolution, m_depth_texture_resolution);
		GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawbuffers);

		// clear depth buffer
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// Bind the shadow mapping program
		m_spot_light_shadow_map_program.Bind();

		// pass the projection and view matrix to the uniforms
		glUniformMatrix4fv(m_spot_light_shadow_map_program["uniform_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetProjectionMatrix()));
		glUniformMatrix4fv(m_spot_light_shadow_map_program["uniform_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetViewMatrix()));

		//Terrain
		DrawGeometryNodeToShadowMap(m_terrain, m_terrain_transformation_matrix, m_terrain_transformation_normal_matrix);

		//Road tiles
		for (int i = 0; i < m_road_transformation_matrix.size(); i++) {
			DrawGeometryNodeToShadowMap(m_road, m_road_transformation_matrix[i], m_road_transformation_normal_matrix[i]);
		}

		//Treasure chests
		for (int i = 0; i < m_treasure_chest_transformation_matrix.size(); i++) {
			DrawGeometryNodeToShadowMap(m_treasure_chest, m_treasure_chest_transformation_matrix[i], m_treasure_chest_transformation_normal_matrix[i]);
		}

		//Cannonballs

		for (int i = 0; i < m_cannonball_transformation_matrix.size(); i++) {
			if (m_cannonball_render[i])
				DrawGeometryNodeToShadowMap(m_cannonball, m_cannonball_transformation_matrix[i], m_cannonball_transformation_normal_matrix[i]);
		}

		//Towers
		for (std::vector<glm::vec2>::iterator pos_it = m_placed_towers.begin(); pos_it != m_placed_towers.end(); ++pos_it) {

			glm::mat4 model_matrix = glm::translate(glm::mat4(1.f), glm::vec3((*pos_it).x + 2, -2.47, (*pos_it).y + 2)) *glm::scale(glm::mat4(1.f), glm::vec3(0.4))
				* glm::translate(glm::mat4(1.f), glm::vec3(0.0101, 0.0626, 0.0758));

			DrawGeometryNodeToShadowMap(m_tower, model_matrix, m_tower_transformation_normal_matrix);
		}

		//Pirates
		for (int i = 0; i < m_pirate_body_transformation_matrix.size(); i++) {
			if (m_pirate_render[i]) {
				DrawGeometryNodeToShadowMap(m_pirate_body, m_pirate_body_transformation_matrix[i], m_pirate_body_transformation_normal_matrix[i]);
				DrawGeometryNodeToShadowMap(m_pirate_rarm, m_pirate_rarm_transformation_matrix[i], m_pirate_rarm_transformation_normal_matrix[i]);
				DrawGeometryNodeToShadowMap(m_pirate_lfoot, m_pirate_lfoot_transformation_matrix[i], m_pirate_lfoot_transformation_normal_matrix[i]);
				DrawGeometryNodeToShadowMap(m_pirate_rfoot, m_pirate_rfoot_transformation_matrix[i], m_pirate_rfoot_transformation_normal_matrix[i]);
			}
		}

		glBindVertexArray(0);

		// Unbind shadow mapping program
		m_spot_light_shadow_map_program.Unbind();


		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}


void Renderer::RenderGeometry()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glViewport(0, 0, m_screen_width, m_screen_height);
	GLenum drawbuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawbuffers);

	// clear color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// render only points, lines, triangles
	switch (m_rendering_mode)
	{
	case RENDERING_MODE::TRIANGLES:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case RENDERING_MODE::LINES:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case RENDERING_MODE::POINTS:
		glPointSize(2);
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	};

	// Bind the shader program
	m_shadowed_geometry_rendering_program.Bind();

	// pass the camera properties
	glUniformMatrix4fv(m_shadowed_geometry_rendering_program["uniform_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_projection_matrix));
	glUniformMatrix4fv(m_shadowed_geometry_rendering_program["uniform_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_view_matrix));
	glUniform3f(m_shadowed_geometry_rendering_program["uniform_camera_position"], m_camera_position.x, m_camera_position.y, m_camera_position.z);

	// pass the light source parameters
	glm::vec3 light_position = m_spotlight_node.GetPosition();
	glm::vec3 light_direction = m_spotlight_node.GetDirection();
	glm::vec3 light_color = m_spotlight_node.GetColor();
	glUniformMatrix4fv(m_shadowed_geometry_rendering_program["uniform_light_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetProjectionMatrix()));
	glUniformMatrix4fv(m_shadowed_geometry_rendering_program["uniform_light_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_spotlight_node.GetViewMatrix()));
	glUniform3f(m_shadowed_geometry_rendering_program["uniform_light_position"], light_position.x, light_position.y, light_position.z);
	glUniform3f(m_shadowed_geometry_rendering_program["uniform_light_direction"], light_direction.x, light_direction.y, light_direction.z);
	glUniform3f(m_shadowed_geometry_rendering_program["uniform_light_color"], light_color.x, light_color.y, light_color.z);
	glUniform1f(m_shadowed_geometry_rendering_program["uniform_light_umbra"], m_spotlight_node.GetUmbra());
	glUniform1f(m_shadowed_geometry_rendering_program["uniform_light_penumbra"], m_spotlight_node.GetPenumbra());
	glUniform1i(m_shadowed_geometry_rendering_program["uniform_cast_shadows"], (m_spotlight_node.GetCastShadowsStatus()) ? 1 : 0);

	// Set the sampler2D uniform to use texture unit 1
	glUniform1i(m_shadowed_geometry_rendering_program["shadowmap_texture"], 1);
	// Bind the shadow map texture to texture unit 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, (m_spotlight_node.GetCastShadowsStatus()) ? m_spotlight_node.GetShadowMapDepthTexture() : 0);

	// Enable Texture Unit 0
	glUniform1i(m_shadowed_geometry_rendering_program["diffuse_texture"], 0);
	glActiveTexture(GL_TEXTURE0);

	//Terrain
	DrawGeometryNode(m_terrain, m_terrain_transformation_matrix, m_terrain_transformation_normal_matrix);

	//Road tiles
	for (int i = 0; i < m_road_transformation_matrix.size(); i++) {
		DrawGeometryNode(m_road, m_road_transformation_matrix[i], m_road_transformation_normal_matrix[i]);
	}

	//Treasure chests
	for (int i = 0; i < m_treasure_chest_transformation_matrix.size(); i++) {
		DrawGeometryNode(m_treasure_chest, m_treasure_chest_transformation_matrix[i], m_treasure_chest_transformation_normal_matrix[i]);
	}

	//Cannonballs
	for (int i = 0; i < m_cannonball_positions.size();i++) {
		if (m_cannonball_render[i])
			DrawGeometryNode(m_cannonball, m_cannonball_transformation_matrix[i], m_cannonball_transformation_normal_matrix[i]);
	}

	//Towers
	for (std::vector<glm::vec2>::iterator pos_it  = m_placed_towers.begin(); pos_it != m_placed_towers.end(); ++pos_it) {

		glm::mat4 model_matrix = glm::translate(glm::mat4(1.f), glm::vec3((*pos_it).x + 1, -2.47, (*pos_it).y + 1)) *glm::scale(glm::mat4(1.f), glm::vec3(0.4))
			* glm::translate(glm::mat4(1.f), glm::vec3(2.6035, 0.0626, 2.6373));

		glm::mat4 normal_matrix = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model_matrix))));

		DrawGeometryNode(m_tower, model_matrix, normal_matrix);
	}

	//Pirates

	for (int i = 0; i < m_pirate_body_transformation_matrix.size();i++) {
		if (m_pirate_render[i]) {
			DrawGeometryNode(m_pirate_body, m_pirate_body_transformation_matrix[i], m_pirate_body_transformation_normal_matrix[i]);
			DrawGeometryNode(m_pirate_rarm, m_pirate_rarm_transformation_matrix[i], m_pirate_rarm_transformation_normal_matrix[i]);
			DrawGeometryNode(m_pirate_lfoot, m_pirate_lfoot_transformation_matrix[i], m_pirate_lfoot_transformation_normal_matrix[i]);
			DrawGeometryNode(m_pirate_rfoot, m_pirate_rfoot_transformation_matrix[i], m_pirate_rfoot_transformation_normal_matrix[i]);
		}
	}

	// unbind the vao
	glBindVertexArray(0);
	// unbind the shader program
	m_shadowed_geometry_rendering_program.Unbind();


	
		//RENDERING WITHOUT SHADOWS OR ILLUMINATION
	m_basic_geometry_rendering_program.Bind();

	glEnable(GL_BLEND);
	// blend using the alpha value of the fragment shader
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUniformMatrix4fv(m_basic_geometry_rendering_program["uniform_projection_matrix"], 1, GL_FALSE, glm::value_ptr(m_projection_matrix));
	glUniformMatrix4fv(m_basic_geometry_rendering_program["uniform_view_matrix"], 1, GL_FALSE, glm::value_ptr(m_view_matrix));

	glUniform1i(m_basic_geometry_rendering_program["uniform_texture"], 0);
	glActiveTexture(GL_TEXTURE0);

	switch (selection) {
	case TILE::RED:
		color = glm::vec3(0.f, 0.f, 0.f);

		glBindVertexArray(m_red_plane->m_vao);
		glUniformMatrix4fv(m_basic_geometry_rendering_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(m_red_plane_transformation_matrix));
		glUniform3f(m_basic_geometry_rendering_program["uniform_color"], color.r, color.g, color.b);
		for (int j = 0; j < m_red_plane->parts.size(); j++)
		{
			glBindTexture(GL_TEXTURE_2D, m_red_plane->parts[j].textureID);
			glDrawArrays(GL_TRIANGLES, m_red_plane->parts[j].start_offset, m_red_plane->parts[j].count);
		}
		break;


	case TILE::GREEN:

		color = glm::vec3(0.f, 0.f, 0.f);

		glBindVertexArray(m_green_plane->m_vao);
		glUniformMatrix4fv(m_basic_geometry_rendering_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(m_green_plane_transformation_matrix));
		glUniform3f(m_basic_geometry_rendering_program["uniform_color"], color.r, color.g, color.b);
		for (int j = 0; j < m_green_plane->parts.size(); j++)
		{
			glBindTexture(GL_TEXTURE_2D, m_green_plane->parts[j].textureID);
			glDrawArrays(GL_TRIANGLES, m_green_plane->parts[j].start_offset, m_green_plane->parts[j].count);
		}
		break;

	case TILE::SELECT:

		color = glm::vec3(1.f, 1.f, (float)204 / 255);

		glBindVertexArray(m_green_plane->m_vao);
		glUniformMatrix4fv(m_basic_geometry_rendering_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(m_green_plane_transformation_matrix));
		glUniform3f(m_basic_geometry_rendering_program["uniform_color"], color.r, color.g, color.b);
		for (int j = 0; j < m_green_plane->parts.size(); j++)
		{
			glBindTexture(GL_TEXTURE_2D, m_green_plane->parts[j].textureID);
			glDrawArrays(GL_TRIANGLES, m_green_plane->parts[j].start_offset, m_green_plane->parts[j].count);
		}
		break;
	}

	glDisable(GL_BLEND);

	glBindVertexArray(0);

	m_basic_geometry_rendering_program.Unbind();
	


	if (m_rendering_mode != RENDERING_MODE::TRIANGLES)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	glDisable(GL_DEPTH_TEST);
	glPointSize(1.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawGeometryNode(GeometryNode* node, glm::mat4 model_matrix, glm::mat4 normal_matrix)
{
	glBindVertexArray(node->m_vao);
	glUniformMatrix4fv(m_shadowed_geometry_rendering_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(model_matrix));
	glUniformMatrix4fv(m_shadowed_geometry_rendering_program["uniform_normal_matrix"], 1, GL_FALSE, glm::value_ptr(normal_matrix));
	for (int j = 0; j < node->parts.size(); j++)
	{
		glm::vec3 diffuseColor = node->parts[j].diffuseColor;
		glm::vec3 specularColor = node->parts[j].specularColor;
		float shininess = node->parts[j].shininess;
		glUniform3f(m_shadowed_geometry_rendering_program["uniform_diffuse"], diffuseColor.r, diffuseColor.g, diffuseColor.b);
		glUniform3f(m_shadowed_geometry_rendering_program["uniform_specular"], specularColor.r, specularColor.g, specularColor.b);
		glUniform1f(m_shadowed_geometry_rendering_program["uniform_shininess"], shininess);
		glUniform1f(m_shadowed_geometry_rendering_program["uniform_has_texture"], (node->parts[j].textureID > 0) ? 1.0f : 0.0f);
		glBindTexture(GL_TEXTURE_2D, node->parts[j].textureID);

		glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
	}
}


void Renderer::DrawGeometryNodeToShadowMap(GeometryNode* node, glm::mat4 model_matrix, glm::mat4 normal_matrix)
{
	glBindVertexArray(node->m_vao);
	glUniformMatrix4fv(m_spot_light_shadow_map_program["uniform_model_matrix"], 1, GL_FALSE, glm::value_ptr(model_matrix));
	for (int j = 0; j < node->parts.size(); j++)
	{
		glDrawArrays(GL_TRIANGLES, node->parts[j].start_offset, node->parts[j].count);
	}
}


void Renderer::RenderToOutFB()
{
	// Bind the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_screen_width, m_screen_height);

	// clear the color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// disable depth testing and blending
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// bind the post processing program
	m_postprocess_program.Bind();

	glBindVertexArray(m_vao_fbo);
	// Bind the intermediate color image to texture unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
	glUniform1i(m_postprocess_program["uniform_texture"], 0);
	// Bind the intermediate depth buffer to texture unit 1
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
	glUniform1i(m_postprocess_program["uniform_depth"], 1);

	glUniform1f(m_postprocess_program["uniform_time"], m_continous_time);
	glm::mat4 projection_inverse_matrix = glm::inverse(m_projection_matrix);
	glUniformMatrix4fv(m_postprocess_program["uniform_projection_inverse_matrix"], 1, GL_FALSE, glm::value_ptr(projection_inverse_matrix));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);

	// Unbind the post processing program
	m_postprocess_program.Unbind();
}

void Renderer::CameraMoveForward(bool enable)
{
	m_camera_movement.x = (enable)? 1.f : 0.f;
}
void Renderer::CameraMoveBackWard(bool enable)
{
	m_camera_movement.x = (enable) ? -1.f : 0.f;
}

void Renderer::CameraMoveLeft(bool enable)
{
	m_camera_movement.y = (enable) ? -1.f : 0.f;
}
void Renderer::CameraMoveRight(bool enable)
{
	m_camera_movement.y = (enable) ? 1.f : 0.f;
}

void Renderer::CameraLook(glm::vec2 lookDir)
{
	m_camera_look_angle_destination = glm::vec2(1, -1) * lookDir;
}

void Renderer::gpMoveForward(bool enable)
{
	m_selection_movement.x = (enable) ? 4.f : 0.f;

	if(m_selection_position.z<9*4.f)
		m_selection_position.z += m_selection_movement.x;
}
void Renderer::gpMoveBackWard(bool enable)
{
	m_selection_movement.x = (enable) ? -4.f : 0.f;

	if (m_selection_position.z>0 * 4.f)
		m_selection_position.z += m_selection_movement.x;
}

void Renderer::gpMoveLeft(bool enable)
{
	m_selection_movement.y = (enable) ? 4.f : 0.f;

	if (m_selection_position.x<9 * 4.f)
		m_selection_position.x += m_selection_movement.y;
}
void Renderer::gpMoveRight(bool enable)
{
	m_selection_movement.y = (enable) ? -4.f : 0.f;

	if (m_selection_position.x>0 * 4.f)
		m_selection_position.x += m_selection_movement.y;
}

void Renderer::currentAction(TILE tileColor) {
	selection = tileColor;
}

void Renderer::placeTower() {
	if (available_towers > 0) {
		glm::vec2 pos = glm::vec2(m_selection_position.x, m_selection_position.z);
		int index = Tools::vectorIndex(m_tower_positions, pos / glm::vec2(4));


		if (index == -1) {
			currentAction(TILE::RED);
		}
		else {
			if ((Tools::vectorIndex(m_tile_positions, pos / glm::vec2(4)) == -1) && (Tools::vectorIndex(m_placed_towers, pos) == -1)) {
				m_placed_towers.push_back(pos);

				m_last_shots.push_back(0.0);

				m_cannonball_positions.push_back(glm::vec3(1.f));
				m_cannonball_spawntimes.push_back(0.0);
				m_cannonball_render.push_back(false);
				m_shootAt.push_back(-1);
				m_cannonball_transformation_matrix.push_back(glm::mat4(1.f));
				m_cannonball_transformation_normal_matrix.push_back(glm::mat4(1.f));

				currentAction(TILE::GREEN);
				available_towers--;
			}
			else {
				currentAction(TILE::RED);
			}
		}
	}
	else {
		currentAction(TILE::RED);
	}
}

void Renderer::removeTower() {
	if (removals_remaining > 0) {
		glm::vec2 pos = glm::vec2(m_selection_position.x, m_selection_position.z);
		int index = Tools::vectorIndex(m_placed_towers, pos);
		if (index != -1) {
			m_placed_towers.erase(m_placed_towers.begin() + index);

			m_last_shots.erase(m_last_shots.begin() + index);

			m_cannonball_positions.erase(m_cannonball_positions.begin() + index);
			m_cannonball_spawntimes.erase(m_cannonball_spawntimes.begin() + index);
			m_cannonball_render.erase(m_cannonball_render.begin() + index);
			m_shootAt.erase(m_shootAt.begin() + index);
			m_cannonball_transformation_matrix.erase(m_cannonball_transformation_matrix.begin() + index);
			m_cannonball_transformation_normal_matrix.erase(m_cannonball_transformation_normal_matrix.begin() + index);

			currentAction(TILE::GREEN);
			removals_remaining--;
			available_towers++;
		}
		else {
			currentAction(TILE::RED);
		}
	}
	else {
		currentAction(TILE::RED);
	}
}


void Renderer::InitializeArrays() {

	m_road_transformation_matrix = std::vector<glm::mat4>(30);
	m_road_transformation_normal_matrix = std::vector<glm::mat4>(30);

	m_treasure_chest_transformation_matrix = std::vector<glm::mat4>(3);
	m_treasure_chest_transformation_normal_matrix = std::vector<glm::mat4>(3);


	m_tile_positions = std::vector<glm::vec2>(30);

	m_tile_positions[0] = glm::vec2(0, 0);
	m_tile_positions[1] = glm::vec2(0, 1);
	m_tile_positions[2] = glm::vec2(0, 2);
	m_tile_positions[3] = glm::vec2(0, 3);
	m_tile_positions[4] = glm::vec2(1, 3);
	m_tile_positions[5] = glm::vec2(1, 4);
	m_tile_positions[6] = glm::vec2(1, 5);
	m_tile_positions[7] = glm::vec2(1, 6);
	m_tile_positions[8] = glm::vec2(1, 7);
	m_tile_positions[9] = glm::vec2(2, 7);
	m_tile_positions[10] = glm::vec2(2, 8);
	m_tile_positions[11] = glm::vec2(3, 8);
	m_tile_positions[12] = glm::vec2(4, 8);
	m_tile_positions[13] = glm::vec2(5, 8);
	m_tile_positions[14] = glm::vec2(6, 8);
	m_tile_positions[15] = glm::vec2(6, 7);
	m_tile_positions[16] = glm::vec2(6, 6);
	m_tile_positions[17] = glm::vec2(7, 6);
	m_tile_positions[18] = glm::vec2(7, 5);
	m_tile_positions[19] = glm::vec2(7, 4);
	m_tile_positions[20] = glm::vec2(7, 3);
	m_tile_positions[21] = glm::vec2(8, 3);
	m_tile_positions[22] = glm::vec2(9, 3);
	m_tile_positions[23] = glm::vec2(9, 2);
	m_tile_positions[24] = glm::vec2(9, 1);
	m_tile_positions[25] = glm::vec2(8, 1);
	m_tile_positions[26] = glm::vec2(7, 1);
	m_tile_positions[27] = glm::vec2(6, 1);
	m_tile_positions[28] = glm::vec2(6, 0);
	m_tile_positions[29] = glm::vec2(6, -1);


	m_tower_positions = std::vector<glm::vec2>(38);

	m_tower_positions[0] = glm::vec2(0, 4);
	m_tower_positions[1] = glm::vec2(0, 5);
	m_tower_positions[2] = glm::vec2(0, 6);
	m_tower_positions[3] = glm::vec2(0, 7);
	m_tower_positions[4] = glm::vec2(1, 0);
	m_tower_positions[5] = glm::vec2(1, 1);
	m_tower_positions[6] = glm::vec2(1, 2);
	m_tower_positions[7] = glm::vec2(1, 8);
	m_tower_positions[8] = glm::vec2(2, 3);
	m_tower_positions[9] = glm::vec2(2, 4);
	m_tower_positions[10] = glm::vec2(2, 5);
	m_tower_positions[11] = glm::vec2(2, 6);
	m_tower_positions[12] = glm::vec2(2, 9);
	m_tower_positions[13] = glm::vec2(3, 7);
	m_tower_positions[14] = glm::vec2(3, 9);
	m_tower_positions[15] = glm::vec2(4, 7);
	m_tower_positions[16] = glm::vec2(4, 9);
	m_tower_positions[17] = glm::vec2(5, 0);
	m_tower_positions[18] = glm::vec2(5, 1);
	m_tower_positions[19] = glm::vec2(5, 6);
	m_tower_positions[20] = glm::vec2(5, 7);
	m_tower_positions[21] = glm::vec2(5, 9);
	m_tower_positions[22] = glm::vec2(6, 2);
	m_tower_positions[23] = glm::vec2(6, 3);
	m_tower_positions[24] = glm::vec2(6, 4);
	m_tower_positions[25] = glm::vec2(6, 5);
	m_tower_positions[26] = glm::vec2(6, 9);
	m_tower_positions[27] = glm::vec2(7, 0);
	m_tower_positions[28] = glm::vec2(7, 2);
	m_tower_positions[29] = glm::vec2(7, 7);
	m_tower_positions[30] = glm::vec2(7, 8);
	m_tower_positions[31] = glm::vec2(8, 0);
	m_tower_positions[32] = glm::vec2(8, 2);
	m_tower_positions[33] = glm::vec2(8, 4);
	m_tower_positions[34] = glm::vec2(8, 5);
	m_tower_positions[35] = glm::vec2(8, 6);
	m_tower_positions[36] = glm::vec2(9, 0);
	m_tower_positions[37] = glm::vec2(9, 4);


	m_treasure_chest_positions = std::vector<glm::vec3>(3);
	m_treasure_chest_positions[0] = glm::vec3(4 * m_tile_positions[29].x + 2.05, -2.48, 4 * m_tile_positions[29].y + 0.57995);
	m_treasure_chest_positions[1] = glm::vec3(4 * m_tile_positions[29].x + 0.57995, -2.48, 4 * m_tile_positions[29].y + 2.7);
	m_treasure_chest_positions[2] = glm::vec3(4 * m_tile_positions[29].x + 3.415, -2.48, 4 * m_tile_positions[29].y + 2.7);

	m_treasure_chest_coins.push_back(100);
	m_treasure_chest_coins.push_back(100);
	m_treasure_chest_coins.push_back(100);

	m_treasure_chest_exists.push_back(true);
	m_treasure_chest_exists.push_back(true);
	m_treasure_chest_exists.push_back(true);
}

//#define reallyRandom
#ifndef reallyRandom
	#define standardSpacing
#endif
void Renderer::addPirateWave(int life) {
	int l;
	int r1;
	std::vector<int> positions = { 0,1,2,3,4 };

	float r2;
	for (int i = 0; i < 5; i++) {


#ifdef reallyRandom
		r2 = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 5));
#endif

#ifdef standardSpacing
		l = (rand() % positions.size());
		r1 = positions[l];
		positions.erase(positions.begin() + l);
#endif

		m_pirate_body_transformation_matrix.push_back(glm::mat4(1.f));
		m_pirate_rarm_transformation_matrix.push_back(glm::mat4(1.f));
		m_pirate_lfoot_transformation_matrix.push_back(glm::mat4(1.f));
		m_pirate_rfoot_transformation_matrix.push_back(glm::mat4(1.f));

		m_pirate_body_transformation_normal_matrix.push_back(glm::mat4(1.f));
		m_pirate_rarm_transformation_normal_matrix.push_back(glm::mat4(1.f));
		m_pirate_lfoot_transformation_normal_matrix.push_back(glm::mat4(1.f));
		m_pirate_rfoot_transformation_normal_matrix.push_back(glm::mat4(1.f));

#ifdef reallyRandom
		m_pirate_spawntimes.push_back(m_continous_time + r2);
#endif

#ifdef standardSpacing
		m_pirate_spawntimes.push_back(m_continous_time + r1*0.5);
#endif
		m_pirate_lives.push_back(5 + life);

		
	}

	m_pirate_render = std::vector<bool>(m_pirate_spawntimes.size());
}

void Renderer::removePirate(int index) {

	for (int i = 0; i <m_shootAt.size(); i++) {
		if (index == m_shootAt[i]) {
			m_shootAt[i] = -1;
			m_cannonball_render[i] = false;
			m_last_shots[i] = 0.0;
		}
		else if (index < m_shootAt[i]) {
			m_shootAt[i]--;
		}
	}

	m_pirate_body_transformation_matrix.erase(m_pirate_body_transformation_matrix.begin() + index);
	m_pirate_rarm_transformation_matrix.erase(m_pirate_rarm_transformation_matrix.begin() + index);
	m_pirate_lfoot_transformation_matrix.erase(m_pirate_lfoot_transformation_matrix.begin() + index);
	m_pirate_rfoot_transformation_matrix.erase(m_pirate_rfoot_transformation_matrix.begin() + index);

	m_pirate_body_transformation_normal_matrix.erase(m_pirate_body_transformation_normal_matrix.begin() + index);
	m_pirate_rarm_transformation_normal_matrix.erase(m_pirate_rarm_transformation_normal_matrix.begin() + index);
	m_pirate_lfoot_transformation_normal_matrix.erase(m_pirate_lfoot_transformation_normal_matrix.begin() + index);
	m_pirate_rfoot_transformation_normal_matrix.erase(m_pirate_rfoot_transformation_normal_matrix.begin() + index);

	m_pirate_spawntimes.erase(m_pirate_spawntimes.begin() + index);
	m_pirate_positions.erase(m_pirate_positions.begin() + index);
	m_pirate_lives.erase(m_pirate_lives.begin() + index);
	m_pirate_render.erase(m_pirate_render.begin() + index);

	
}

void Renderer::movePirates(int pirateCount) {
	std::vector<glm::vec3> pirate_transformation_coords = std::vector<glm::vec3>(pirateCount);
	m_pirate_positions= std::vector<glm::vec3>(pirateCount);
	std::vector<bool> marked = std::vector<bool>(pirateCount);

	for (int index = 0; index < pirateCount; index++) {

		int i = glm::floor(m_continous_time - m_pirate_spawntimes[index]);

		if (i >= 0) {
			boardIsEmpty = false;
			m_pirate_render[index] = true;
			glm::mat4 tr = glm::mat4(1.f);

			switch (i) {
			case 3:
			case 8:
			case 10:

				pirate_transformation_coords[index] = glm::vec3(m_tile_positions[i].x * 4 + 2, -2.35, m_tile_positions[i].y * 4 );
				m_pirate_positions[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 2, -2.35, m_tile_positions[i].y * 4), glm::vec3(m_tile_positions[i + 1].x * 4, -2.35, m_tile_positions[i + 1].y * 4 + 2.0), m_continous_time - m_pirate_spawntimes[index] - i);

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4.5*0.09, 12 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4 * 0.09, 0, 2 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4 * 0.09, 0, 2 * 0.09));


				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2, 0, 0));

				m_pirate_body_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 + (-4.5*0.09), 12 * 0.09, 0));

				m_pirate_rarm_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 + (4 * 0.09), 0, 2 * 0.09));

				m_pirate_lfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 + (-4 * 0.09), 0, 2 * 0.09));

				m_pirate_rfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= tr;
				break;

			case 4:
			case 9:


				pirate_transformation_coords[index] = glm::vec3(m_tile_positions[i].x * 4 , -2.35, m_tile_positions[i].y * 4 + 2);
				m_pirate_positions[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4, -2.35, m_tile_positions[i].y * 4 + 2.0), glm::vec3(m_tile_positions[i + 1].x * 4 + 2, -2.35, m_tile_positions[i + 1].y * 4), m_continous_time - m_pirate_spawntimes[index] - i);
				


				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(0, 12 * 0.09, 4.5*0.09));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(2 * 0.09, 0, -4 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(2 * 0.09, 0, 4 * 0.09));


				tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -2));

				m_pirate_body_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 12 * 0.09, -2 + (4.5*0.09)));
				m_pirate_rarm_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 * 0.09, 0, -2 + (-4 * 0.09)));
				m_pirate_lfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 * 0.09, 0, -2 + (4 * 0.09)));
				m_pirate_rfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= tr;


				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));



				break;

			case 11:
			case 12:
			case 13:
			case 21:
				pirate_transformation_coords[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4, -2.35, m_tile_positions[i].y * 4 + 2.0), glm::vec3(m_tile_positions[i + 1].x * 4, -2.35, m_tile_positions[i + 1].y * 4 + 2.0), m_continous_time - m_pirate_spawntimes[index] - i);
				m_pirate_positions[index] = pirate_transformation_coords[index];

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(0, 12 * 0.09, 4.5*0.09));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(2 * 0.09, 0, -4 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(2 * 0.09, 0, 4 * 0.09));

				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));

				break;

			case 14:
			case 17:
			case 22:

				pirate_transformation_coords[index] = glm::vec3(m_tile_positions[i].x * 4, -2.35, m_tile_positions[i].y * 4 + 2.0);
				m_pirate_positions[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4, -2.35, m_tile_positions[i].y * 4 + 2), glm::vec3(m_tile_positions[i + 1].x * 4 + 2, -2.35, m_tile_positions[i + 1].y * 4 + 4), m_continous_time - m_pirate_spawntimes[index] - i);

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(0, 12 * 0.09, 4.5*0.09));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(2 * 0.09, 0, -4 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(2 * 0.09, 0, 4 * 0.09));


				tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2));

				m_pirate_body_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 12 * 0.09, 2 + (4.5*0.09)));
				m_pirate_rarm_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 * 0.09, 0, 2 + (-4 * 0.09)));
				m_pirate_lfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 * 0.09, 0, 2 + (4 * 0.09)));
				m_pirate_rfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= tr;


				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0, 1, 0));
				break;

			case 15:
			case 18:
			case 19:
			case 23:
				pirate_transformation_coords[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 2.0, -2.35, m_tile_positions[i].y * 4 + 4.0), glm::vec3(m_tile_positions[i + 1].x * 4 + 2.0, -2.35, m_tile_positions[i + 1].y * 4 + 4.0), m_continous_time - m_pirate_spawntimes[index] - i);
				m_pirate_positions[index] = pirate_transformation_coords[index];

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4.5*0.09, 12 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4 * 0.09, 0, -2 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4 * 0.09, 0, -2 * 0.09));

				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				break;

			case 16:
			case 20:
				pirate_transformation_coords[index] = glm::vec3(m_tile_positions[i].x * 4 + 2, -2.35, m_tile_positions[i].y * 4 + 4);
				m_pirate_positions[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 2, -2.35, m_tile_positions[i].y * 4 + 4), glm::vec3(m_tile_positions[i + 1].x * 4, -2.35, m_tile_positions[i + 1].y * 4 + 2), m_continous_time - m_pirate_spawntimes[index] - i);

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4.5*0.09, 12 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4 * 0.09, 0, -2 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4 * 0.09, 0, -2 * 0.09));


				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2, 0, 0));

				m_pirate_body_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 + (4.5*0.09), 12 * 0.09, 0));

				m_pirate_rarm_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 + (-4 * 0.09), 0, -2 * 0.09));

				m_pirate_lfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 + (4 * 0.09), 0, -2 * 0.09));

				m_pirate_rfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= tr;

				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				break;

			case 24:

				pirate_transformation_coords[index] = glm::vec3(m_tile_positions[i].x * 4 + 2, -2.35, m_tile_positions[i].y * 4 + 4);
				m_pirate_positions[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 2, -2.35, m_tile_positions[i].y * 4 + 4), glm::vec3(m_tile_positions[i + 1].x * 4 + 4, -2.35, m_tile_positions[i + 1].y * 4 + 2), m_continous_time - m_pirate_spawntimes[index] - i);

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4.5*0.09, 12 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4 * 0.09, 0, -2 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4 * 0.09, 0, -2 * 0.09));


				tr = glm::translate(glm::mat4(1.f), glm::vec3(2, 0, 0));

				m_pirate_body_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 + (4.5*0.09), 12 * 0.09, 0));

				m_pirate_rarm_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 + (-4 * 0.09), 0, -2 * 0.09));

				m_pirate_lfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(2 + (4 * 0.09), 0, -2 * 0.09));

				m_pirate_rfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= tr;

				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				break;


			case 25:
			case 26:

				pirate_transformation_coords[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 4.0, -2.35, m_tile_positions[i].y * 4 + 2.0), glm::vec3(m_tile_positions[i + 1].x * 4 + 4.0, -2.35, m_tile_positions[i + 1].y * 4 + 2.0), m_continous_time - m_pirate_spawntimes[index] - i);
				m_pirate_positions[index] = pirate_transformation_coords[index];

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(0, 12 * 0.09, -4.5*0.09));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-2 * 0.09, 0, 4 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-2 * 0.09, 0, -4 * 0.09));

				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));

				break;

			case 27:
				pirate_transformation_coords[index] = glm::vec3(m_tile_positions[i].x * 4 + 4, -2.35, m_tile_positions[i].y * 4 + 2);
				m_pirate_positions[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 4, -2.35, m_tile_positions[i].y * 4 + 2), glm::vec3(m_tile_positions[i + 1].x * 4 + 2, -2.35, m_tile_positions[i + 1].y * 4 + 4), m_continous_time - m_pirate_spawntimes[index] - i);

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(0, 12 * 0.09, -4.5*0.09));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-2 * 0.09, 0, 4 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-2 * 0.09, 0, -4 * 0.09));


				tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2));

				m_pirate_body_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 12 * 0.09, 2 + (-4.5*0.09)));
				m_pirate_rarm_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 * 0.09, 0, 2 + (4 * 0.09)));
				m_pirate_lfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= tr;

				tr = glm::translate(glm::mat4(1.f), glm::vec3(-2 * 0.09, 0, 2 + (-4 * 0.09)));
				m_pirate_rfoot_transformation_matrix[index] *= glm::inverse(tr);
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f)*(m_continous_time - m_pirate_spawntimes[index] - i), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= tr;


				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(0, 1, 0));
				break;
			

			case 28:
			case 29:
				pirate_transformation_coords[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 2.0, -2.35, m_tile_positions[i].y * 4 + 4.0), glm::vec3(m_tile_positions[i].x * 4 + 2.0, -2.35, m_tile_positions[i].y * 4), m_continous_time - m_pirate_spawntimes[index] - i);
				m_pirate_positions[index] = pirate_transformation_coords[index];

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4.5*0.09, 12 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4 * 0.09, 0, -2 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4 * 0.09, 0, -2 * 0.09));

				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				
				if (m_treasure_chest_positions.size() > 0) {
					int min = 0;
					float minLength = glm::length(m_pirate_positions[index] - m_treasure_chest_positions[0]);
					float currentLength;
					for (int j = 0; j < m_treasure_chest_transformation_matrix.size(); j++) {
						currentLength = glm::length(m_pirate_positions[index] - m_treasure_chest_positions[j]);
						if (currentLength < minLength) {
							min = j;
							minLength = currentLength;
						}
					}

					if (minLength <= (12.87075 + 12.0284)*0.09) {
						marked[index] = true;
						updateChest(min);
						if (m_treasure_chest_positions.size() <= 0)
							gameOver = true;
					}

				}
				else
					gameOver = true;
				
				break;

			default:

				pirate_transformation_coords[index] = glm::mix(glm::vec3(m_tile_positions[i].x * 4 + 2.0, -2.35, m_tile_positions[i].y * 4 + 0.0), glm::vec3(m_tile_positions[i + 1].x * 4 + 2.0, -2.35, m_tile_positions[i + 1].y * 4 + 0.0), m_continous_time - m_pirate_spawntimes[index] - i);
				m_pirate_positions[index] = pirate_transformation_coords[index];

				m_pirate_body_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index]);
				m_pirate_rarm_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4.5*0.09, 12 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(4 * 0.09, 0, 2 * 0.09));
				m_pirate_rfoot_transformation_matrix[index] = glm::translate(glm::mat4(1.f), pirate_transformation_coords[index] + glm::vec3(-4 * 0.09, 0, 2 * 0.09));
				break;

			}




			if (!marked[index]) {
				m_pirate_body_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_body_transformation_matrix[index] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
				m_pirate_body_transformation_normal_matrix[index] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_pirate_body_transformation_matrix[index]))));

				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), sin(m_continous_time * 5), glm::vec3(1, 0, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::translate(glm::mat4(1.f), glm::vec3(0, -3 * 0.09, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rarm_transformation_matrix[index] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
				m_pirate_rarm_transformation_normal_matrix[index] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_pirate_rarm_transformation_matrix[index]))));


				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::translate(glm::mat4(1.f), glm::vec3(0, 6 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), -0.8f*sin(m_continous_time * 5), glm::vec3(1, 0, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::translate(glm::mat4(1.f), glm::vec3(0, -6 * 0.09, 0));
				m_pirate_lfoot_transformation_matrix[index] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
				m_pirate_lfoot_transformation_normal_matrix[index] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_pirate_lfoot_transformation_matrix[index]))));

				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::translate(glm::mat4(1.f), glm::vec3(0, 6 * 0.09, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::rotate(glm::mat4(1.f), 0.8f*sin(m_continous_time * 5), glm::vec3(1, 0, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::translate(glm::mat4(1.f), glm::vec3(0, -6 * 0.09, 0));
				m_pirate_rfoot_transformation_matrix[index] *= glm::scale(glm::mat4(1.f), glm::vec3(0.09));
				m_pirate_rfoot_transformation_normal_matrix[index] = glm::mat4(glm::transpose(glm::inverse(glm::mat3(m_pirate_rfoot_transformation_matrix[index]))));
			}
		}
	}

	for (int index = pirateCount - 1; index >= 0; index--) {
		if (marked[index]) {
			removePirate(index);
		}
	}

}


void Renderer::shootCannonballs(int i) {
		glm::vec2 towerCenter = m_placed_towers[i] + glm::vec2(2.0);

		int min = 0;
		float minLength = glm::length(towerCenter - glm::vec2(m_pirate_positions[min].x, m_pirate_positions[min].z));
		float currentLength = minLength;
		
		for (int j = 1; j < m_pirate_positions.size(); j++) {
			if (m_pirate_render[j]) {
				currentLength = glm::length(towerCenter - glm::vec2(m_pirate_positions[j].x, m_pirate_positions[j].z));
				if (currentLength < minLength) {
					min = j;
					minLength = currentLength;
				}
			}
		}

		if (minLength <= 2 * 4.0) {
			m_cannonball_spawntimes[i] = m_continous_time;
			m_last_shots[i] = m_continous_time;
			m_shootAt[i] = min;
			m_cannonball_render[i] = true;
		}
		else
			m_last_shots[i] = 0.0;
}

void Renderer::updateCannonballs() {
	int cannonballCount = m_cannonball_positions.size();
	int pirateCount = m_pirate_lives.size();
	std::vector<bool> marked_pirates = std::vector<bool>(pirateCount);

	for (int i = 0; i < cannonballCount; i++) {
		if (m_cannonball_render[i]) {
			glm::vec3 a = glm::vec3(m_placed_towers[i].x + 2, 9.5626*0.4 - 2.47, m_placed_towers[i].y + 2);
			glm::vec3 b = m_pirate_positions[m_shootAt[i]] + glm::vec3(0, 1, 0);
			glm::vec3 c = glm::normalize(b - a);
			glm::vec3 n = a + c;

			m_cannonball_positions[i] = glm::mix(a, n, (m_continous_time - m_cannonball_spawntimes[i]) * 9);
			if (glm::length(m_cannonball_positions[i] - b) < (1.0*0.1 + 12.87075*0.09)) {

				m_cannonball_render[i] = false;
				m_last_shots[i] = 0.0;

				m_pirate_lives[m_shootAt[i]]--;

				if (m_pirate_lives[m_shootAt[i]] <= 0)
					marked_pirates[m_shootAt[i]] = true;
			}
			else {
				m_cannonball_transformation_matrix[i] = glm::translate(glm::mat4(1.f), m_cannonball_positions[i]);
				m_cannonball_transformation_matrix[i] *= glm::scale(glm::mat4(1.f), glm::vec3(0.1));
				m_cannonball_transformation_normal_matrix[i] = glm::mat4(glm::transpose(glm::inverse(m_cannonball_transformation_matrix[i])));
			}
		}
	}

	for (int i = pirateCount - 1; i >= 0; i--) {
		if (marked_pirates[i]) {
			removePirate(i);
		}
	}
	

}

void Renderer::addRemoval() {
	removals_remaining++;
}

void Renderer::giveTower() {
	available_towers++;
}

void Renderer::updateChest(int i) {
	m_treasure_chest_coins[i] -= 10;

	if (m_treasure_chest_coins[i] <= 0) {
		m_treasure_chest_transformation_matrix.erase(m_treasure_chest_transformation_matrix.begin() + i);
		m_treasure_chest_transformation_normal_matrix.erase(m_treasure_chest_transformation_normal_matrix.begin() + i);
		m_treasure_chest_positions.erase(m_treasure_chest_positions.begin() + i);
		m_treasure_chest_coins.erase(m_treasure_chest_coins.begin() + i);
		m_treasure_chest_exists[i] = false;
	}

}

bool Renderer::getGameOver() {
	return gameOver;
}

bool Renderer::isBoardEmpty() {
	return boardIsEmpty;
}