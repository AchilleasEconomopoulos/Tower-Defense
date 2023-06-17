#include <string>
#include <vector>
#include "GLEW\glew.h"
#include "glm\glm.hpp"

#ifndef TOOLS_H
#define TOOLS_H

namespace Tools
{
	char* LoadWholeStringFile(const char* filename);

	std::string GetFolderPath(const char* filename);

	std::string tolowerCase(std::string str);

	bool compareStringIgnoreCase(std::string str1, std::string str2);

	GLenum CheckGLError();

	GLenum CheckFramebufferStatus(GLuint framebuffer_object);

	int vectorIndex(std::vector<glm::vec2> v, glm::vec2 entry);
};

#endif