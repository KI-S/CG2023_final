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
#include <sstream>
#include <string>
#include <chrono>
#include <cmath>

#define RED 0.905f, 0.298f, 0.235f
#define BLUE 0.203f, 0.096f, 0.858f
#define GREEN 0.18f, 0.8f, 0.443f
#define YELLOW 1.0f, 1.0f, 0.0f
#define WHITE 1.0f, 1.0f, 1.0f

#define COLOR_BLUE 0.403f, 0.496f, 0.858f
#define COLOR_GREEN 0.18f, 0.8f, 0.443f
#define COLOR_RED 0.905f, 0.298f, 0.235f
#define COLOR_DARK_BLUE 0.101f, 0.048f, 0.700f
#define COLOR_DARK_RED 0.452f, 0.199f, 0.117f
#define COLOR_LIGHT_GREEN 0.19f, 0.9f, 0.443f
#define COLOR_DARK_BROWN 0.55f, 0.27f, 0.08f
#define COLOR_DARK_GRAY 0.5f, 0.5f, 0.5f
#define COLOR_GOLD 1.0f, 0.843f, 0.0f

bool isClick;
bool isRightClick;

bool isReset = true;

double last_x, last_y;
double x, y;
float rot_x = 0.0f, rot_y = 0.0f, pre_rot_x = 0.0f, pre_rot_y = 0.0f;
double mouseX = 0.0;
double mouseY = 0.0;
int width, height;

// game setting
int gameSize = 3; // default = 3
int mineCount = 1; // default = 1

// init rotation degree and position information
float rot = 0.0f;
float wing_rot = 0.0f;
bool wing_down = true;
glm::vec3 pos = {0.0, 0.0, 0.0};

void applyRay(GLFWwindow* window, Camera camera, glm::vec4 &start, glm::vec4 &end);
int parseGameSetting(const std::string filename);
void rotateCoord(GLFWwindow* window);
void drawMine(glm::vec3 center, float size);
int handleClickBlock(glm::vec3 &id, Minesweeper &game, glm::vec4 start, glm::vec4 end);
void drawBoard(Minesweeper &game, glm::vec3 id);

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



bool ifIntersect(glm::vec3 center, float size, glm::vec3 rayDir, glm::vec3 rayOrigin) {
  float epsilon = 0.001f;  // 精度

  glm::vec3 normArray[6] = {glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), 
                            glm::vec3(0, -1, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, -1)};

  for (int i = 0; i < 6; ++i) {
    glm::vec3 norm = normArray[i];
    glm::vec3 faceCenter = glm::vec3(center.x + size * norm.x, center.y + size * norm.y, center.z + size * norm.z);

    float denom = glm::dot(norm, rayDir);
    if (abs(denom) > epsilon) {
      glm::vec3 rayToPlane = faceCenter - rayOrigin;
      float t = glm::dot(rayToPlane, norm) / denom;

      if (t >= 0) {
        // 計算交點
        glm::vec3 intersectionPoint = rayOrigin + t * rayDir;
        
        if (norm.x) {
          if (intersectionPoint.y <= faceCenter.y + size && intersectionPoint.y >= faceCenter.y - size &&
                  intersectionPoint.z <= faceCenter.z + size && intersectionPoint.z >= faceCenter.z - size) {
            return true;
          }
        } else if (norm.y) {
          if (intersectionPoint.x <= faceCenter.x + size && intersectionPoint.x >= faceCenter.x - size &&
                  intersectionPoint.z <= faceCenter.z + size && intersectionPoint.z >= faceCenter.z - size) {
            return true;
          }
        } else if (norm.z) {
          if (intersectionPoint.x <= faceCenter.x + size && intersectionPoint.x >= faceCenter.x - size &&
                  intersectionPoint.y <= faceCenter.y + size &&  intersectionPoint.y >= faceCenter.y - size) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

double getDistance(const glm::vec3 rayOrigin, const glm::vec3 rayEnd, const glm::vec3 boxCenter,
                              double boxHalfSize) {
  double minDistance = std::numeric_limits<double>::max();

  double min_x = boxCenter.x - boxHalfSize;
  double max_x = boxCenter.x + boxHalfSize;
  double min_y = boxCenter.y - boxHalfSize;
  double max_y = boxCenter.y + boxHalfSize;

  float start_posX = rayOrigin.x * (10.0f - boxCenter.z - 0.5f);
  float start_posY = rayOrigin.y * (10.0f - boxCenter.z - 0.5f);

  float end_posX = rayEnd.x * (10.0f - boxCenter.z - 0.5f);
  float end_posY = rayEnd.y * (10.0f - boxCenter.z - 0.5f);

  if (!ifIntersect(boxCenter, boxHalfSize, rayEnd - rayOrigin, rayOrigin))
    return minDistance;
  minDistance = glm::distance(rayOrigin, boxCenter);
  return minDistance;
}

void cursor_position_callback(double height, double width, glm::mat4 projection, glm::mat4 view, double *xpos, double *ypos) {
  float screenX = *xpos;
  float screenY = height - *ypos;

  glm::mat4 inverseProjection = glm::inverse(projection);
  glm::mat4 inverseView = glm::inverse(view);

  glm::vec4 screenPos((screenX / width - 0.5f ) * 2.0f, (screenY / height - 0.5f ) * 2.0f, -1.0f, 0.0f);

  glm::vec4 worldPos = inverseProjection * screenPos;
  //worldPos /= worldPos.w;
  worldPos = inverseView * worldPos;
  //worldPos /= worldPos.w;
  *xpos = worldPos.x;
  *ypos = worldPos.y;
  //*xpos = worldPos.x * (10 - 0.5 * gameSize - 0.05 * (gameSize - 1));
  //*ypos = worldPos.y * (10 - 0.5 * gameSize - 0.05 * (gameSize - 1));
  //std::cout << "World coordinates: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")" << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
  // There are three actions: press, release, hold(repeat)
  // if (action == GLFW_REPEAT) return;

  // Press ESC to close the window.
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  if (key == GLFW_KEY_R) {
    isReset = true;
  }
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    glfwGetCursorPos(window, &last_x, &last_y);
    isClick = true;
  } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    isClick = false;
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    glfwGetCursorPos(window, &last_x, &last_y);
    isRightClick = true;
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    isRightClick = false;
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

void drawFlag(glm::vec3 pos, float halfSize) {
  glColor3f(RED);
  glBegin(GL_TRIANGLES);
  glNormal3f(0.0, 0.0, 1.0f);
  glVertex3f(pos.x, pos.y + 0.45f * halfSize, pos.z);
  glVertex3f(pos.x, pos.y, pos.z);
  glVertex3f(pos.x + 0.38 * halfSize, pos.y + 0.225 * halfSize, pos.z);
  glEnd();

  glColor3f(COLOR_DARK_BROWN);
  glBegin(GL_QUADS);
  glNormal3f(0.0, 0.0, 1.0f);
  glVertex3f(pos.x + 0.05f * halfSize, pos.y + 0.45f * halfSize, pos.z);
  glVertex3f(pos.x - 0.05f * halfSize, pos.y + 0.45f * halfSize, pos.z);
  glVertex3f(pos.x - 0.05f * halfSize, pos.y - 0.45f * halfSize, pos.z);
  glVertex3f(pos.x + 0.05f * halfSize, pos.y - 0.45f * halfSize, pos.z);
  glEnd();
}

void drawCube(glm::vec3 pos, float halfSize) { 
  float halfLength = halfSize;
  float halfWidth = halfSize;
  float halfHeight = halfSize;

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

void drawBar(float line[4][3]) {
  glBegin(GL_QUADS);
  glNormal3f(0.0f, 0.0f, 1.0f);
  for (int i = 0; i < 4; ++i) {
    glVertex3f(line[i][0], line[i][1], line[i][2]);
  }
  glEnd();
}

void drawNum(glm::vec3 pos, std::int8_t num, float size, bool isBoard) {
  float lines[][4][3] = {
    {{(pos.x + 0.3 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y + 0.4 * size), pos.z},
     {(pos.x + 0.3 * size), (pos.y + 0.4 * size), pos.z}},
    {{(pos.x - 0.2 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y), pos.z},
     {(pos.x - 0.2 * size), (pos.y), pos.z}},
    {{(pos.x - 0.2 * size), (pos.y), pos.z},
     {(pos.x - 0.3 * size), (pos.y), pos.z},
     {(pos.x - 0.3 * size), (pos.y - 0.5 * size), pos.z},
     {(pos.x - 0.2 * size), (pos.y - 0.5 * size), pos.z}},
    {{(pos.x + 0.3 * size), (pos.y + 0.05 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y + 0.05 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y - 0.05 * size), pos.z},
     {(pos.x + 0.3 * size), (pos.y - 0.05 * size), pos.z}},
    {{(pos.x + 0.3 * size), (pos.y - 0.4 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y - 0.4 * size), pos.z},
     {(pos.x - 0.3 * size), (pos.y - 0.5 * size), pos.z},
     {(pos.x + 0.3 * size), (pos.y - 0.5 * size), pos.z}},
    {{(pos.x + 0.3 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x + 0.2 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x + 0.2 * size), (pos.y), pos.z},
     {(pos.x + 0.3 * size), (pos.y), pos.z}},
    {{(pos.x + 0.3 * size), (pos.y), pos.z},
     {(pos.x + 0.2 * size), (pos.y), pos.z},
     {(pos.x + 0.2 * size), (pos.y - 0.5 * size), pos.z},
     {(pos.x + 0.3 * size), (pos.y - 0.5 * size), pos.z}},
    {{(pos.x + 0.05 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x - 0.05 * size), (pos.y + 0.5 * size), pos.z},
     {(pos.x - 0.05 * size), (pos.y - 0.5 * size), pos.z},
     {(pos.x + 0.05 * size), (pos.y - 0.5 * size), pos.z}},
  };

  //glBegin(GL_QUADS);
  //glNormal3f(0.0f, 0.0f, 1.0f);
  // glTranslatef();
  switch (num) {
    case 0:
      drawBar(lines[1]);
      drawBar(lines[0]);
      drawBar(lines[5]);
      drawBar(lines[6]);
      drawBar(lines[2]);
      drawBar(lines[4]);
      break;
    case 1:
      if (isBoard) glColor3d(COLOR_BLUE);
      drawBar(lines[7]);
      break;
    case 2:
      if (isBoard) glColor3d(COLOR_GREEN);
      drawBar(lines[0]);
      drawBar(lines[5]);
      drawBar(lines[3]);
      drawBar(lines[2]);
      drawBar(lines[4]);
      break;
    case 3:
      if (isBoard) glColor3d(COLOR_RED);
      drawBar(lines[0]);
      drawBar(lines[5]);
      drawBar(lines[6]);
      drawBar(lines[4]);
      drawBar(lines[3]);
      break;
    case 4:
      if (isBoard) glColor3d(COLOR_DARK_BLUE);
      drawBar(lines[1]);
      drawBar(lines[3]);
      drawBar(lines[6]);
      drawBar(lines[5]);
      break;
    case 5:
      if (isBoard) glColor3d(COLOR_DARK_RED);
      drawBar(lines[0]);
      drawBar(lines[1]);
      drawBar(lines[3]);
      drawBar(lines[6]);
      drawBar(lines[4]);
      break;
    case 6:
      if (isBoard) glColor3d(COLOR_LIGHT_GREEN);
      drawBar(lines[0]);
      drawBar(lines[1]);
      drawBar(lines[2]);
      drawBar(lines[4]);
      drawBar(lines[6]);
      drawBar(lines[3]);
      break;
    case 7:
      if (isBoard) glColor3d(COLOR_DARK_BROWN);
      drawBar(lines[1]);
      drawBar(lines[0]);
      drawBar(lines[5]);
      drawBar(lines[6]);
      break;
    case 8:
      if (isBoard) glColor3d(COLOR_DARK_GRAY);
      drawBar(lines[1]);
      drawBar(lines[0]);
      drawBar(lines[5]);
      drawBar(lines[6]);
      drawBar(lines[2]);
      drawBar(lines[4]);
      drawBar(lines[3]);
      break;
    case 9:
      if (isBoard) glColor3d(WHITE);
      drawBar(lines[3]);
      drawBar(lines[1]);
      drawBar(lines[0]);
      drawBar(lines[5]);
      drawBar(lines[6]);
      drawBar(lines[4]);
      break;
    default:
      break;
  }
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

void displayDecimal(glm::vec3 center, int num, float size) { 
    std::vector<int> nums;
    while (1) {
      nums.push_back(num % 10);
      num /= 10;
      if (!num) {
        break;
      }
    }

    glPushMatrix();
    glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
    glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);

    for (int i = nums.size() - 1; i >= 0; --i) {
      glm::vec3 cent = center;
      cent.x += size * (nums.size() - 1 - i);
      glColor3f(COLOR_GOLD);
      drawNum(cent, nums[i], size, false);
    }   

    glPopMatrix();
}

void displayMineCount() { 
    int mineNum = mineCount; 

    glPushMatrix();
    glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
    glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);

    glColor3f(COLOR_DARK_GRAY);

    drawMine(glm::vec3(-6.5, 3.5, 0.0), 0.3);

    glPopMatrix();

    displayDecimal(glm::vec3(-5.7, 3.5, 0.0), mineNum, 0.5);
}

void displayFlagCount(Minesweeper game) { 
  int flagNum = game.flagCount(); 
    
  glPushMatrix();
  glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
  glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);

  glColor3f(COLOR_DARK_GRAY);

  drawFlag(glm::vec3(-6.5, 2.5, 0.0), 0.5);

  glPopMatrix();

  displayDecimal(glm::vec3(-5.7, 2.5, 0.0), flagNum, 0.5);
}

std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point now;
void displayTimer(Minesweeper game) {
  if (!game.checkLose() && !game.checkWin())
    now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start);

  glPushMatrix();
  glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
  glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);

  glColor3f(1.0f, 1.0f, 1.0f);
  glBegin(GL_TRIANGLE_FAN);
  glNormal3f(0, 0, 1);
  glVertex3f(-6.5f, 1.5f, 0.0f);  // 圆心
  for (int i = 0; i <= 360; ++i) {
    glVertex3f(cos(i * 3.14159 / 180.0f) * 0.3f - 6.5f, sin(i * 3.14159 / 180.0f) * 0.3f + 1.5f, 0.0f);
  }
  glEnd();

  float angle = (duration.count() % 60) * 6.0f;

  glColor3f(COLOR_DARK_BROWN);
  glBegin(GL_TRIANGLES);
  glNormal3f(0, 0, 1);
  glVertex3f(-6.5f + sin(angle * 3.14159 / 180.0f) * 0.3f, 1.5f + cos(angle * 3.14159 / 180.0f) * 0.3f, 0.0f);
  glVertex3f(-6.5f + sin((angle - 90.0) * 3.14159 / 180.0f) * 0.025f,
             1.5f + cos((angle - 90.0) * 3.14159 / 180.0f) * 0.025f, 0.0f);
  glVertex3f(-6.5f + sin((angle + 90.0) * 3.14159 / 180.0f) * 0.025f,
             1.5f + cos((angle + 90.0) * 3.14159 / 180.0f) * 0.025f,
             0.0f);
  glEnd();

  angle = (duration.count() / 60) * 6.0f;
  glColor3f(COLOR_LIGHT_GREEN);
  glBegin(GL_TRIANGLES);
  glNormal3f(0, 0, 1);
  glVertex3f(-6.5f + sin(angle * 3.14159 / 180.0f) * 0.2f, 1.5f + cos(angle * 3.14159 / 180.0f) * 0.2f, 0.0f);
  glVertex3f(-6.5f + sin((angle - 90.0) * 3.14159 / 180.0f) * 0.025f,
             1.5f + cos((angle - 90.0) * 3.14159 / 180.0f) * 0.025f, 0.0f);
  glVertex3f(-6.5f + sin((angle + 90.0) * 3.14159 / 180.0f) * 0.025f,
             1.5f + cos((angle + 90.0) * 3.14159 / 180.0f) * 0.025f, 0.0f);
  glEnd();

  glPopMatrix();

  displayDecimal(glm::vec3(-5.7, 1.5, 0.0), duration.count(), 0.5);
}

int main() {
  initOpenGL();
  GLFWwindow* window = OpenGLContext::getWindow();
  glfwGetWindowSize(window, &width, &height);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  glm::vec3 ray_direction = {0.0, 0.0, 0.0};

  // Init Camera helper
  Camera camera(glm::vec3(0, 0, 10));
  camera.initialize(OpenGLContext::getAspectRatio());
  // Store camera as glfw global variable for callbasks use
  glfwSetWindowUserPointer(window, &camera);

  // read setting file
  if (parseGameSetting("setting.txt") == -1) {
    std::cout << "Read file failed" << std::endl;
    gameSize = 3;
    mineCount = 1;
  }

  // Initialize mineswapper
  Minesweeper game = Minesweeper(gameSize, gameSize, gameSize, mineCount);

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
    // use mouse to rotate
    rotateCoord(window);

    // apply ray
    glm::vec4 cameraPos;
    glm::vec4 cursorPos;
    applyRay(window, camera, cameraPos, cursorPos);

    if (isReset) {
      isReset = false;
      start = std::chrono::steady_clock::now();
      game = Minesweeper(gameSize, gameSize, gameSize, mineCount);
    }

    //displayDecimal(glm::vec3(0, 4, 0), 123, 0.8);
    displayTimer(game);
    displayMineCount();
    displayFlagCount(game);

    glm::vec3 id(-1, -1, -1);
    handleClickBlock(id, game, cameraPos, cursorPos);
    drawBoard(game, id);

#ifdef __APPLE__
    // Some platform need explicit glFlush
    glFlush();
#endif
    glfwSwapBuffers(window);
  }
  return 0;
}

void applyRay(GLFWwindow* window, Camera camera, glm::vec4& start, glm::vec4& end) {
  glPushMatrix();
  glfwGetCursorPos(window, &mouseX, &mouseY);
  cursor_position_callback(height, width, camera.getProjection(), camera.getView(), &mouseX, &mouseY);
  //std::cout << "x: " << mouseX << " y: " << mouseY << std::endl;


  glm::mat4 trans(1.0f);
  trans = glm::rotate(trans, glm::radians(-normalRot(rot_y + pre_rot_y)), glm::vec3(0.0, 1.0, 0.0));
  trans = glm::rotate(trans, glm::radians(-normalRot(rot_x + pre_rot_x)), glm::vec3(1.0, 0.0, 0.0));
  start = (trans * glm::vec4(0.0f, 0.0f, 10.0, 1.0));
  end = (trans * glm::vec4(mouseX, mouseY, 9.0, 1.0));
  glm::vec3 tmp =
      glm::vec3((end.x - start.x) * 10 + start.x, (end.y - start.y) * 10 + start.y, (end.z - start.z) * 10 + start.z);

  glPopMatrix();
}

int parseGameSetting(const std::string filename) {
  std::ifstream file("setting.txt");

  if (!file.is_open()) {
    std::cout << "file name should be " << filename << std::endl;
    return -1;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string key;
    int value;

    if (iss >> key >> value) {
      if (key == "GAME_SIZE") {
        std::cout << "game size : " << value << std::endl;
        gameSize = value;
      } else if (key == "MINE_COUNT") {
        std::cout << "mine count : " << value << std::endl;
        mineCount = value;
      } else {
        std::cerr << "undefine key : " << key << std::endl;
      }
    }
  }

  file.close();

  return 0;
}

void rotateCoord(GLFWwindow* window) {
  float speed = 0.1f;
  static glm::vec3 up(0, 1, 0);
  if (isClick) {
    glfwGetCursorPos(window, &x, &y);
    rot_y = normalRot((float)(x - last_x) * speed);
    rot_x = normalRot((float)(y - last_y) * speed);

    // ray_direction = GetRayFromMouse(camera, mouseX, mouseY);
  } else {
    pre_rot_x += rot_x;
    pre_rot_y += rot_y;
    pre_rot_x = normalRot(pre_rot_x);
    pre_rot_y = normalRot(pre_rot_y);
    rot_x = 0.0f;
    rot_y = 0.0f;
  }
  if ((pre_rot_x >= 90.0f && pre_rot_x < 270.0f) || (pre_rot_x <= -90.0f && pre_rot_x > -270.0f)) {
    rot_y = -rot_y;
  }
  glRotatef(normalRot(rot_x + pre_rot_x), 1.0, 0.0, 0.0);
  glRotatef(normalRot(rot_y + pre_rot_y), 0.0, 1.0, 0.0);

  
  //glm::vec3 rotDir = glm::cross(glm::vec3(rot_x, rot_y, 0), glm::vec3(pre_rot_x, pre_rot_y, 1));
  //glRotatef(rotDir.length(), rotDir.x, rotDir.y, rotDir.z);
}

void drawCone(float size) {
  glBegin(GL_TRIANGLES);
  float height = 6 * size;
  for (int i = 0; i < 360; i += 10) {
    float x1 = size * cos(i * M_PI / 180.0);
    float y1 = size * sin(i * M_PI / 180.0);
    float x2 = size * cos((i + 10) * M_PI / 180.0);
    float y2 = size * sin((i + 10) * M_PI / 180.0);

    // 側面三角形
    float nx1 = cos(i * M_PI / 180.0);
    float ny1 = sin(i * M_PI / 180.0);
    float nx2 = cos((i + 10) * M_PI / 180.0);
    float ny2 = sin((i + 10) * M_PI / 180.0);

    glNormal3f(nx1 + nx2, ny1 + ny2, height);
    glVertex3f(0.0, 0.0, height);
    glVertex3f(x1, y1, 0.0);
    glVertex3f(x2, y2, 0.0);
  }
  glEnd();
}

void drawBoard(Minesweeper &game, glm::vec3 id) {
  bool stopGame = false;
  if (game.checkLose() == true || game.checkWin()) {
    stopGame = true;
    //std::cout << "lose!" << std::endl;
  }

  float maxWidth = 5.0f;
  auto status = game.getStatus();
  auto board = game.getBoard();
  int mid = gameSize / 2;
  int first = -mid;
  int last = gameSize - mid;
  float areaWidth = maxWidth / gameSize;
  float blockWidth = areaWidth * 0.9f;
  //std::cout << blockWidth << ' ';

  //game.printStatus();
  for (int i = 0; i < gameSize; ++i) {
    for (int j = 0; j < gameSize; ++j) {
      for (int k = 0; k < gameSize; ++k) {
        glm::vec3 center((i - (gameSize - 1) / 2.0f) * areaWidth,
                         (j - (gameSize - 1) / 2.0f) * areaWidth,
                         (k - (gameSize - 1) / 2.0f) * areaWidth);
        switch (status[i][j][k]) {
          case Minesweeper::Unvisit:
            if (stopGame && board[i][j][k] == -1) {
              glColor3f(COLOR_DARK_GRAY);
              drawMine(center, areaWidth / 2);
              break;
            }   
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            if (id.x == i && id.y == j && id.z == k) {
              glColor4f(YELLOW, 0.8f);
            } else {
              glColor4f(WHITE, 0.8f);
            }
            drawCube(center, blockWidth / 2);
            glDisable(GL_BLEND);
            break;
          case Minesweeper::Flagged:
            glPushMatrix();
            glTranslatef(center.x, center.y, center.z);
            glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
            glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);
            glTranslatef(-center.x, -center.y, -center.z);
            drawFlag(glm::vec3(center), areaWidth / 2);
            glPopMatrix();
            break;
          case Minesweeper::Visited:
            if (board[i][j][k] > 0) {
                glPushMatrix();
                glTranslatef(center.x, center.y, center.z);
                glRotatef(normalRot(-(rot_y + pre_rot_y)), 0.0, 1.0, 0.0);
                glRotatef(normalRot(-(rot_x + pre_rot_x)), 1.0, 0.0, 0.0);
                glTranslatef(-center.x, -center.y, -center.z);
                glColor3f(GREEN);
                drawNum(center, static_cast<int>(board[i][j][k]), areaWidth / 2, true);
                glPopMatrix();
            }
            if (board[i][j][k] == -1) {
              glColor3f(RED);
              drawMine(center, areaWidth / 2);
            }
            break;
          default:
            std::cout << static_cast<int>(status[i][j][k]) << " why?" << std::endl;
            break;
        }
      }
    }
  }
}

void drawMine(glm::vec3 center, float size) { 
  // 繪製地雷主體
  glPushMatrix();
  glTranslatef(center.x, center.y, center.z);
  int stacks = 64;
  int slices = 64;
  float radius = 0.7f * size;
  for (int i = 0; i <= stacks; ++i) {
    double lat0 = M_PI * (-0.5 + (double)i / stacks);
    double z0 = sin(lat0);
    double zr0 = cos(lat0);

    double lat1 = M_PI * (-0.5 + (double)(i - 1) / stacks);
    double z1 = sin(lat1);
    double zr1 = cos(lat1);

    glBegin(GL_QUAD_STRIP);
    for (int j = 0; j <= slices; ++j) {
      double lng = 2 * M_PI * (double)(j - 1) / slices;
      double x = cos(lng);
      double y = sin(lng);

      glNormal3f(x * zr0, y * zr0, z0);
      glVertex3f(radius * x * zr0, radius * y * zr0, radius * z0);

      glNormal3f(x * zr1, y * zr1, z1);
      glVertex3f(radius * x * zr1, radius * y * zr1, radius * z1);
    }
    glEnd();
  }
  glPopMatrix();

  // 繪製地雷上的尖刺，可以根據需要增減尖刺的數量和位置
  int dir[][3] = {{1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {-1, 1, 0}};
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 8; ++j) {
      float x = 0.2 * cos(45 * j);
      float y = 0.2 * sin(45 * j);
      glPushMatrix();
      glTranslatef(center.x, center.y, center.z);
      glRotatef(j * 45, dir[i][0], dir[i][1], dir[i][2]);
      drawCone(size * 0.2f);
      glPopMatrix();
    }
  }
}


int handleClickBlock(glm::vec3& getId, Minesweeper &game, glm::vec4 start, glm::vec4 end) {
  if (game.checkLose() || game.checkWin()) {
    return - 1;
  }
  static bool flg = false;
  static bool r_flg = false;
  static glm::vec3 lastId(999, 999, 999);
  glm::vec3 id(999, 999, 999); 
  float minDistance = std::numeric_limits<float>::max();

  float maxWidth = 5.0f;
  auto status = game.getStatus();
  auto board = game.getBoard();
  float areaWidth = maxWidth / gameSize;
  float blockWidth = areaWidth * 0.9f;
  
  for (int i = 0; i < gameSize; ++i) {
    for (int j = 0; j < gameSize; ++j) {
      for (int k = 0; k < gameSize; ++k) {
        if (status[i][j][k] != Minesweeper::Visited) {
          glm::vec3 center((i - (gameSize - 1) / 2.0f) * areaWidth, (j - (gameSize - 1) / 2.0f) * areaWidth,
                           (k - (gameSize - 1) / 2.0f) * areaWidth);
          float tmp = getDistance(start, end, center, blockWidth / 2.0f);
          if (tmp < minDistance) {
            minDistance = tmp;
            id = glm::vec3(i, j, k); /*
            switch (status[i][j][k]) {
              case Minesweeper::Unvisit:
                std::cout << "0 ";
                break;
              case Minesweeper::Flagged:
                std::cout << "F ";
                break;
              case Minesweeper::Visited:
                std::cout << "1 ";
                break;
              default:
                std::cout << "Bugged ";
                break;
            }*/
          }
        }
      }
    }
  }
  getId = id;
  // click on block 
  if (isClick) {
    if (flg == false) {
      lastId = id;
    }
    flg = true;
  }
  // release on block
  if (flg && !isClick) {
    flg = false;
    if (id.x >= gameSize || id.y >= gameSize || id.z >= gameSize) {
      return -99999;
    }

    return id == lastId && game.selectTile(id.x, id.y, id.z);
  }

  // Flag
  if (isRightClick) {
    if (r_flg == false) {
      lastId = id;
    }
    r_flg = true;
  }
  // release on block
  if (r_flg && !isRightClick) {
    r_flg = false;
    if (id.x >= gameSize || id.y >= gameSize || id.z >= gameSize) {
      return -99999;
    }

    return id == lastId && game.flagTile(id.x, id.y, id.z);
  }
  return -2;
}