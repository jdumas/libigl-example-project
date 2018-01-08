#include <igl/readOFF.h>
#include <igl/viewer/Viewer.h>
#include <iostream>
#include <igl/png/writePNG.h>
#include <igl/png/readPNG.h>

#include <GLFW/glfw3.h>

#ifndef __APPLE__
#  define GLEW_STATIC
#  include <GL/glew.h>
#endif

#ifdef __APPLE__
#   include <OpenGL/gl3.h>
#   define __gl_h_ /* Prevent inclusion of the old gl.h */
#else
#   include <GL/gl.h>
#endif

// This function is called every time a keyboard button is pressed
bool key_down(igl::viewer::Viewer& viewer, unsigned char key, int modifier)
{
  if (key == '1')
  {
    // Allocate temporary buffers
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> R(1280,800);
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> G(1280,800);
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> B(1280,800);
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> A(1280,800);

    // Draw the scene in the buffers
    viewer.core.draw_buffer(viewer.data,viewer.opengl,false,R,G,B,A);

    // Save it to a PNG
    igl::png::writePNG(R,G,B,A,"out.png");
  }

  if (key == '2')
  {
    // Allocate temporary buffers
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> R,G,B,A;

    // Read the PNG
    igl::png::readPNG("out.png",R,G,B,A);

    // Replace the mesh with a triangulated square
    Eigen::MatrixXd V(4,3);
    V <<
      -0.5,-0.5,0,
       0.5,-0.5,0,
       0.5, 0.5,0,
      -0.5, 0.5,0;
    Eigen::MatrixXi F(2,3);
    F <<
      0,1,2,
      2,3,0;
    Eigen::MatrixXd UV(4,2);
    UV <<
      0,0,
      1,0,
      1,1,
      0,1;

    viewer.data.clear();
    viewer.data.set_mesh(V,F);
    viewer.data.set_uv(UV);
    viewer.core.align_camera_center(V);
    viewer.core.show_texture = true;

    // Use the image as a texture
    viewer.data.set_texture(R,G,B);

  }


  return false;
}
namespace {
static void my_glfw_error_callback(int error, const char* description)
{
  fputs(description, stderr);
  fputs("\n", stderr);
}
}

int main(int argc, char *argv[])
{
  // Load a mesh in OFF format
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  igl::readOFF(TUTORIAL_SHARED_PATH "/bunny.off", V, F);

  std::cerr << "Press 1 to render the scene and save it in a png." << std::endl;
  std::cerr << "Press 2 to load the saved png and use it as a texture." << std::endl;

  // Plot the mesh and register the callback
  igl::viewer::Viewer viewer;
  viewer.data.set_mesh(V, F);
  // viewer.launch();
  // return 0;

    glfwSetErrorCallback(my_glfw_error_callback);
    if (!glfwInit()) {
      std::cout << "init failure" << std::endl;
      return EXIT_FAILURE;
    }

  //glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_OSMESA_CONTEXT_API);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

  printf("create window\n");
  GLFWwindow* offscreen_context = glfwCreateWindow(640, 480, "", NULL, NULL);
  printf("create context\n");
  glfwMakeContextCurrent(offscreen_context);
    #ifndef __APPLE__
      glewExperimental = true;
      GLenum err = glewInit();
      if(GLEW_OK != err)
      {
        /* Problem: glewInit failed, something is seriously wrong. */
       fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      }
      glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
      fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    #endif
    viewer.opengl.init();
    viewer.core.align_camera_center(viewer.data.V, viewer.data.F);
    viewer.init();

    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> R(800,800);
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> G(800,800);
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> B(800,800);
    Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> A(800,800);

    // Draw the scene in the buffers
    viewer.core.draw_buffer(viewer.data,viewer.opengl,true,R,G,B,A);

    // Save it to a PNG
    igl::png::writePNG(R,G,B,A,"out.png");
}
