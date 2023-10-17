//Matthew Cochrane
//CS-330
//October 11, 2023

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <vector>
#include "meshes.h"


// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//image loader for textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Cochrane - 3D Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1200;
    const int WINDOW_HEIGHT = 900;

    //camera initial settings
    // camera
    glm::vec3 cameraPos = glm::vec3(2.5f, 3.0f, 12.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, -0.5f, -1.0f);
    glm::vec3 cameraRight = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraPos, cameraRight));

    bool firstMouse = true;
    bool orthoOn = false;
    glm::mat4 projection; //move to a global to set from a keystroke

    float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
    float pitch = 0.0f;
    float lastX = WINDOW_WIDTH / 2.0;
    float lastY = WINDOW_HEIGHT / 2.0;
    float fov = 45.0f;
    float sensitivity = 0.03f; // change this to a global with a default; to be controlled by scroll

    // timing
    float deltaTime = 0.0f;	// time between current frame and last frame
    float lastFrame = 0.0f;

    // Key light
    glm::vec3 gKeyColor(1.0f, 0.95f, 0.8f);
    glm::vec3 gKeyPosition(1.0f, 3.0f, 3.0f);
    glm::vec3 gKeyScale(0.3f);
    glm::vec2 gUVScale(1.0f, 1.0f);

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    //Generic primative shape mesh
    Meshes meshes;

    // Shader programs
    GLuint gProgramId;
    GLuint gLampId;

    // Texture id
    GLuint gTextureWatchFace, gTextureLeather, gTextureWood, gTextureCubeFace, gTextureCopper, gTextureGlass;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

//textures
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

//add the new prototypes for the keys and mouse controls
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

/* Cube Vertex Shader Source Code*/
const GLchar*  cubeVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);

/* Cube Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 keyColor;
uniform vec3 keyPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    float keyStrength = 1.0f;
    vec3 key = keyStrength * keyColor;

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 2.0f; // Set specular light strength
    float highlightSize = 10.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + key + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the shader program
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampId))
        return EXIT_FAILURE;
    // Create the mesh
    //UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
    meshes.CreateMeshes();

    // Load texture 0
    const char* tex1 = "Textures/wood.jpg";
    if (!UCreateTexture(tex1, gTextureWood))
    {
        cout << "Failed to load texture " << tex1 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 1
    const char* tex2 = "Textures/leather.jpg";
    if (!UCreateTexture(tex2, gTextureLeather))
    {
        cout << "Failed to load texture " << tex2 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 2
    const char* tex3 = "Textures/faceWatch.png";
    if (!UCreateTexture(tex3, gTextureWatchFace))
    {
        cout << "Failed to load texture " << tex3 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 3
    const char* tex4 = "Textures/cubeface.png";
    if (!UCreateTexture(tex4, gTextureCubeFace))
    {
        cout << "Failed to load texture " << tex4 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 3
    const char* tex5 = "Textures/copper.jpg";
    if (!UCreateTexture(tex5, gTextureCopper))
    {
        cout << "Failed to load texture " << tex5 << endl;
        return EXIT_FAILURE;
    }

    // Load texture 3
    const char* tex6 = "Textures/blue.jpg";
    if (!UCreateTexture(tex6, gTextureGlass))
    {
        cout << "Failed to load texture " << tex6 << endl;
        return EXIT_FAILURE;
    }



    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);

    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        // Turn on wireframe mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //use wireframe for QA GL_FILL for off GL_LINE for on
        URender();

        glfwPollEvents();
    }

    //delete the meshes
    meshes.DestroyMeshes();

    // Release texture
    UDestroyTexture(gTextureWood);
    UDestroyTexture(gTextureWatchFace);
    UDestroyTexture(gTextureLeather);
    UDestroyTexture(gTextureCubeFace);
    UDestroyTexture(gTextureCopper);
    UDestroyTexture(gTextureGlass);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);

    //Register the new callbacks
    glfwSetCursorPosCallback(*window, mouseCallback);
    glfwSetScrollCallback(*window, scrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);
    glfwSetKeyCallback(*window, keyCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    //TODO
    //WASD Keys to pan Left, Tight, forward, back
    float cameraSpeed = static_cast<float>(25 * deltaTime * sensitivity);
    glm::vec3 front;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        pitch += cameraSpeed * 5;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        pitch -= cameraSpeed * 5;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        //toggle the orthographic view
        //set the ortho view then flip the global variable
        if (orthoOn) {
            orthoOn = false;
            cout << "Ortho State" << orthoOn << endl;
        }
        else {
            orthoOn = true;
            cout << "Ortho State" << orthoOn << endl;
        }
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    //fov -= (float)yoffset;
    //if (fov < 1.0f)
    //    fov = 1.0f;
    //if (fov > 90.0f)
    //    fov = 90.0f;
    sensitivity += 0.01 * (float)yoffset;;
    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (sensitivity > 3.0f)
        sensitivity = 3.0f;
    if (sensitivity < 0.01f)
        sensitivity = 0.01f;

    cout << sensitivity << endl;
}

void mouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS) {
            cout << "Middle mouse button pressed - sensitivity reset" << endl;
            sensitivity = 0.03f;
        }
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Functioned called to render a frame
void URender()
{

    glm::mat4 scale;
    glm::mat4 rotation;
    glm::mat4 translation;
    glm::mat4 model;
    glm::mat4 view;

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Transforms the camera: move the camera back (z axis)
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    if (orthoOn) {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f); //use the orthographic projection for QA
    }
    else
        projection = glm::perspective(glm::radians(fov), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gProgramId);

    //Create the Plane
    glBindVertexArray(meshes.gPlaneMesh.vao);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(5.0f, 1.0f, 4.0f));
    // 2. Rotate the object
    rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint objectColorLoc = glGetUniformLocation(gProgramId, "uObjectColor");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
    
    GLint keyColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint keyPositionLoc = glGetUniformLocation(gProgramId, "lightPos");

    glUniform3f(keyColorLoc, gKeyColor.r, gKeyColor.g, gKeyColor.b);
    glUniform3f(keyPositionLoc, gKeyPosition.x, gKeyPosition.y, gKeyPosition.z);

    const glm::vec3 cameraPosition = cameraPos;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
    
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureWood);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Watch face

    glBindVertexArray(meshes.gCylinderMesh.vao);
    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.4f, 0.03f, 0.4f));
    // 2. Rotate the object
    rotation = glm::rotate(0.0f, glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(0.0f, 0.1f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glProgramUniform4f(gProgramId, objectColorLoc, 0.15f, 0.14f, 0.32f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureWatchFace);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
    glDrawArrays(GL_TRIANGLE_FAN, 36, 36);
    glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);

    glBindVertexArray(0);

    //Strap 
    glBindVertexArray(meshes.gBoxMesh.vao);

    // 1. Scales the object
    scale = glm::scale(glm::vec3(2.5f, 0.02f, 0.4f));
    // 2. Rotate the object
    rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
    // 3. Position the object
    translation = glm::translate(glm::vec3(0.0f, 0.07f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureLeather);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    glBindVertexArray(0);

    // Rubik's Cube
    glBindVertexArray(meshes.gBoxMesh.vao);

    // 1. Scales the object
    scale = glm::scale(glm::vec3(1.2f, 1.2f, 1.2f));
    // 2. Rotate the object
    rotation = glm::rotate(45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    // 3. Position the objecta
    translation = glm::translate(glm::vec3(-2.0f, 0.65f, 1.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureCubeFace);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

    glBindVertexArray(0);

    // Ring
    glBindVertexArray(meshes.gTorusMesh.vao);

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.15f, 0.15f, 0.5f));
    // 2. Rotate the object
    rotation = glm::rotate(1.55f, glm::vec3(1.0f, 0.0f, 0.0f));
    // 3. Position the objecta
    translation = glm::translate(glm::vec3(0.5f, 0.1f, 1.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureCopper);

    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);
    glBindVertexArray(0);

    // Glass slide

    glBindVertexArray(meshes.gTorusMesh.vao);

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.2f, 0.2f, 5.0f));
    // 2. Rotate the object
    rotation = glm::rotate(1.8f, glm::vec3(0.0f, 1.0f, 0.0f));
    // 3. Position the objecta
    translation = glm::translate(glm::vec3(2.0f, 0.2f, 1.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureGlass);

    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);
    glBindVertexArray(0);


    // Spacer to prevent artifacting

    glBindVertexArray(meshes.gTorusMesh.vao);

    // 1. Scales the object
    scale = glm::scale(glm::vec3(0.01f, 0.01f, 0.01f));
    // 2. Rotate the object
    rotation = glm::rotate(1.8f, glm::vec3(0.0f, 1.0f, 0.0f));
    // 3. Position the objecta
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    model = translation * rotation * scale;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glProgramUniform4f(gProgramId, objectColorLoc, 0.97f, 0.48f, 0.04f, 1.0f);

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureCopper);

    glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);
    glBindVertexArray(0);

    // Key Light
    glBindVertexArray(meshes.gBoxMesh.vao);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gKeyPosition) * glm::scale(gKeyScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampId, "model");
    viewLoc = glGetUniformLocation(gLampId, "view");
    projLoc = glGetUniformLocation(gLampId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gBoxMesh.nVertices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        //flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}