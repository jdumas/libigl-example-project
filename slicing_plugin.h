#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/material_colors.h>
#include <imgui/imgui.h>
#include "imguizmo/ImGuizmo.h"

class SlicingPlugin : public igl::opengl::glfw::imgui::ImGuiMenu {

	igl::opengl::ViewerData data;

	virtual void init(igl::opengl::glfw::Viewer *_viewer) {
		using namespace igl;
		ImGuiMenu::init(_viewer);

		// Inline mesh of a square
		const Eigen::MatrixXd V = (Eigen::MatrixXd(4, 3) <<
			-0.5, -0.5, 0.0,
			-0.5,  0.5, 0.0,
			 0.5,  0.5, 0.0,
			 0.5, -0.5, 0.0).finished();

		const Eigen::MatrixXi F = (Eigen::MatrixXi(2, 3) <<
			0, 2, 1,
			0, 3, 2).finished();

		data.set_mesh(V, F);
		data.set_face_based(true);
		data.set_colors(Eigen::RowVector4d(149, 165, 166, 128)/255.0);
		data.show_lines = false;
	}

	virtual bool pre_draw() override {
		ImGuiMenu::pre_draw();
		ImGuizmo::BeginFrame();
		return false;
	}

	virtual bool post_draw() override {
		viewer->core.draw(data);
		ImGuiMenu::post_draw();
		return false;
	}

};
