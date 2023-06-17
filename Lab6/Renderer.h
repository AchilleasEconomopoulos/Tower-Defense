#ifndef BIM_ENGINE_RENDERER_H
#define BIM_ENGINE_RENDERER_H

#include "GLEW\glew.h"
#include "glm\glm.hpp"
#include <vector>
#include "ShaderProgram.h"
#include "SpotlightNode.h"
#include <unordered_set>

class Renderer
{
public:

	enum RENDERING_MODE
	{
		TRIANGLES,
		LINES,
		POINTS
	};

	enum TILE
	{
		SELECT,
		GREEN,
		RED,
	};

protected:
	int												m_screen_width, m_screen_height;
	glm::mat4										m_view_matrix;
	glm::mat4										m_projection_matrix;
	glm::vec3										m_camera_position;
	glm::vec3										m_camera_target_position;
	glm::vec3										m_camera_up_vector;
	glm::vec2										m_camera_movement;
	glm::vec2										m_camera_look_angle_destination;
	std::vector<glm::vec2>							m_tile_positions;
	glm::vec3										color;

	std::vector<glm::vec2>							m_tower_positions;
	std::vector<glm::vec2>							m_placed_towers;
	std::vector<glm::vec3>							m_pirate_positions;
	std::vector<glm::vec3>							m_cannonball_positions;
	std::vector<glm::vec3>							m_treasure_chest_positions;

	

	int												m_skeleton_spawntimes;
	std::vector<float>								m_last_shots;


	int												available_towers;
	int											    removals_remaining;
	bool											boardIsEmpty;
	bool											gameOver;

	TILE											selection;


	glm::vec2										m_selection_movement;
	glm::vec3										m_selection_position;


	// Geometry Rendering Intermediate Buffer
	GLuint m_fbo;
	GLuint m_fbo_depth_texture;
	GLuint m_fbo_texture;

	GLuint m_vao_fbo, m_vbo_fbo_vertices;

	
	float m_continous_time;

	// Rendering Mode
	RENDERING_MODE m_rendering_mode;

	// Lights
	SpotLightNode m_spotlight_node;

	// Meshes
	class GeometryNode*								m_terrain;
	glm::mat4										m_terrain_transformation_matrix;
	glm::mat4										m_terrain_transformation_normal_matrix;
	class GeometryNode*								m_road;
	std::vector<glm::mat4>							m_road_transformation_matrix;
	std::vector<glm::mat4>							m_road_transformation_normal_matrix;
	class GeometryNode*								m_treasure_chest;
	std::vector<glm::mat4>							m_treasure_chest_transformation_matrix;
	std::vector<glm::mat4>							m_treasure_chest_transformation_normal_matrix;
	class GeometryNode*								m_green_plane;
	glm::mat4										m_green_plane_transformation_matrix;
	glm::mat4										m_green_plane_transformation_normal_matrix;
	class GeometryNode*								m_red_plane;
	glm::mat4										m_red_plane_transformation_matrix;
	glm::mat4										m_red_plane_transformation_normal_matrix;
	class GeometryNode*								m_tower;
	glm::mat4										m_tower_transformation_matrix;
	glm::mat4										m_tower_transformation_normal_matrix;
	class GeometryNode*								m_cannonball;
	std::vector<glm::mat4>							m_cannonball_transformation_matrix;
	std::vector<glm::mat4>							m_cannonball_transformation_normal_matrix;
	class GeometryNode*								m_pirate_body;
	std::vector<glm::mat4>							m_pirate_body_transformation_matrix;
	std::vector<glm::mat4>							m_pirate_body_transformation_normal_matrix;
	class GeometryNode*								m_pirate_rarm;
	std::vector<glm::mat4>							m_pirate_rarm_transformation_matrix;
	std::vector<glm::mat4>							m_pirate_rarm_transformation_normal_matrix;
	class GeometryNode*								m_pirate_rfoot;
	std::vector<glm::mat4>							m_pirate_rfoot_transformation_matrix;
	std::vector<glm::mat4>							m_pirate_rfoot_transformation_normal_matrix;
	class GeometryNode*								m_pirate_lfoot;
	std::vector<glm::mat4>							m_pirate_lfoot_transformation_matrix;
	std::vector<glm::mat4>							m_pirate_lfoot_transformation_normal_matrix;

	
	std::vector<float>								m_pirate_spawntimes;
	std::vector<int>								m_pirate_lives;
	std::vector<bool>								m_pirate_render;
	std::vector<float>								m_cannonball_spawntimes;
	std::vector<bool>								m_cannonball_render;
	std::vector<int>								m_shootAt;
	std::vector<int>								m_treasure_chest_coins;
	std::vector<bool>								m_treasure_chest_exists;


	// Protected Functions
	bool InitRenderingTechniques();
	bool InitIntermediateShaderBuffers();
	bool InitCommonItems();
	bool InitLightSources();
	bool InitGeometricMeshes();

	void DrawGeometryNode(class GeometryNode* node, glm::mat4 model_matrix, glm::mat4 normal_matrix);

	void DrawGeometryNodeToShadowMap(class GeometryNode* node, glm::mat4 model_matrix, glm::mat4 normal_matrix);

	ShaderProgram								m_shadowed_geometry_rendering_program;
	ShaderProgram								m_basic_geometry_rendering_program;
	ShaderProgram								m_postprocess_program;
	ShaderProgram								m_spot_light_shadow_map_program;

	ShaderProgram								m_particle_rendering_program;

public:
	Renderer();
	~Renderer();
	bool										Init(int SCREEN_WIDTH, int SCREEN_HEIGHT);
	void										Update(float dt);
	bool										ResizeBuffers(int SCREEN_WIDTH, int SCREEN_HEIGHT);
	bool										ReloadShaders();
	void										Render();
	void										InitializeArrays();

	// Passes
	void										RenderShadowMaps();
	void										RenderGeometry();
	void										RenderToOutFB();
	
	// Set functions
	void										SetRenderingMode(RENDERING_MODE mode);

	// Camera Function
	void										CameraMoveForward(bool enable);
	void										CameraMoveBackWard(bool enable);
	void										CameraMoveLeft(bool enable);
	void										CameraMoveRight(bool enable);
	void										CameraLook(glm::vec2 lookDir);	

	// gp Move Functions
	void										gpMoveForward(bool enable);
	void										gpMoveBackWard(bool enable);
	void										gpMoveLeft(bool enable);
	void										gpMoveRight(bool enable);


	void										currentAction(TILE tileColor);
	void										placeTower();
	void										removeTower();
	void										addPirateWave(int life);
	void										removePirate(int index);
	void										movePirates(int count);
	void										shootCannonballs(int i);
	void										updateCannonballs();
	void										addRemoval();
	void										giveTower();
	void										updateChest(int index);

	bool										getGameOver();
	bool										isBoardEmpty();
};

#endif
