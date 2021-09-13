#pragma once
#include <GL/glew.h>
#include <assert.h>
#include <Mesh.h>
#include <Shader.h>
class Renderer
{
private:
    Renderer();
    GLuint cubeMap;
    GLuint framebuffers[6];
    std::vector<Mesh> meshes;
    std::vector<glm::vec3> axis;
    Shader lightShader, mainShader;

public:
    static Renderer &instance();
    void init();
    void renderPass();
};