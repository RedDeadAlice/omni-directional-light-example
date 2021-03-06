/*
MIT License

Copyright (c) 2021 Mohammad Issawi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <Renderer.h>
using namespace glm;
using namespace std;
#define SHADOWMAP 1024
Renderer::Renderer()
{
}
Renderer &Renderer::instance()
{
    static Renderer renderer;
    return renderer;
}
void Renderer::init()
{
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
        assert("Failed to init glew" && 0);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    //Generate cubemap
    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //bind cubemap to 6 framebuffers
    for (unsigned i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH24_STENCIL8,
                     SHADOWMAP, SHADOWMAP, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glGenFramebuffers(1, &framebuffers[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        assert(glGetError() == 0);
    }
    //write shadow views matrices for depth writing
    vec3 lightPos = vec3(0, 0, 0);
    shadowViews.push_back(
        glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    shadowViews.push_back(
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    shadowViews.push_back(
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
    shadowViews.push_back(
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
    shadowViews.push_back(
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
    shadowViews.push_back(
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
    //set up shaders and uniforms
    lightShader = Shader("./Assets/light.vert", "./Assets/light.frag");
    glUseProgram(lightShader.id);
    lightShader.registerUniform("model");
    lightShader.registerUniform("view");
    lightShader.registerUniform("persp");
    //90 degrees per face to cover the whole scene
    lightShader.updateUniform("persp", perspective(radians(90.0f), 1.0f, 0.1f, 100.0f));
    mainShader = Shader("./Assets/main.vert", "./Assets/main.frag");
    glUseProgram(mainShader.id);
    mainShader.registerUniform("model");
    mainShader.registerUniform("view");
    mainShader.registerUniform("persp");
    mainShader.updateUniform("persp", perspective(radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f));
    mainShader.updateUniform("view", lookAt(vec3(-9,-3,-9), vec3(2, 5, 2), vec3(0, 1, 0)));
    //a huge cube acting as a room, weirdo
    Mesh room("./Assets/FlippedCube.glb", vec3(0, 0, 0));
    Mesh cubeA("./Assets/Cube.glb", vec3(2, 2, 2));
    meshes.push_back(room);
    meshes.push_back(cubeA);
    assert("Failed to init" && glGetError() == 0);
}
void Renderer::renderPass()
{
    //writing depths to cubemap
    glViewport(0, 0, SHADOWMAP, SHADOWMAP);
    for (unsigned i = 0; i < 6; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(lightShader.id);
        lightShader.updateUniform("view", shadowViews[i]);
        for (unsigned j = 0; j < meshes.size(); ++j)
        {
            lightShader.updateUniform("model", meshes[j].model);
            meshes[j].draw();
        }
    }
    //drawing the final scene with sampling the cubemap
    glViewport(0, 0, 1280, 720);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(mainShader.id);
    for (unsigned i = 0; i < meshes.size(); ++i)
    {
        mainShader.updateUniform("model", meshes[i].model);
        meshes[i].draw();
    }
    assert("Error in renderPass" && glGetError() == 0);
}