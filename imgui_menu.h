#pragma once

////////////////////////////////////////////////////////////////////////////////
#include <igl/viewer/Viewer.h>
#include <igl/viewer/ViewerPlugin.h>
#include <igl/project.h>
#include <imgui/imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <GLFW/glfw3.h>
////////////////////////////////////////////////////////////////////////////////

class ImGuiMenuBase : public igl::viewer::ViewerPlugin {
protected:
	// Text rendering
	float m_PixelRatio;

public:
	virtual void init(igl::viewer::Viewer *_viewer) override {
		ViewerPlugin::init(_viewer);
		// Setup ImGui binding
		if (_viewer) {
			ImGui_ImplGlfwGL3_Init(_viewer->window, false);
		}
	}

	virtual void shutdown() override {
		// Cleanup
		ImGui_ImplGlfwGL3_Shutdown();
	}

	virtual bool pre_draw() override {
		glfwPollEvents();
		ImGui_ImplGlfwGL3_NewFrame();
		return false;
	}

	virtual bool post_draw() override {
		draw_menu();
		ImGui::Render();
		return false;
	}

	virtual void post_resize(int width, int height) override {
		ImGui::GetIO().DisplaySize.x = float(width);
		ImGui::GetIO().DisplaySize.y = float(height);
	}

	// Mouse IO
	virtual bool mouse_down(int button, int modifier) override {
		ImGui_ImplGlfwGL3_MouseButtonPressedCallback(button, modifier);
		return ImGui::GetIO().WantCaptureMouse;
	}

	virtual bool mouse_up(int button, int modifier) override {
		return ImGui::GetIO().WantCaptureMouse;
	}

	virtual bool mouse_move(int mouse_x, int mouse_y) override {
		return ImGui::GetIO().WantCaptureMouse;
	}

	virtual bool mouse_scroll(float delta_y) override {
		ImGui_ImplGlfwGL3_ScrollCallback(delta_y);
		return ImGui::GetIO().WantCaptureMouse;
	}

	// Keyboard IO
	virtual bool key_pressed(unsigned int key, int modifiers) override {
		ImGui_ImplGlfwGL3_CharCallback(key);
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	virtual bool key_down(int key, int modifiers) override {
		ImGui_ImplGlfwGL3_KeyCallback(key, GLFW_PRESS, modifiers);
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	virtual bool key_up(int key, int modifiers) override {
		ImGui_ImplGlfwGL3_KeyCallback(key, GLFW_RELEASE, modifiers);
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	// Draw menu
	virtual void draw_menu() {
		float min_width = 200.f;

		// Text labels
		draw_labels_menu();

		// Viewer settings
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(min_width, -1.0f), ImVec2(min_width, -1.0f));
		bool _viewer_menu_visible = true;
		ImGui::Begin(
			"Viewer", &_viewer_menu_visible,
			ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoSavedSettings
		);
		draw_viewer_menu();
		ImGui::End();
	}

	void draw_viewer_menu() {
		// Workspace
		if (ImGui::CollapsingHeader("Workspace", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Columns(2, nullptr, false);
			if (ImGui::Button("Load##Workspace", ImVec2(-1, 0))) {
				viewer->load_scene();
			}
			ImGui::NextColumn();
			if (ImGui::Button("Save##Workspace", ImVec2(-1, 0))) {
				viewer->save_scene();
			}
			ImGui::Columns(1);
		}

		// IO
		if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Columns(2, nullptr, false);
			if (ImGui::Button("Load##Mesh", ImVec2(-1, 0))) {
				viewer->open_dialog_load_mesh();
			}
			ImGui::NextColumn();
			if (ImGui::Button("Save##Mesh", ImVec2(-1, 0))) {
				viewer->open_dialog_save_mesh();
			}
			ImGui::Columns(1);
		}

		// Viewing options
		if (ImGui::CollapsingHeader("Viewing Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Center object", ImVec2(-1, 0))) {
				viewer->core.align_camera_center(viewer->data.V, viewer->data.F);
			}
			if (ImGui::Button("Snap canonical view", ImVec2(-1, 0))) {
				viewer->snap_to_canonical_quaternion();
			}
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);
			ImGui::DragFloat("Zoom", &(viewer->core.camera_zoom), 0.05f, 0.1f, 20.0f);
						ImGui::Checkbox("Orthographic view", &(viewer->core.orthographic));
			ImGui::PopItemWidth();
			static int rotation_type = 2;
			static Eigen::Quaternionf trackball_angle = Eigen::Quaternionf::Identity();
			static bool orthographic = true;
			ImGui::PushItemWidth(100);
			if (ImGui::Combo("Camera Type", &rotation_type, "Trackball\0Two Axis\0002D Mode\0\0")) {
				using RT = igl::viewer::ViewerCore::RotationType;
				auto new_type = static_cast<RT>(rotation_type);
				if (new_type != viewer->core.rotation_type) {
					if (new_type == RT::ROTATION_TYPE_NO_ROTATION) {
						trackball_angle = viewer->core.trackball_angle;
						orthographic = viewer->core.orthographic;
						viewer->core.trackball_angle = Eigen::Quaternionf::Identity();
						viewer->core.orthographic = true;
					} else if (viewer->core.rotation_type == RT::ROTATION_TYPE_NO_ROTATION) {
						viewer->core.trackball_angle = trackball_angle;
						viewer->core.orthographic = orthographic;
					}
					viewer->core.set_rotation_type(new_type);
				}
			}
			ImGui::PopItemWidth();
		}

		// Draw options
		if (ImGui::CollapsingHeader("Draw Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Checkbox("Face-based", &(viewer->data.face_based))) {
				viewer->data.set_face_based(viewer->data.face_based);
			}
			ImGui::Checkbox("Show texture", &(viewer->core.show_texture));
			if (ImGui::Checkbox("Invert normals", &(viewer->core.invert_normals))) {
				viewer->data.dirty |= igl::viewer::ViewerData::DIRTY_NORMAL;
			}
			ImGui::Checkbox("Show overlay", &(viewer->core.show_overlay));
			ImGui::Checkbox("Show overlay depth", &(viewer->core.show_overlay_depth));
			ImGui::ColorEdit4("Background", viewer->core.background_color.data(),
				ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::ColorEdit4("Line color", viewer->core.line_color.data(),
				ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);
			ImGui::DragFloat("Shininess", &(viewer->core.shininess), 0.05f, 0.0f, 100.0f);
			ImGui::PopItemWidth();
		}

		// Overlays
		if (ImGui::CollapsingHeader("Overlays", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Wireframe", &(viewer->core.show_lines));
			ImGui::Checkbox("Fill", &(viewer->core.show_faces));
			ImGui::Checkbox("Show vertex labels", &(viewer->core.show_vertid));
			ImGui::Checkbox("Show faces labels", &(viewer->core.show_faceid));
		}
	}

	void draw_labels_menu() {
		// Calculate pixel ratio for hi-dpi devices
		{
			Eigen::Vector2i mFBSize;
			Eigen::Vector2i mSize;
			GLFWwindow* mGLFWWindow = glfwGetCurrentContext();
			glfwGetFramebufferSize(mGLFWWindow, &mFBSize[0], &mFBSize[1]);
			glfwGetWindowSize(mGLFWWindow, &mSize[0], &mSize[1]);
			glViewport(0,0, mFBSize[0], mFBSize[1]);
			glClear(GL_STENCIL_BUFFER_BIT);
			m_PixelRatio = (float) mFBSize[0] / (float) mSize[0];
		}

		// Text labels
		ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiSetCond_Always);
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiSetCond_Always);
		bool visible = true;
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
		ImGui::Begin("ViewerLabels", &visible,
			ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoInputs);
		draw_labels();
		ImGui::End();
		ImGui::PopStyleColor();
	}

	void draw_labels() {
		if (viewer->core.show_vertid) {
			for (int i = 0; i < viewer->data.V.rows(); ++i) {
				draw_text(viewer->data.V.row(i), viewer->data.V_normals.row(i), std::to_string(i));
			}
		}

		if (viewer->core.show_faceid) {
			for (int i = 0; i < viewer->data.F.rows(); ++i) {
				Eigen::RowVector3d p = Eigen::RowVector3d::Zero();
				for (int j = 0; j < viewer->data.F.cols(); ++j) {
					p += viewer->data.V.row(viewer->data.F(i,j));
				}
				p /= (double) viewer->data.F.cols();

				draw_text(p, viewer->data.F_normals.row(i), std::to_string(i));
			}
		}

		if (viewer->data.labels_positions.rows() > 0) {
			for (int i = 0; i < viewer->data.labels_positions.rows(); ++i) {
				draw_text(viewer->data.labels_positions.row(i), Eigen::Vector3d(0.0,0.0,0.0),
					viewer->data.labels_strings[i]);
			}
		}
	}

	void draw_text(Eigen::Vector3d pos, Eigen::Vector3d normal, const std::string &text) {
		Eigen::Matrix4f view_matrix = viewer->core.view*viewer->core.model;
		pos += normal * 0.005f * viewer->core.object_scale;
		Eigen::Vector3f coord = igl::project(Eigen::Vector3f(pos.cast<float>()),
			view_matrix, viewer->core.proj, viewer->core.viewport);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize()*m_PixelRatio,
				ImVec2(coord[0]/m_PixelRatio, (viewer->core.viewport[3] - coord[1])/m_PixelRatio),
				ImGui::GetColorU32(ImVec4(0, 0, 10, 255)),
				&text[0], &text[0] + text.size());

		//ImGui::SetCursorScreenPos(ImVec2(64, 64));
		//ImGui::TextColored(ImVec4(0,0,255,128), "barbar");
	}
};
