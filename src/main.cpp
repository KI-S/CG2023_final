#include <algorithm>
#include <memory>
#include <vector>
#include <iostream>

#include <GLFW/glfw3.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include <glm/glm.hpp>

#include "camera.h"
#include "opengl_context.h"
#include "utils.h"

#define ANGLE_TO_RADIAN(x) (float)((x)*M_PI / 180.0f) 
#define RADIAN_TO_ANGEL(x) (float)((x)*180.0f / M_PI) 

#define CIRCLE_SEGMENT 64

#define ROTATE_SPEED 1.0f
#define WING_ROTATE_SPEED 5.0f
#define FLYING_SPEED ROTATE_SPEED / 20.f
#define BULLET_SPEED 3.0f

#define RED 0.905f, 0.298f, 0.235f
#define BLUE 0.203f, 2.096f, 0.858f
#define GREEN 0.18f, 0.8f, 0.443f
#define YELLOW 1.0f, 1.0f, 0.0f
#define WHITE 1.0f, 1.0f, 1.0f

// init move bool
bool isSpace = false;
bool isLeft = false;
bool isRight = false;

// bonus bullet
bool isShoot = false;

// init rotation degree and position information
float rot = 0.0f;
float wing_rot = 0.0f;
bool wing_down = true;
glm::vec3 pos = {0.0, 0.0, 0.0};

void resizeCallback(GLFWwindow* window, int width, int height) {
  OpenGLContext::framebufferResizeCallback(window, width, height);
  auto ptr = static_cast<Camera*>(glfwGetWindowUserPointer(window));
  if (ptr) {
    ptr->updateProjectionMatrix(OpenGLContext::getAspectRatio());
  }
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
  // There are three actions: press, release, hold(repeat)
  // if (action == GLFW_REPEAT) return;

  // Press ESC to close the window.
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  /* TODO#4-1: Detect key-events, perform rotation or fly
   *       1. Use switch && case to find the key you want.
   *       2. Press "SPACE" for fly up, fly forward and wing rotate meanwhile.
   *       3. Press "GLFW_KEY_LEFT" for turn left.
   *       4. Press "GLFW_KEY_RIGHT " for turn right.
   * Hint:
   *       glfw3's key list (https://www.glfw.org/docs/3.3/group__keys.html)
   *       glfw3's action codes (https://www.glfw.org/docs/3.3/group__input.html#gada11d965c4da13090ad336e030e4d11f)
   * Note:
   *       You should finish rendering your airplane first.
   *       Otherwise you will spend a lot of time debugging this with a black screen.
   */
  
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    isSpace = true;
  } 
  else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
    isSpace = false;
  }
  if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
    isLeft = true;
  } else if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
    isLeft = false;
  }
  if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
    isRight = true;
  } else if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
    isRight = false;
  }
  if (key == GLFW_KEY_X && action == GLFW_PRESS) {
    isShoot = true;
  } else if (key == GLFW_KEY_X && action == GLFW_RELEASE) {
    isShoot = false;
  }
}

void initOpenGL() {
  // Initialize OpenGL context, details are wrapped in class.
#ifdef __APPLE__
  // MacOS need explicit request legacy support
  OpenGLContext::createContext(21, GLFW_OPENGL_ANY_PROFILE);
#else
  //OpenGLContext::createContext(21, GLFW_OPENGL_ANY_PROFILE);
  OpenGLContext::createContext(43, GLFW_OPENGL_COMPAT_PROFILE);
#endif
  GLFWwindow* window = OpenGLContext::getWindow();
  /* TODO#0: Change window title to "HW1 - `your student id`"
   *        Ex. HW1 - 312550000 
   */
  glfwSetWindowTitle(window, "HW1 - 312551115");
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, resizeCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#ifndef NDEBUG
  OpenGLContext::printSystemInfo();
  // This is useful if you want to debug your OpenGL API calls.
  OpenGLContext::enableDebugCallback();
#endif
}

void drawCube(glm::vec3 pos) { 
  float halfLength = 0.3f;
  float halfWidth = 0.3f;
  float halfHeight = 0.3f;

  float vertices[][3] = {
      {pos.x - halfLength, pos.y - halfWidth + 2.0f, pos.z - halfHeight},  // 0
      {pos.x + halfLength, pos.y - halfWidth + 2.0f, pos.z - halfHeight},  // 1
      {pos.x + halfLength, pos.y + halfWidth + 2.0f, pos.z - halfHeight},  // 2
      {pos.x - halfLength, pos.y + halfWidth + 2.0f, pos.z - halfHeight},  // 3
      {pos.x - halfLength, pos.y - halfWidth + 2.0f, pos.z + halfHeight},  // 4
      {pos.x + halfLength, pos.y - halfWidth + 2.0f, pos.z + halfHeight},  // 5
      {pos.x + halfLength, pos.y + halfWidth + 2.0f, pos.z + halfHeight},  // 6
      {pos.x - halfLength, pos.y + halfWidth + 2.0f, pos.z + halfHeight}   // 7
  };

  int faces[][4] = {
      {0, 1, 5, 4}, // down
      {1, 2, 6, 5}, // right
      {2, 3, 7, 6}, // up
      {3, 0, 4, 7}, // left
      {4, 5, 6, 7}, // foward
      {0, 3, 2, 1}  // back
  };

  float normal[][3] = {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
                       {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
  glBegin(GL_QUADS);
  
  for (int i = 0; i < 6; i++) {
    glNormal3f(normal[i][0], normal[i][1], normal[i][2]);
    for (int j = 0; j < 4; j++) {
      glVertex3f(vertices[faces[i][j]][0], vertices[faces[i][j]][1], vertices[faces[i][j]][2]);
    }
  }
  glEnd();
}

void light() {
  GLfloat light_specular[] = {0.6f, 0.6f, 0.6f, 1};
  GLfloat light_diffuse[] = {0.6f, 0.6f, 0.6f, 1};
  GLfloat light_ambient[] = {0.4f, 0.4f, 0.4f, 1};
  GLfloat light_position[] = {50.0, 75.0, 80.0, 1.0};
  // z buffer enable
  glEnable(GL_DEPTH_TEST);
  // enable lighting
  glEnable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_NORMALIZE);
  // set light property
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
}

void render_cube(glm::vec3 pos) { 
  glColor3f(RED);
  drawCube(pos);
}

int main() {
  initOpenGL();
  GLFWwindow* window = OpenGLContext::getWindow();

  // Init Camera helper
  Camera camera(glm::vec3(0, 5, 10));
  camera.initialize(OpenGLContext::getAspectRatio());
  // Store camera as glfw global variable for callbasks use
  glfwSetWindowUserPointer(window, &camera);

  // Main rendering loop
  while (!glfwWindowShouldClose(window)) {
    // Polling events.
    glfwPollEvents();
    // Update camera position and view
    camera.move(window);
    // GL_XXX_BIT can simply "OR" together to use.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /// TO DO Enable DepthTest
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera.getProjectionMatrix());
    // ModelView Matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(camera.getViewMatrix());

#ifndef DISABLE_LIGHT
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearDepth(1.0f);
    light();
#endif
    if (isRight) {
      rot += ROTATE_SPEED;
    }
    if (isLeft) {
      rot -= ROTATE_SPEED;
    }
    if (isSpace) {
      // upload pos
      float angleInRadians = glm::radians(rot);
      float forwardX = sin(angleInRadians);
      float forwardZ = -cos(angleInRadians);
      pos.x += forwardX * FLYING_SPEED;
      pos.z += forwardZ * FLYING_SPEED;
      pos.y += FLYING_SPEED;

      // make wings shake
      if (wing_rot >= 30.0f) {
        wing_down = false;
      }
      if (wing_rot <= -30.0f && wing_rot < 0.0f) {
        wing_down = true;
      }
      if (wing_down) {
        wing_rot += WING_ROTATE_SPEED;
      } else {
        wing_rot -= WING_ROTATE_SPEED;
      }
    } else if (!isSpace && pos.y >= FLYING_SPEED) {
      // sown to bottom
      pos.y -= FLYING_SPEED;

      // stop wing
      wing_rot = 0.0;
    }


    glPushMatrix();
    glRotated(-wing_rot, 0.0, 0.0, 1.0);
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        for (int k = 0; k < 3; k++) {
          glColor3f(WHITE);
          drawCube(glm::vec3(i * 0.65f, j * 0.65f, k * 0.65f));
        }
      }
    }
    glPopMatrix();

#ifdef __APPLE__
    // Some platform need explicit glFlush
    glFlush();
#endif
    glfwSwapBuffers(window);
  }
  return 0;
}
