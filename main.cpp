#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <imgui/imgui.h>
#include "menu_extra.h"
#include "slicing_plugin.h"

int main(int argc, char *argv[])
{
  // Inline mesh of a cube
  const Eigen::MatrixXd V = (Eigen::MatrixXd(8,3)<<
    0.0,0.0,0.0,
    0.0,0.0,1.0,
    0.0,1.0,0.0,
    0.0,1.0,1.0,
    1.0,0.0,0.0,
    1.0,0.0,1.0,
    1.0,1.0,0.0,
    1.0,1.0,1.0).finished();
  const Eigen::MatrixXi F = (Eigen::MatrixXi(12,3)<<
    1,7,5,
    1,3,7,
    1,4,3,
    1,2,4,
    3,8,7,
    3,4,8,
    5,7,8,
    5,8,6,
    1,5,6,
    1,6,2,
    2,6,8,
    2,8,4).finished().array()-1;

  // Plot the mesh
  igl::opengl::glfw::Viewer viewer;
  viewer.load_mesh_from_file(std::string("/home/jdumas/external/git/libigl/tutorial/shared") + "/" + "bunny_unit.off");
  viewer.data().set_mesh(V, F);
  viewer.data().set_face_based(true);

  // Custom menu
  SlicingPlugin menu;
  viewer.plugins.push_back(&menu);
  // menu.draw_viewer_menu_func = [&]() {
  //   menu.draw_viewer_menu();

  //   static Eigen::Matrix4f matrix = Eigen::Matrix4f::Identity();
  //   Eigen::Matrix4f view = viewer.core.view * viewer.core.model;
  //   Eigen::Matrix4f proj = viewer.core.proj;
  //   // ImGuizmo::DrawCube(view.data(), proj.data(), matrix.data());
  //   ImGuizmo::EditTransform(view.data(), proj.data(), matrix.data());
  // };

  viewer.resize(1024, 1024);
  viewer.launch();
}
