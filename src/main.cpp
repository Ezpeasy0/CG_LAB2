#undef GLFW_DLL
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Libs/Window.h"
#include "Libs/Shader.h"

static const GLint WIDTH = 900, HEIGHT = 650;

struct MeshGL {
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;

    void destroy() {
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
        VAO = VBO = EBO = 0;
        indexCount = 0;
    }
};

// Helper to build mesh with specific stride (8 for PBR: Pos, Norm, UV)
static MeshGL buildIndexedMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    MeshGL m;
    glGenVertexArrays(1, &m.VAO);
    glBindVertexArray(m.VAO);

    glGenBuffers(1, &m.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Stride is 8: 3 for Pos, 3 for Normal, 2 for UV
    GLsizei stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    m.indexCount = static_cast<GLsizei>(indices.size());
    return m;
}

static MeshGL makeSphere(int stacks = 32, int slices = 64) {
    std::vector<float> v;
    std::vector<unsigned int> idx;
    const float PI = 3.1415926535f;

    for (int i = 0; i <= stacks; ++i) {
        float t = float(i) / float(stacks);
        float phi = t * PI;
        for (int j = 0; j <= slices; ++j) {
            float s = float(j) / float(slices);
            float theta = s * 2.0f * PI;
            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);
            
            v.push_back(x); v.push_back(y); v.push_back(z); // Pos
            v.push_back(x); v.push_back(y); v.push_back(z); // Normal
            v.push_back(s); v.push_back(1.0f - t);          // UV
        }
    }

    const int ring = slices + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int a = i * ring + j;
            unsigned int b = (i + 1) * ring + j;
            unsigned int c = (i + 1) * ring + (j + 1);
            unsigned int d = i * ring + (j + 1);
            idx.push_back(a); idx.push_back(b); idx.push_back(c);
            idx.push_back(a); idx.push_back(c); idx.push_back(d);
        }
    }
    return buildIndexedMesh(v, idx);
}

int main() {
    Window mainWindow(WIDTH, HEIGHT, 3, 3);
    if (mainWindow.initialise() != 0) return 1;

    GLFWwindow* w = mainWindow.getWindow();
    glfwSetWindowTitle(w, "Lab 2 - Physically Based Rendering (PBR)");
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    Shader pbr;
    pbr.CreateFromFiles("Shaders/Lab2/pbr.vert", "Shaders/Lab2/pbr.frag");

    // Uniform locations
    GLuint uModel = pbr.GetUniformLocation("uModel");
    GLuint uView = pbr.GetUniformLocation("uView");
    GLuint uProj = pbr.GetUniformLocation("uProj");
    GLuint uCamPos = pbr.GetUniformLocation("uCamPos");
    GLuint uAlbedo = pbr.GetUniformLocation("uAlbedo");
    GLuint uMetallic = pbr.GetUniformLocation("uMetallic");
    GLuint uRoughness = pbr.GetUniformLocation("uRoughness");
    GLuint uAO = pbr.GetUniformLocation("uAO");

    auto srgbToLinear = [](glm::vec3 c) {
        return glm::vec3(std::pow(c.x, 2.2f), std::pow(c.y, 2.2f), std::pow(c.z, 2.2f));
    };

    MeshGL sphere = makeSphere(32, 64);
    glm::vec3 camPos(0.0f, 0.0f, 13.5f);
    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    glm::vec3 albedo(0.95f, 0.0f, 0.0f);
    float ao = 1.0f;

    while (!glfwWindowShouldClose(w)) {
        glfwPollEvents();
        static int prev[GLFW_KEY_LAST + 1] = {0};
        auto pressedOnce = [&](int key) {
            int cur = glfwGetKey(w, key);
            bool fired = (cur == GLFW_PRESS && prev[key] != GLFW_PRESS);
            prev[key] = cur;
            return fired;
        };

        if (pressedOnce(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(w, GLFW_TRUE);
        if (pressedOnce(GLFW_KEY_C)) {
            static int preset = 0; preset = (preset + 1) % 3;
            if (preset == 0) albedo = glm::vec3(0.95f, 0.64f, 0.54f); //copper
            if (preset == 1) albedo = glm::vec3(0.91f, 0.78f, 0.45f); //brass
            if (preset == 2) albedo = glm::vec3(0.82f, 0.67f, 0.60f); //bronze
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        pbr.UseShader();

        glUniformMatrix4fv(uView, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(uProj, 1, GL_FALSE, &proj[0][0]);
        glUniform3fv(uCamPos, 1, &camPos[0]);

        // Dynamic Lighting
        float t = static_cast<float>(glfwGetTime());
        glm::vec3 lightPositions[] = {
            glm::vec3(6.0f * std::cos(t), 4.0f, 6.0f * std::sin(t)),
            glm::vec3(6.0f * std::cos(t + 3.14f), 4.0f, 6.0f * std::sin(t + 3.14f)),
            glm::vec3(0.0f, 6.0f + 1.5f * std::sin(t * 1.3f), 0.0f),
            glm::vec3(8.0f * std::cos(t * 0.35f), -2.5f, 8.0f * std::sin(t * 0.35f))
        };
        glm::vec3 lightColors[] = {
            glm::vec3(80.0f), glm::vec3(50.0f), glm::vec3(30.0f), glm::vec3(60.0f)
        };

        for (int i = 0; i < 4; i++) {
            std::string posName = "uLightPos[" + std::to_string(i) + "]";
            std::string colName = "uLightColor[" + std::to_string(i) + "]";
            glUniform3fv(pbr.GetUniformLocation(posName.c_str()), 1, &lightPositions[i][0]);
            glUniform3fv(pbr.GetUniformLocation(colName.c_str()), 1, &lightColors[i][0]);
        }

        glUniform3fv(uAlbedo, 1, &srgbToLinear(albedo)[0]);
        glUniform1f(uAO, ao);

        // Rendering Grid
        const int rows = 5, cols = 5;
        const float spacing = 2.2f;
        glBindVertexArray(sphere.VAO);
        for (int r = 0; r < rows; r++) {
            glUniform1f(uMetallic, (float)r / (float)(rows - 1));
            for (int c = 0; c < cols; c++) {
                glUniform1f(uRoughness, glm::clamp((float)c / (float)(cols - 1), 0.05f, 1.0f));
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((c - 2) * spacing, (r - 2) * spacing, 0.0f));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, &model[0][0]);
                glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
            }
        }
        glfwSwapBuffers(w);
    }
    sphere.destroy();
    return 0;
}