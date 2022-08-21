#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "glad/include/glad/gl.h"
#include <GLFW/glfw3.h>

/* *****************************************************************************
 * DEFINITIONS
 **************************************************************************** */
#define WIN_WIDTH  512
#define WIN_HEIGHT 512

#define SPEED 16 /* snake actions per second */

#define GRID_COUNT_X 16
#define GRID_COUNT_Y 16

#define GRID_LINE_WIDTH 2

#define COLOR_BACKGROUND (GLfloat[3]){0.14, 0.16, 0.18}
#define COLOR_GRID       (GLfloat[3]){0.91, 0.91, 0.91}
#define COLOR_SNAKE_HEAD (GLfloat[3]){0.5, 1.0, 0.5}
#define COLOR_SNAKE_TAIL (GLfloat[3]){0.6, 0.88, 0.6}
#define COLOR_FOOD       (GLfloat[3]){1.0, 0.5, 0.5}

/* *****************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************** */
GLfloat vertices[] = {
  0.0, 0.0,
  0.0, 1.0,
  1.0, 1.0,
  1.0, 0.0
};
GLuint indices[] = {
  0, 1, 2,
  2, 3, 0
};

const GLchar *vShader_Code =
"#version 330 core\n"
"layout (location = 0) in vec2 v_pos;\n"
"uniform float POSITION;\n"
"uniform vec2 GRID_COUNT;\n"
"void main(){\n"
"  vec2 GRID_SIZE = vec2(2 / GRID_COUNT.x, 2 / GRID_COUNT.y);\n"
"  vec2 pos_center = vec2((mod(POSITION, GRID_COUNT.x) * GRID_SIZE.x) - 1.0 + (GRID_SIZE.x / 2),\n"
"    1.0 - GRID_SIZE.y - (floor(POSITION / GRID_COUNT.x) * GRID_SIZE.y) + (GRID_SIZE.y / 2));\n"
"  gl_Position = vec4(\n"
"    pos_center.x - (GRID_SIZE.x / 2) + (v_pos.x * GRID_SIZE.x),\n"
"    pos_center.y - (GRID_SIZE.y / 2) + (v_pos.y * GRID_SIZE.y),\n"
"    0.0, 1.0);\n"
//"  gl_Position = vec4(v_pos, 0.0, 1.0);\n"
"}\0";
const GLchar *fShader_Code =
"#version 330 core\n"
"out vec4 fragment_color;\n"
"uniform vec3 COLOR;\n"
"void main(){\n"
"  fragment_color = vec4(COLOR, 1.0);\n"
//"  fragment_color = vec4(1.0, 0.5, 0.5, 1.0);\n"
"}\0";

const GLchar *grid_vShader_Code =
"#version 330 core\n"
"layout (location = 0) in vec2 v_pos;\n"
"void main(){\n"
"  gl_Position = vec4(v_pos * 2 - 1, 0.5, 1.0);\n"
"}\0";
const GLchar *grid_fShader_Code =
"#version 330 core\n"
"out vec4 fragment_color;\n"
"uniform vec3 COLOR;\n"
"uniform vec2 WIN_SIZE;\n"
"uniform vec2 GRID_COUNT;\n"
"uniform float GRID_LINE_WIDTH;\n"
"void main(){\n"
"  vec2 GRID_SIZE = vec2(WIN_SIZE.x / GRID_COUNT.x, WIN_SIZE.y / GRID_COUNT.y);\n"
"  vec2 mod_val = vec2(mod(gl_FragCoord.x, GRID_SIZE.x), mod(gl_FragCoord.y, GRID_SIZE.y));\n"
//"  if(\n"
//"    (mod_val.x < GRID_LINE_WIDTH / 2 || mod_val.x > GRID_SIZE.x - GRID_LINE_WIDTH / 2)\n"
//"  )\n"
//"    fragment_color = vec4(COLOR, 1.0);\n"
//"  else if(\n"
//"    (mod_val.y < GRID_LINE_WIDTH / 2 || mod_val.y > GRID_SIZE.y - GRID_LINE_WIDTH / 2)\n"
//"  )\n"
//"    fragment_color = vec4(COLOR, 1.0);\n"
"  if(\n"
"    (mod_val.x < GRID_LINE_WIDTH / 2 || mod_val.x > GRID_SIZE.x - GRID_LINE_WIDTH / 2) &&\n"
"    (mod_val.y < GRID_LINE_WIDTH * 2 || mod_val.y > GRID_SIZE.y - GRID_LINE_WIDTH * 2)\n"
"  )\n"
"    fragment_color = vec4(COLOR, 1.0);\n"
"  else if(\n"
"    (mod_val.y < GRID_LINE_WIDTH / 2 || mod_val.y > GRID_SIZE.y - GRID_LINE_WIDTH / 2) &&\n"
"    (mod_val.x < GRID_LINE_WIDTH * 2 || mod_val.x > GRID_SIZE.x - GRID_LINE_WIDTH * 2)\n"
"  )\n"
"    fragment_color = vec4(COLOR, 1.0);\n"
"  else discard;\n"
//"  fragment_color = vec4(COLOR, 1.0);\n"
"}\0";

struct Button
{
  GLboolean down, last, pressed;
};
struct Keyboard
{
  struct Button keys[GLFW_KEY_LAST];
};
struct Keyboard keyboard = {};

enum Directions
{
  LEFT  = 1,
  RIGHT = 2,
  UP    = 4,
  DOWN  = 8,
  PAUSE = 16
};

struct Snake
{
  GLuint counter;
  GLuint positions[GRID_COUNT_X * GRID_COUNT_Y];
  GLuint direction;
};
struct Snake snake = {};
GLuint food_position = 0;

GLdouble TIME_NOW, TIME_LAST, TIME_DELTA, TIME_SUM;

/* *****************************************************************************
 * FUNCTIONS
 **************************************************************************** */
GLuint random_position(void)
{
  GLuint RND = (GLuint) (rand() % (GRID_COUNT_X * GRID_COUNT_Y));
  GLboolean free_position;
  do {
    free_position = GL_TRUE;
    for(GLuint i = 0; i < snake.counter; i++)
    {
      if(snake.positions[i] == RND)
      {
        free_position = GL_FALSE;
        RND++;
        if(RND >= (GRID_COUNT_X * GRID_COUNT_Y))
          RND %= (GRID_COUNT_X * GRID_COUNT_Y);
      }
    }
  } while(!free_position);
  return RND;
}

void snake_logic(void)
{
  /* if PAUSE -> do nothing */
  if(snake.direction & PAUSE)
    return;
  
  /* check wall hit */
  if(
    (snake.positions[0] % GRID_COUNT_X == 0
      && snake.direction == LEFT)  ||
    (snake.positions[0] % GRID_COUNT_X == GRID_COUNT_X - 1
      && snake.direction == RIGHT) ||
    (snake.positions[0] / GRID_COUNT_X == 0
      && snake.direction == UP)    ||
    (snake.positions[0] / GRID_COUNT_X == GRID_COUNT_Y - 1
      && snake.direction == DOWN)
  )
  {
    snake.counter = 1;
    snake.direction = PAUSE;
    return;
  }

  /* ELSE */
  GLuint _last_snake_head_position = snake.positions[0];
  GLuint _last_snake_length = snake.counter;

  /* update snake_head_position */
  if(snake.direction == LEFT) snake.positions[0]      -= 1;
  else if(snake.direction == RIGHT) snake.positions[0] += 1;
  else if(snake.direction == UP) snake.positions[0]   -= GRID_COUNT_X;
  else if(snake.direction == DOWN) snake.positions[0] += GRID_COUNT_X;

  /* FOOOOOD eat check */
  if(snake.positions[0] == food_position)
    snake.counter += 1;

  /* check if game won */
  if(snake.counter >= GRID_COUNT_X * GRID_COUNT_Y)
    snake.counter = 1;

  /* update snake_tail_positions */
  if(snake.counter > 2)
    for(GLuint i = 0; i < snake.counter - 2; i++)
      snake.positions[snake.counter - 1 - i] =
        snake.positions[snake.counter - 1 - i - 1];
  if(snake.counter > 1)
    snake.positions[1] = _last_snake_head_position;
  
  /* check for self destruction */
  for(GLuint i = 0; i < snake.counter - 1; i++)
    if(snake.positions[0] == snake.positions[i + 1])
      snake.counter = 1;

  /* spawn new food if it was eaten before */
  if(_last_snake_length != snake.counter)
    food_position = random_position();
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if(key < 0)
    return;

  switch(action)
  {
    case GLFW_PRESS:
      keyboard.keys[key].down = GL_TRUE;
      break;
    case GLFW_RELEASE:
      keyboard.keys[key].down = GL_FALSE;
      break;
    default:
      break;
  }
}
void _button_array_update(struct Button *buttons)
{
  for(int i = 0; i < GLFW_KEY_LAST; i++)
  {
    buttons[i].pressed = buttons[i].down && !buttons[i].last;
    buttons[i].last = buttons[i].down;
  }
}
void key_functions(GLFWwindow *window)
{
  _button_array_update(keyboard.keys);
  GLuint _last_snake_direction = snake.direction;

  if(keyboard.keys[GLFW_KEY_ESCAPE].down)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  if(keyboard.keys[GLFW_KEY_SPACE].pressed)
    snake.direction |= PAUSE;

  if(keyboard.keys[GLFW_KEY_LEFT].pressed)
    if((snake.direction != LEFT) && !(snake.direction & RIGHT))
      snake.direction = LEFT;

  if(keyboard.keys[GLFW_KEY_RIGHT].pressed)
    if(!(snake.direction & LEFT) && (snake.direction != RIGHT))
      snake.direction = RIGHT;

  if(keyboard.keys[GLFW_KEY_UP].pressed)
    if((snake.direction != UP) && !(snake.direction & DOWN))
      snake.direction = UP;

  if(keyboard.keys[GLFW_KEY_DOWN].pressed)
    if(!(snake.direction & UP) && (snake.direction != DOWN))
      snake.direction = DOWN;
  
  if(snake.direction != _last_snake_direction)
  {
    snake_logic();
    TIME_SUM = 0.0;
  }
}

/* *****************************************************************************
 * MAIN LOOP
 **************************************************************************** */
int main(int argc, char const *argv[])
{
  printf("Hi guys!\n");
  srand(time(NULL));

  if(!glfwInit())
  {
    fprintf(stderr, "ERROR: Failed init GLFW!\n");
    exit(EXIT_FAILURE);
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "dnqr.snake_clone", NULL, NULL);
  if(window == NULL)
  {
    fprintf(stderr, "ERROR: Failed creating GLFW window!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);

  if(!gladLoadGL(glfwGetProcAddress))
  {
    fprintf(stderr, "ERROR: Failed init GLAD (OpenGL)!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glEnable(GL_DEPTH_TEST);

  GLuint vShader, fShader, shader_programm;
  vShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vShader, 1, &vShader_Code, NULL);
  glCompileShader(vShader);
  fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fShader, 1, &fShader_Code, NULL);
  glCompileShader(fShader);
  shader_programm = glCreateProgram();
  glAttachShader(shader_programm, vShader);
  glAttachShader(shader_programm, fShader);
  glLinkProgram(shader_programm);
  glDeleteShader(vShader);
  glDeleteShader(fShader);

  GLuint grid_vShader, grid_fShader, grid_shader_programm;
  grid_vShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(grid_vShader, 1, &grid_vShader_Code, NULL);
  glCompileShader(grid_vShader);
  grid_fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(grid_fShader, 1, &grid_fShader_Code, NULL);
  glCompileShader(grid_fShader);
  grid_shader_programm = glCreateProgram();
  glAttachShader(grid_shader_programm, grid_vShader);
  glAttachShader(grid_shader_programm, grid_fShader);
  glLinkProgram(grid_shader_programm);
  glDeleteShader(grid_vShader);
  glDeleteShader(grid_fShader);


  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
  glEnableVertexAttribArray(0);

  snake = (struct Snake)
  {
    .counter = 1,
    .positions[0] = (GLuint) (rand() % (GRID_COUNT_X * GRID_COUNT_Y)),
    .direction = PAUSE
  };
  food_position = random_position();

  TIME_NOW = TIME_LAST = glfwGetTime();
  TIME_DELTA = TIME_SUM = 0.0;

  while(!glfwWindowShouldClose(window))
  {
    glClearColor(
      COLOR_BACKGROUND[0],
      COLOR_BACKGROUND[1],
      COLOR_BACKGROUND[2],
      1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    key_functions(window);

    TIME_NOW = glfwGetTime();
    TIME_DELTA = TIME_NOW - TIME_LAST;
    TIME_LAST = TIME_NOW;
    TIME_SUM += TIME_DELTA;
    if(TIME_SUM >= 1.0 / SPEED)
    {
      snake_logic();
      TIME_SUM = 0.0;
    }

    glBindVertexArray(VAO);
    glUseProgram(shader_programm);
    glUniform2f(
      glGetUniformLocation(shader_programm, "GRID_COUNT"),
      (GLfloat) GRID_COUNT_X,
      (GLfloat) GRID_COUNT_Y);
    /* snake */
    for(GLuint i = 0; i < snake.counter; i++)
    {
      glUniform1f(
        glGetUniformLocation(shader_programm, "POSITION"),
        snake.positions[i]);
      if(i == 0)
        glUniform3f(
          glGetUniformLocation(shader_programm, "COLOR"),
          COLOR_SNAKE_HEAD[0],
          COLOR_SNAKE_HEAD[1],
          COLOR_SNAKE_HEAD[2]);
      else
        glUniform3f(
          glGetUniformLocation(shader_programm, "COLOR"),
          COLOR_SNAKE_TAIL[0],
          COLOR_SNAKE_TAIL[1],
          COLOR_SNAKE_TAIL[2]);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    /* food */
    glUniform1f(
      glGetUniformLocation(shader_programm, "POSITION"),
      food_position);
    glUniform3f(
      glGetUniformLocation(shader_programm, "COLOR"),
      COLOR_FOOD[0],
      COLOR_FOOD[1],
      COLOR_FOOD[2]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    /* grid */
    glUseProgram(grid_shader_programm);
    glUniform3f(
      glGetUniformLocation(grid_shader_programm, "COLOR"),
      COLOR_GRID[0],
      COLOR_GRID[1],
      COLOR_GRID[2]);
    int win_width, win_height;
    glfwGetFramebufferSize(window, &win_width, &win_height);
    glUniform2f(
      glGetUniformLocation(grid_shader_programm, "WIN_SIZE"),
      (GLfloat) win_width,
      (GLfloat) win_height);
    glUniform2f(
      glGetUniformLocation(grid_shader_programm, "GRID_COUNT"),
      (GLfloat) GRID_COUNT_X,
      (GLfloat) GRID_COUNT_Y);
    glUniform1f(
      glGetUniformLocation(grid_shader_programm, "GRID_LINE_WIDTH"),
      (GLfloat) GRID_LINE_WIDTH);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
