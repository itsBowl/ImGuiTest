#pragma once
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

class Framebuffer
{
public:
	Framebuffer(float, float);
	Framebuffer(glm::vec2);
	~Framebuffer();

	GLuint getFrameTexture() { return texture; }
	void Bind() const;
	void Unbind() const;
	void Clear() const;

private:

	GLuint frameBuffer;
	GLuint texture;
	GLuint renderBuffer;

};

