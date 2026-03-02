#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h" 

struct Node {
    int parent = -1;
    glm::mat4 localFrame = glm::mat4(1.0f);
    glm::mat4 worldFrame = glm::mat4(1.0f);
    glm::mat4 drawFromFrame = glm::mat4(1.0f);
    glm::vec3 color = glm::vec3(1.0f);
};

struct MeshGL { GLuint vao; int indexCount; };
struct LinesGL { GLuint vao; int vertCount; };

MeshGL createCube() {
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    unsigned int indices[] = {
        0,1,2,2,3,0, 4,5,6,6,7,4, 8,9,10,10,11,8, 12,13,14,14,15,12, 16,17,18,18,19,16, 20,21,22,22,23,20
    };
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    return { vao, 36 };
}

LinesGL createAxes() {
    float vertices[] = { 0,0,0, 1,0,0, 0,0,0, 0,1,0, 0,0,0, 0,0,1 };
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return { vao, 6 };
}

static void updateWorldFrames(std::vector<Node>& nodes) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].parent < 0) nodes[i].worldFrame = nodes[i].localFrame;
        else nodes[i].worldFrame = nodes[nodes[i].parent].worldFrame * nodes[i].localFrame;
    }
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "Robot Arm FK", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Shader meshSh, lineSh;
    meshSh.CreateFromFiles("Shaders/Lab3/mesh.vert", "Shaders/Lab3/mesh.frag");
    lineSh.CreateFromFiles("Shaders/Lab3/line.vert", "Shaders/Lab3/line.frag");

    MeshGL cube = createCube();
    LinesGL axes = createAxes();

    std::vector<Node> nodes(5);
    nodes[1].parent = 0; nodes[2].parent = 1; nodes[3].parent = 2; nodes[4].parent = 3;

    float upperLen = 1.3f;
    float foreLen = 0.7f;
    float fingerLen = 0.3f;

    nodes[0].drawFromFrame = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.2f, 0.5f));
    nodes[0].color = glm::vec3(0.4f);
    nodes[1].drawFromFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, upperLen/2, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, upperLen, 0.2f));
    nodes[1].color = glm::vec3(0.8f, 0.2f, 0.2f);
    nodes[2].drawFromFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, foreLen/2, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, foreLen, 0.12f));
    nodes[2].color = glm::vec3(0.2f, 0.8f, 0.2f);
    nodes[3].drawFromFrame = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f));
    nodes[3].color = glm::vec3(0.2f, 0.2f, 0.8f);
    nodes[4].drawFromFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, fingerLen/2, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, fingerLen, 0.06f));
    nodes[4].color = glm::vec3(1.0f, 0.8f, 0.0f);

    float shoulderYaw = 0.0f;
    float elbowBend = 0.0f;
    float fingerAngle = 0.0f;
    float dt = 0.016f;

    glm::mat4 view = glm::lookAt(glm::vec3(3,3,3), glm::vec3(0,0.5,0), glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) shoulderYaw += 2.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) shoulderYaw -= 2.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) elbowBend += 2.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) elbowBend -= 2.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) fingerAngle += 2.0f * dt;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) fingerAngle -= 2.0f * dt;

        elbowBend = glm::clamp(elbowBend, glm::radians(-10.0f), glm::radians(130.0f));
        fingerAngle = glm::clamp(fingerAngle, glm::radians(-45.0f), glm::radians(45.0f));

        nodes[0].localFrame = glm::mat4(1.0f);
        nodes[1].localFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.1f, 0)) * glm::rotate(glm::mat4(1.0f), shoulderYaw, glm::vec3(0,1,0));
        nodes[2].localFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, upperLen, 0)) * glm::rotate(glm::mat4(1.0f), elbowBend, glm::vec3(1,0,0));
        nodes[3].localFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, foreLen, 0));
        nodes[4].localFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0.05f, 0)) * glm::rotate(glm::mat4(1.0f), fingerAngle, glm::vec3(1,0,0));

        updateWorldFrames(nodes);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        meshSh.UseShader();
        glUniformMatrix4fv(meshSh.GetUniformLocation("uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(meshSh.GetUniformLocation("uProj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(meshSh.GetUniformLocation("uLightDir"), 1.0f, 1.0f, 1.0f);

        for (auto& n : nodes) {
            glm::mat4 modelMat = n.worldFrame * n.drawFromFrame;
            glUniformMatrix4fv(meshSh.GetUniformLocation("uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
            glUniform3fv(meshSh.GetUniformLocation("uColor"), 1, glm::value_ptr(n.color));
            glBindVertexArray(cube.vao);
            glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
        }

        lineSh.UseShader();
        glUniformMatrix4fv(lineSh.GetUniformLocation("uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(lineSh.GetUniformLocation("uProj"), 1, GL_FALSE, glm::value_ptr(proj));
        for (auto& n : nodes) {
            glUniformMatrix4fv(lineSh.GetUniformLocation("uModel"), 1, GL_FALSE, glm::value_ptr(n.worldFrame));
            glBindVertexArray(axes.vao);
            glDrawArrays(GL_LINES, 0, axes.vertCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}