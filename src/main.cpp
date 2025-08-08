#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

// settings
unsigned int scrWidth =  800;
unsigned int scrHeight = 600;
const char* title = "pong";
GLuint shaderProgram;

// graphics parameters
const float paddleSpeed = 600.0f;
const float paddleHeight = 100.0f;
const float halfPaddleHeight = paddleHeight / 2.0f;
const float paddleWidth = 10.0f;
const float halfPaddleWidth = paddleWidth / 2.0f;
const float ballDiameter = 20.0f;
const float ballRadius = ballDiameter / 2.0f;
const float offset = ballRadius;
const float paddleBoundary = halfPaddleHeight + offset;

/*
    2d vector structure
*/
struct vec2 {
    float x;
    float y;
};

// public offset arrays
vec2 paddleOffsets[2];
vec2 ballOffset;

// public velocities
float paddleVelocities[2];
vec2 initialBallVelocity = {150.0f, 150.0f};
vec2 ballVelocity = {150.0f, 150.0f};

// Initialize GLFW
void initGLFW(unsigned int versionMajor, unsigned int versionMinor) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

// Create window
void createWindow(GLFWwindow*& window,
    const char* title, unsigned int width, unsigned int height,
    GLFWframebuffersizefun framebufferSizeCallback ) {

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if(!window) {
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

// Load Glad library
bool loadGlad() {
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

/*
    Shader Method
*/

// read file
std::string readFile(const char* filename) {
    std::ifstream file;
    std::stringstream buf;

    std::string ret = "";

    // open file
    file.open(filename);
    if(file.is_open()) {
        // read buffer
        buf << file.rdbuf();
        ret = buf.str();
    }
    else {
        std::cout << "Could not open " << filename << std::endl;
    }
    file.close();
    return ret;
}

// generate shade
int genShader(const char* filePath, GLenum type) {
    std::string shaderSrc = readFile(filePath);
    const GLchar* shader = shaderSrc.c_str();

    // build and compile shader
    int shaderObj = glCreateShader(type);
    glShaderSource(shaderObj, 1, &shader, NULL);
    glCompileShader(shaderObj);

    // check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(shaderObj, 512, NULL, infoLog);
        std::cout << "ERROR::COMPILE::SHADER : " << infoLog << std::endl;
        return -1;
    }

    return shaderObj;
}

// generate shader program
int genShaderProgram(const char* vertexShaderPath, const char* fragmentShaderPath) {
    int shaderProgram = glCreateProgram();
    int vertexShader = genShader(vertexShaderPath, GL_VERTEX_SHADER);
    int fragmentShader = genShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

    if(vertexShader == -1 ||  fragmentShader == -1){
        return -1;
    }

    // link shaders
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::LINKING::SHADERS" << infoLog << std::endl;
        return -1;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// bind shader
void bindShader(int shaderProgram) {
    glUseProgram(shaderProgram);
}

// set Projection
void setOrthoGraphicProjection(int shaderProgram,
    float left, float right,
    float bottom, float top,
    float near, float far) {
    float mat[4][4] = {
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
        {0.0f, 0.0f, -2.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f},
    };

    bindShader(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &mat[0][0]);
}

// delete shader
void deleteShader(int shaderProgram) {
    glDeleteProgram(shaderProgram);
}

/*
    Vertex Array Object/Buffer Object Methods
*/

// structure for VAO storing array object  and its Buffer Objects
struct VAO {
    GLuint val;
    GLuint posVBO;
    GLuint offsetVBO;
    GLuint  sizeVBO;
    GLuint EBO;
};

// generate VAO
void genVAO(VAO* vao) {
    glGenVertexArrays(1, &vao->val);
    glBindVertexArray(vao->val);
}

// generate buffer of certain types and set data
template<typename T>
void genBufferObject(GLuint& bo, GLenum type, GLuint noELements, T* data, GLenum usage) {
    glGenBuffers(1, &bo);
    glBindBuffer(type, bo);
    glBufferData(type, noELements * sizeof(T), data, usage);
}

// update data in buffer object
template<typename T>
void updateData(GLuint& bo, GLintptr offset, GLuint noElements, T* data) {
    glBindBuffer(GL_ARRAY_BUFFER, bo);
    glBufferSubData(GL_ARRAY_BUFFER, offset, noElements * sizeof(T), data);
}

// set attribute pointers
template<typename T>
void setAttPointer(GLuint& bo, GLuint idx, GLint size, GLenum type, GLuint stride, GLuint offset, GLuint divisor = 0) {
    glBindBuffer(GL_ARRAY_BUFFER, bo);
    glVertexAttribPointer(idx, size, type, GL_FALSE, stride * sizeof(T), (void*)(offset * sizeof(T)));
    glEnableVertexAttribArray(idx);
    if(divisor > 0) {
        // reset _idx_ attribute very _divisor_ iteration through instances
        glVertexAttribDivisor(idx, divisor);
    }
}

// draw method
void draw(VAO vao, GLenum mode, GLuint count, GLenum type, GLint indices,  GLuint instanceCount = 1) {
    glBindVertexArray(vao.val);
    glDrawElementsInstanced(mode, count, type, (void*)indices, instanceCount);

}

//unbing buffer
void unbindBuffer(GLenum type) {
    glBindBuffer(type, 0);
}

//unbind VAO
void unbindVAO() {
    glBindVertexArray(0);
}

//deallocate VBO/VAO memory
void cleanup(VAO vao) {
    glDeleteBuffers(1, &vao.posVBO);
    glDeleteBuffers(1, &vao.offsetVBO);
    glDeleteBuffers(1, &vao.sizeVBO);
    glDeleteBuffers(1, &vao.EBO);
    glDeleteVertexArrays(1, &vao.val);
}

// method to generate arrays for circle model
void gen2DCircleArray(float*& vertices, unsigned int*& indices, unsigned int noTriangles, float radius = 0.5){
     vertices = new float[(noTriangles + 1) * 2];

     // set origin
     vertices[0] = 0.0f;
     vertices[1] = 0.0f;

     indices = new unsigned int[noTriangles * 3];

    float pi = 4 * atanf(1.0f);
    float noTrianglesF = (float)noTriangles;
    float theta = 0.0f;

    for(int i = 0; i < noTriangles; i++) {
        /*
            theta = i * (2 * pi / noTriangles)
            x = r * cos(theta) = vertices[(i + 1) * 2]
            y = r * sin(theta) = vertices[((i + 1) * 2)+ 1]
        */

        // set vertices
        vertices[((i + 1) * 2) + 0] = radius * cosf(theta);
        vertices[((i + 1) * 2) + 1] = radius * sinf(theta);

        // set indicies
        indices[(i * 3) + 0] = 0;
        indices[(i * 3) + 1] = i + 1;
        indices[(i * 3) + 2] = i + 2;

        // step up theta
        theta += (2 * pi) / noTriangles;
    }

    // set last index to wrap around
    indices[((noTriangles - 1) * 3) + 2] = 1;
}

/*
    main loop methods
*/

// Callback for window size change
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    scrWidth = winWidth;
    scrHeight = winHeight;

    // update orthographic projection
    setOrthoGraphicProjection(shaderProgram, 0, winWidth, 0, winHeight, 0.0f, 1.0f);

    paddleOffsets[1].x = scrWidth- 20.0f;
}

// process input
void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }

    paddleVelocities[0] = 0.0f;
    paddleVelocities[1] = 0.0f;

    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        if(paddleOffsets[0].y < scrHeight - paddleBoundary) {
            paddleVelocities[0] = paddleSpeed;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        if(paddleOffsets[0].y > paddleBoundary) {
            paddleVelocities[0] = -paddleSpeed;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
        if(paddleOffsets[1].y < scrHeight - paddleBoundary) {
            paddleVelocities[1] = paddleSpeed;
        }
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
        if(paddleOffsets[1].y > paddleBoundary) {
            paddleVelocities[1] = -paddleSpeed;
        }
    }
}

//clear screen
void clearScreen() {
    glClearColor(0.f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// new frame
void newFrame(GLFWwindow* window){
    glfwSwapBuffers(window);
    glfwPollEvents();
}

/*
    cleanup mehtods
*/

// terminate GLFW
void cleanup() {
    glfwTerminate();
}

int main() {
    std::cout << "Welcome to pong!" << std::endl;

    // timing
    double dt = 0.0;
    double lastFrame = 0.0;

    // initialization
    initGLFW(3, 3);

    // create window
    GLFWwindow* window = nullptr;
    createWindow(window, title, scrWidth, scrHeight, framebufferSizeCallback);
    if(!window) {
        std::cout << "Could not create window..." << std::endl;
        cleanup();
        return -1;
    }

    // load glad
    if(!loadGlad()) {
        std::cout << "Could not init GLAD..." << std::endl;
        cleanup();
        return -1;
    }

    //glViewport(0, 0, scrWidth, scrHeight);

    // shaders
    shaderProgram = genShaderProgram("main.vs", "main.fs");
    if(shaderProgram == -1) return -1;
    setOrthoGraphicProjection(shaderProgram, 0, scrWidth, 0, scrHeight, 0.0f, 1.0f);

    /*
        paddle VAO/BOs
    */

    //
    // vertex data
    float paddleVertices[] = {
    //    x      y
        0.5f,  0.5f,
       -0.5f,  0.5f,
       -0.5f, -0.5f,
        0.5f, -0.5f,
    };

    //setup index data
    unsigned int paddleIndices[] = {
        0, 1, 2, // top left triangle
        2, 3, 0, // bottom right triangle
    };

    // offset array
    paddleOffsets[0] = {20.0f, scrHeight / 2.0f};
    paddleOffsets[1] = {scrWidth - 20.0f, scrHeight / 2.0f};

    // size array
    vec2 paddleSizes[] = {
        paddleWidth, paddleHeight
    };

    VAO paddleVAO;
    genVAO(&paddleVAO);

    // position VBO
    genBufferObject<float>(paddleVAO.posVBO, GL_ARRAY_BUFFER, 2 * 4, paddleVertices, GL_STATIC_DRAW);
    setAttPointer<float>(paddleVAO.posVBO, 0, 2, GL_FLOAT, 2, 0);

    // offset VBO
    genBufferObject<vec2>(paddleVAO.offsetVBO, GL_ARRAY_BUFFER, 2 * 1, paddleOffsets, GL_DYNAMIC_DRAW);
    setAttPointer<float>(paddleVAO.offsetVBO, 1, 2, GL_FLOAT, 2, 0, 1);

    // size VBO
    genBufferObject<vec2>(paddleVAO.sizeVBO, GL_ARRAY_BUFFER, 1, paddleSizes, GL_STATIC_DRAW);
    setAttPointer<float>(paddleVAO.sizeVBO, 2, 2, GL_FLOAT, 2, 0, 2); // no divisor because it doesn't change between instances

    // EBO
    genBufferObject<unsigned int>(paddleVAO.EBO, GL_ELEMENT_ARRAY_BUFFER, 3 * 2, paddleIndices, GL_STATIC_DRAW);

    // unbind VBO and VAO
    unbindBuffer(GL_ARRAY_BUFFER);
    unbindVAO();

    /*
        ball VAO/BOs
    */

    float* ballVertices;
    unsigned int* ballIndices;
    unsigned int noTriangles = 20;

    gen2DCircleArray(ballVertices, ballIndices, noTriangles, 0.5f);

    // offsets array
    ballOffset = {scrWidth / 2.0f, scrHeight / 2.0f};

    // size array
    vec2 ballSizes[] = {
        ballDiameter, ballDiameter,
    };

    // setup VAOs/VBOs
    VAO ballVAO;
    genVAO(&ballVAO);

    // position VBO
    genBufferObject<float>(ballVAO.posVBO, GL_ARRAY_BUFFER, 2 * (noTriangles + 1), ballVertices, GL_STATIC_DRAW);
    setAttPointer<float>(ballVAO.posVBO, 0, 2, GL_FLOAT, 2, 0);

    // offset VBO
    genBufferObject<vec2>(ballVAO.offsetVBO, GL_ARRAY_BUFFER, 1 * 1, &ballOffset, GL_DYNAMIC_DRAW);
    setAttPointer<float>(ballVAO.offsetVBO, 1, 2, GL_FLOAT, 2, 0, 1);

    // size VBO
    genBufferObject<vec2>(ballVAO.sizeVBO, GL_ARRAY_BUFFER, 1, ballSizes, GL_STATIC_DRAW);
    setAttPointer<float>(ballVAO.sizeVBO, 2, 2, GL_FLOAT, 2, 0, 1);

    // EBO
    genBufferObject<unsigned int>(ballVAO.EBO, GL_ELEMENT_ARRAY_BUFFER, 3 * noTriangles, ballIndices, GL_STATIC_DRAW);

    // unbind VBO and VAO
    unbindBuffer(GL_ARRAY_BUFFER);
    unbindVAO();

    // render loop
    while(!glfwWindowShouldClose(window)){
        // update time
        dt = glfwGetTime() - lastFrame;
        lastFrame += dt;

        // input
        processInput(window);

        /*
            physics
        */

        // update paddle positions
        paddleOffsets[0].y += paddleVelocities[0] * dt;
        paddleOffsets[1].y += paddleVelocities[1] * dt;

        // update ball position
        ballOffset.x += ballVelocity.x * dt;
        ballOffset.y += ballVelocity.y * dt;

        /*
            collision detection
        */

        // check for collision on playing field
        if(ballOffset.y - ballRadius <= 0){
            ballVelocity.y = fabs(ballVelocity.y);
        }
        if(ballOffset.y + ballRadius >= scrHeight) {
            ballVelocity.y = -fabs(ballVelocity.y);
        }

        // paddle and ball collision
        vec2 distanceLeftPaddle  = {fabs(ballOffset.x - paddleOffsets[0].x), fabs(ballOffset.y - paddleOffsets[0].y)};
        vec2 distanceRightPaddle = {fabs(ballOffset.x - paddleOffsets[1].x), fabs(ballOffset.y - paddleOffsets[1].y)};

        bool collisionLeft  = false;
        bool collisionRight = false;
        if(distanceLeftPaddle.x <= ballRadius + halfPaddleWidth && distanceLeftPaddle.y <= ballRadius + halfPaddleHeight){
            collisionLeft = true;
            ballVelocity.x = fabs(ballVelocity.x);
        }
        if(distanceRightPaddle.x <= ballRadius + halfPaddleWidth && distanceRightPaddle.y <= ballRadius + halfPaddleHeight){
            collisionRight = true;
            ballVelocity.x = -fabs(ballVelocity.x);
        }

        float k = 0.2f;

        if(collisionLeft){
            ballVelocity.y += k * paddleVelocities[0];
            ballVelocity.x += 5.0f;
        }
        if(collisionRight){
            ballVelocity.y += k * paddleVelocities[1];
            ballVelocity.x -= 5.0f;
        }

/*
        double squaredDistance[2] = {
            (std::pow((distanceLeftPaddle.x - halfPaddleWidth), 2) + std::pow((distanceLeftPaddle.y - halfPaddleHeight), 2)),
            (std::pow((distanceRightPaddle.x - halfPaddleWidth), 2) + std::pow((distanceRightPaddle.y - halfPaddleHeight), 2)),
        };

        float k = 1.0f;
        if(squaredDistance[0] <= ballRadius) {
            ballVelocity.x  =  fabs(ballVelocity.x);
            ballVelocity.x += k * paddleVelocities[0];
        }

        if(squaredDistance[1] <= ballRadius) {
            ballVelocity.x  = -fabs(ballVelocity.x);
            ballVelocity.x += k * paddleVelocities[0];
        }
*/
        unsigned char reset = 0;
        if(ballOffset.x - ballRadius <= 0) {
            std::cout << "Right Player Point!!! \n";
            reset = 1;
        }
        if(ballOffset.x + ballRadius >= scrWidth){
            std::cout << "Left Player Point!!! \n";
            reset = 2;
        }

        if(reset) {
            // reset ball position
            ballOffset = {scrWidth / 2.0f, scrHeight / 2.0f};

            // reset ball velocity
            ballVelocity.x = (reset == 1 ? initialBallVelocity.x : -initialBallVelocity.x);
            ballVelocity.y = initialBallVelocity.y;

            // reset paddle position
            paddleOffsets[0] = {20.0f, scrHeight / 2.0f};
            paddleOffsets[1] = {scrWidth - 20.0f, scrHeight / 2.0f};
        }

        /*
            graphics
        */

        // clear screen for new frame
        clearScreen();

        //update data
        updateData<vec2>(paddleVAO.offsetVBO, 0, 2 * 1, paddleOffsets);
        updateData<vec2>(ballVAO.offsetVBO, 0, 1 * 1, &ballOffset);

        // render object
        bindShader(shaderProgram);
        draw(paddleVAO, GL_TRIANGLES, 3 * 2, GL_UNSIGNED_INT, 0, 2);
        draw(ballVAO, GL_TRIANGLES, 3 * noTriangles, GL_UNSIGNED_INT, 0);

        // swap frames
        newFrame(window);
    }

    // cleanup memory
    cleanup(ballVAO);
    deleteShader(shaderProgram);
    cleanup();

    return 0;
}
