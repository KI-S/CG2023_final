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
#include "minesweeper.h"

#include <iostream>
#include <fstream>
#include <string>

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

bool isClick;
double last_x, last_y;
double x, y;
float rot_x = 0.0f, rot_y = 0.0f, pre_rot_x = 0.0f, pre_rot_y = 0.0f;
double mouseX = 0.0;
double mouseY = 0.0;
int width, height;

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

float normalRot(float rot) {
  while (rot >= 360.0f) {
    rot -= 360.0f;
  }
  while (rot <= -360.0f) {
    rot += 360.0f;
  }
  return rot;
}

void keyCallback(GLFWwindow* window, int key, int, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  
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
}

double getDistance(const glm::vec3 rayOrigin, const glm::vec3 rayEnd, const glm::vec3 boxCenter,
                              double boxHalfSize) {
  double minDistance = std::numeric_limits<double>::max();

  double min_x = boxCenter.x - boxHalfSize;
  double max_x = boxCenter.x + boxHalfSize;
  double min_y = boxCenter.y - boxHalfSize;
  double max_y = boxCenter.y + boxHalfSize;

  if ((rayOrigin.x < min_x && rayEnd.x < min_x) || (rayOrigin.y < min_y && rayEnd.y < min_y) ||
      (rayOrigin.x > max_x && rayEnd.x > max_x) || (rayOrigin.y > max_y && rayEnd.y > max_y)) {
    return minDistance;
  }
  minDistance = glm::distance(rayOrigin, boxCenter);
  return minDistance;
}

void cursor_position_callback(double height, double width, glm::mat4 projection, glm::mat4 view, double* xpos, double* ypos) {
  float screenX = *xpos;
  float screenY = height - *ypos;

  glm::mat4 inverseProjection = glm::inverse(projection);
  glm::mat4 inverseView = glm::inverse(view);

  glm::vec4 screenPos((screenX / width - 0.5f ) * 2.0f, (screenY / height - 0.5f ) * 2.0f, -1.0f, 0.0f);

  glm::vec4 worldPos = inverseProjection * screenPos;
  //worldPos /= worldPos.w;
  worldPos = inverseView * worldPos;
  //worldPos /= worldPos.w;
  *xpos = worldPos.x * 8.4;
  *ypos = worldPos.y * 8.4;
  //std::cout << "World coordinates: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
     glfwGetCursorPos(window, &last_x, &last_y);
     isClick = true;
  } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    isClick = false;
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
  glfwSetWindowTitle(window, "final project");
  glfwSetKeyCallback(window, keyCallback);
  glfwSetMouseButtonCallback(window, mouse_callback);
  glfwSetFramebufferSizeCallback(window, resizeCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#ifndef NDEBUG
  OpenGLContext::printSystemInfo();
  // This is useful if you want to debug your OpenGL API calls.
  OpenGLContext::enableDebugCallback();
#endif
}

void drawCube(glm::vec3 pos) { 
  float halfLength = 0.5f;
  float halfWidth = 0.5f;
  float halfHeight = 0.5f;

  float vertices[][3] = {
      {pos.x - halfLength, pos.y - halfWidth, pos.z - halfHeight},  // 0
      {pos.x + halfLength, pos.y - halfWidth, pos.z - halfHeight},  // 1
      {pos.x + halfLength, pos.y + halfWidth, pos.z - halfHeight},  // 2
      {pos.x - halfLength, pos.y + halfWidth, pos.z - halfHeight},  // 3
      {pos.x - halfLength, pos.y - halfWidth, pos.z + halfHeight},  // 4
      {pos.x + halfLength, pos.y - halfWidth, pos.z + halfHeight},  // 5
      {pos.x + halfLength, pos.y + halfWidth, pos.z + halfHeight},  // 6
      {pos.x - halfLength, pos.y + halfWidth, pos.z + halfHeight}   // 7
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

void drawNum(glm::vec3 pos, std::int8_t num) {
  float lines[][4][3] = {
      {{pos.x + 0.3, pos.y + 0.5, pos.z},
       {pos.x - 0.3, pos.y + 0.5, pos.z},
       {pos.x - 0.3, pos.y + 0.4, pos.z},
       {pos.x + 0.3, pos.y + 0.4, pos.z}},
      {{pos.x - 0.2, pos.y + 0.5, pos.z},
       {pos.x - 0.3, pos.y + 0.5, pos.z},
       {pos.x - 0.3, pos.y, pos.z},
       {pos.x - 0.2, pos.y, pos.z}},
      {{pos.x - 0.2, pos.y, pos.z},
       {pos.x - 0.3, pos.y, pos.z},
       {pos.x - 0.3, pos.y - 0.5, pos.z},
       {pos.x - 0.2, pos.y - 0.5, pos.z}},
      {{pos.x + 0.3, pos.y + 0.05, pos.z},
       {pos.x - 0.3, pos.y + 0.05, pos.z},
       {pos.x - 0.3, pos.y - 0.05, pos.z},
       {pos.x + 0.3, pos.y - 0.05, pos.z}},
      {{pos.x + 0.3, pos.y - 0.4, pos.z},
       {pos.x - 0.3, pos.y - 0.4, pos.z},
       {pos.x - 0.3, pos.y - 0.5, pos.z},
       {pos.x + 0.3, pos.y - 0.5, pos.z}},
      {{pos.x + 0.3, pos.y + 0.5, pos.z},
       {pos.x + 0.2, pos.y + 0.5, pos.z},
       {pos.x + 0.2, pos.y, pos.z},
       {pos.x + 0.3, pos.y, pos.z}},
      {{pos.x + 0.3, pos.y, pos.z},
       {pos.x + 0.2, pos.y, pos.z},
       {pos.x + 0.2, pos.y - 0.5, pos.z},
       {pos.x + 0.3, pos.y - 0.5, pos.z}},
  };

  glBegin(GL_QUADS);
  glNormal3f(0.0f, 0.0f, 1.0f);
  if (num == 1) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[5][i][0], lines[5][i][1], lines[5][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[6][i][0], lines[6][i][1], lines[6][i][2]);
    }
  } else if (num == 2) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[0][i][0], lines[0][i][1], lines[0][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[5][i][0], lines[5][i][1], lines[5][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[3][i][0], lines[3][i][1], lines[3][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[2][i][0], lines[2][i][1], lines[2][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[4][i][0], lines[4][i][1], lines[4][i][2]);
    }
  } else if (num == 3) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[0][i][0], lines[0][i][1], lines[0][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[5][i][0], lines[5][i][1], lines[5][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[6][i][0], lines[6][i][1], lines[6][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[4][i][0], lines[4][i][1], lines[4][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[3][i][0], lines[3][i][1], lines[3][i][2]);
    }
  } else if (num == 4) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[1][i][0], lines[1][i][1], lines[1][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[3][i][0], lines[3][i][1], lines[3][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[6][i][0], lines[6][i][1], lines[6][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[5][i][0], lines[5][i][1], lines[5][i][2]);
    }
  } else if (num == 5) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[0][i][0], lines[0][i][1], lines[0][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[1][i][0], lines[1][i][1], lines[1][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[3][i][0], lines[3][i][1], lines[3][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[6][i][0], lines[6][i][1], lines[6][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[4][i][0], lines[4][i][1], lines[4][i][2]);
    }
  } else if (num == 6) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[0][i][0], lines[0][i][1], lines[0][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[1][i][0], lines[1][i][1], lines[1][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[2][i][0], lines[2][i][1], lines[2][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[4][i][0], lines[4][i][1], lines[4][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[6][i][0], lines[6][i][1], lines[6][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[3][i][0], lines[3][i][1], lines[3][i][2]);
    }
  } else if (num == 7) {
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[1][i][0], lines[1][i][1], lines[1][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[0][i][0], lines[0][i][1], lines[0][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[5][i][0], lines[5][i][1], lines[5][i][2]);
    }
    for (int i = 0; i < 4; i++) {
      glVertex3f(lines[6][i][0], lines[6][i][1], lines[6][i][2]);
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


/* int main() { 
  Minesweeper game = Minesweeper(3, 3, 3, 1);
  game.flagTile(0, 1, 0);
  game.flagTile(1, 1, 0);
  game.flagTile(1, 0, 0);
  game.flagTile(0, 1, 1);
  game.flagTile(1, 1, 1);
  game.flagTile(1, 0, 1);
  game.flagTile(0, 0, 1);
  game.selectTile(0, 0, 0);
  game.printBoard();
  game.printActualBoard();
  game.printStatus();
  return 0; 
}*/

int main() {
  initOpenGL();
  GLFWwindow* window = OpenGLContext::getWindow();
  glfwGetWindowSize(window, &width, &height);

  glm::vec3 ray_direction = {0.0, 0.0, 0.0};

  // Init Camera helper
  Camera camera(glm::vec3(0, 0, 10));
  camera.initialize(OpenGLContext::getAspectRatio());
  // Store camera as glfw global variable for callbasks use
  glfwSetWindowUserPointer(window, &camera);

  // read setting file
  std::ifstream set_file("setting.txt");
  if (set_file.is_open()) {
    while (set_file) {
      std::string line;
      std::getline(set_file, line);
    }
  }

  // Initisl mineswapper
  enum TileStatus : int8_t { Unvisit, Flagged, Visited };
  Minesweeper game = Minesweeper(3, 3, 3, 1);
  std::vector<std::vector<std::vector<std::int8_t>>> board = game.getBoard();
  std::vector<std::vector<std::vector<Minesweeper::TileStatus>>> status = game.getStatus();

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
    glm::vec3 half_size = {0.5, 0.5, 0.5};
    glfwGetCursorPos(window, &mouseX, &mouseY);
    cursor_position_callback(height, width, camera.getProjection(), camera.getView(), &mouseX, &mouseY);
    //std::cout << mouseX << ", " << mouseY << "\n" << std::endl;

    if (isClick) {
      glfwGetCursorPos(window, &x, &y);
      rot_y = normalRot((float)(x - last_x) * 0.1f);
      rot_x = normalRot((float)(y - last_y) * 0.1f);

      //ray_direction = GetRayFromMouse(camera, mouseX, mouseY);
    } else {
      pre_rot_x += rot_x;
      pre_rot_y += rot_y;
      pre_rot_x = normalRot(pre_rot_x);
      pre_rot_y = normalRot(pre_rot_y);
      rot_x = 0.0f;
      rot_y = 0.0f;
    }

    //glPushMatrix();
    //glTranslated(0.0, 0.0, -1.0);
    
    //glPopMatrix();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if ((pre_rot_x >= 90.0f && pre_rot_x < 270.0f) || (pre_rot_x <= -90.0f && pre_rot_x > -270.0f)) {
      rot_y = -rot_y;
    }
    
    //glTranslated(0.0, 0.0, -10.0);

    glRotatef(normalRot(rot_x + pre_rot_x), 1.0, 0.0, 0.0);
    glRotatef(normalRot(rot_y + pre_rot_y), 0.0, 1.0, 0.0);

    glPushMatrix();
    //glTranslated(0.0, 0.0, 1.6);
    glm::mat4 trans(1.0f);
    trans = glm::rotate(trans, glm::radians(-normalRot(rot_y + pre_rot_y)), glm::vec3(0.0, 1.0, 0.0));
    trans = glm::rotate(trans, glm::radians(-normalRot(rot_x + pre_rot_x)), glm::vec3(1.0, 0.0, 0.0));
    glm::vec4 start = (trans * glm::vec4(mouseX, mouseY, 5.0, 1.0));
    glm::vec4 end = (trans * glm::vec4(mouseX, mouseY, -5.0, 1.0));
    glColor3d(RED);
    glBegin(GL_LINES);
    glVertex3d(start.x, start.y, start.z);
    glVertex3d(end.x, end.y, end.z);
    glEnd();
    glPopMatrix();
    int nearest_id = -1000;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = -1; i < 2; ++i) {
      for (int j = -1; j < 2; ++j) {
        for (int k = -1; k < 2; ++k) {
          // Calculate cube's position
          glm::vec3 cubePos = glm::vec3(i * 1.1f, j * 1.1f, k * 1.1f);

          double distance = getDistance(glm::vec3(start.x, start.y, start.z),
                                        glm::vec3(end.x, end.y, end.z), cubePos, 0.5);
          if (distance < minDistance) {
            minDistance = distance;
            nearest_id = i * 100 + j * 10 + k;
          }
        }
      }
    }

    for (int i = -1; i < 2; i++) {
      for (int j = -1; j < 2; j++) {
        for (int k = -1; k < 2; k++) {
          if ((i * 100 + j * 10 + k) != nearest_id) {
            glColor3f(WHITE);
          } else {
            glColor3f(YELLOW);

          }
          if (i == 1 && j == 1 && k == 1) {
            glPushMatrix();
            glTranslatef(i * 1.1f, j * 1.1f, k * 1.1f);
            glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
            glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);
            glTranslatef(-i * 1.1f, -j * 1.1f, -k * 1.1f);
            glColor3f(GREEN);
            drawNum(glm::vec3(i * 1.1f, j * 1.1f, k * 1.1f), 2);
            glPopMatrix();
          } else {
            drawCube(glm::vec3(i * 1.1f, j * 1.1f, k * 1.1f));
          }
        }
      }
    }

    for (std::int8_t i = -3; i < 4; i++) {
      glPushMatrix();
      glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
      glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);
      //glTranslatef();
      glColor3f(YELLOW);
      drawNum(glm::vec3(i * 1.1f, 3.0f, 1.1f), (i + 4));
      glPopMatrix();
    }
#ifdef __APPLE__
    // Some platform need explicit glFlush
    glFlush();
#endif
    glfwSwapBuffers(window);
  }
  return 0;
}
