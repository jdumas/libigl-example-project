// make sure the modern opengl headers are included before any others
#include <igl/frustum.h>
#include <igl/read_triangle_mesh.h>
#include <igl/opengl/create_shader_program.h>
#include <igl/opengl/destroy_shader_program.h>
#include <igl/opengl/bind_vertex_attrib_array.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/material_colors.h>
#include <imgui/imgui.h>
#include <Eigen/Core>
#include <string>

// Number of passes
#define NUM_PASSES 6
GLuint tex_id[NUM_PASSES], dtex_id[NUM_PASSES], fbo_id[NUM_PASSES];

void _check_gl_error(const char *file, int line) {
  GLenum err = glGetError();

  while (err!=GL_NO_ERROR) {
    std::string error;

    switch (err) {
      case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
      case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
      case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
      case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
    }

    std::cerr << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
    err = glGetError();
  }
}

// From: https://blog.nobel-joergensen.com/2013/01/29/debugging-opengl-using-glgeterror/
void _check_gl_error(const char *file, int line);

///
/// Usage
/// [... some opengl calls]
/// glCheckError();
///
#define check_gl_error() _check_gl_error(__FILE__,__LINE__)

void init_render_to_texture(
  const size_t w, const size_t h, GLuint & tex, GLuint & dtex, GLuint & fbo)
{
  const auto & gen_tex = [](GLuint & tex) {
    // http://www.opengl.org/wiki/Framebuffer_Object_Examples#Quick_example.2C_render_to_texture_.282D.29
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  };

  // Generate texture for colors and attached to color component of framebuffer
  gen_tex(tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_BGRA, GL_FLOAT, NULL);
  glBindTexture(GL_TEXTURE_2D, 0);
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // Generate texture for depth and attached to depth component of framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  gen_tex(dtex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex, 0);

  // Clean up
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D,0);
}

// For rendering a full-viewport quad, set tex-coord from position
std::string tex_v_shader = R"(
  #version 150 core
  in vec3 position;
  out vec2 tex_coord;
  void main() {
    gl_Position = vec4(position, 1.0);
    tex_coord = vec2(0.5*(position.x+1), 0.5*(position.y+1));
  }
)";

// Render directly from color or depth texture
std::string tex_f_shader = R"(
  #version 150 core
  in vec2 tex_coord;
  out vec4 color;
  uniform sampler2D color_texture;
  uniform sampler2D depth_texture;
  uniform bool show_depth;

  void main() {
    vec4 depth = texture(depth_texture,tex_coord);
    // Mask out background which is set to 1
    if(depth.r<1) {
      color = texture(color_texture, tex_coord);
      gl_FragDepth = depth.r;
      if (show_depth) {
        // Depth of background seems to be set to exactly 1.
        color.rgb = vec3(1,1,1)*(1.-depth.r)/0.006125;
      }
    } else {
      discard;
    }
  }
)";

// Pass-through vertex shader with projection and view matrices
std::string mesh_vertex_shader_string = R"(
  #version 150
  uniform mat4 view;
  uniform mat4 proj;
  uniform mat4 normal_matrix;
  in vec3 position;
  in vec3 normal;
  out vec3 position_eye;
  out vec3 normal_eye;
  in vec4 Ka;
  in vec4 Kd;
  in vec4 Ks;
  in vec2 texcoord;
  out vec2 texcoordi;
  out vec4 Kai;
  out vec4 Kdi;
  out vec4 Ksi;

  void main()
  {
    position_eye = vec3 (view * vec4 (position, 1.0));
    normal_eye = vec3 (normal_matrix * vec4 (normal, 0.0));
    normal_eye = normalize(normal_eye);
    gl_Position = proj * vec4 (position_eye, 1.0); //proj * view * vec4(position, 1.0);"
    Kai = Ka;
    Kdi = Kd;
    Ksi = Ks;
    texcoordi = texcoord;
  }
)";

// Render if first pass or farther than closest frag on last pass
std::string mesh_fragment_shader_string = R"(
  #version 150
  uniform mat4 view;
  uniform mat4 proj;
  uniform vec4 fixed_color;
  in vec3 position_eye;
  in vec3 normal_eye;
  uniform vec3 light_position_eye;
  vec3 Ls = vec3 (1, 1, 1);
  vec3 Ld = vec3 (1, 1, 1);
  vec3 La = vec3 (1, 1, 1);
  in vec4 Ksi;
  in vec4 Kdi;
  in vec4 Kai;
  in vec2 texcoordi;

  uniform sampler2D tex;
  uniform float specular_exponent;
  uniform float lighting_factor;
  uniform float texture_factor;

  uniform bool first_pass;
  uniform float width;
  uniform float height;
  uniform sampler2D depth_texture;

  out vec4 outColor;

  void main()
  {
    vec3 Ia = La * vec3(Kai); // ambient intensity

    vec3 normal_direction = normalize(normal_eye);
    if (!gl_FrontFacing) {
      normal_direction = -normal_direction;
    }

    vec3 vector_to_light_eye = light_position_eye - position_eye;
    vec3 direction_to_light_eye = normalize (vector_to_light_eye);
    float dot_prod = dot (direction_to_light_eye, normal_direction);
    float clamped_dot_prod = max (dot_prod, 0.0);
    vec3 Id = Ld * vec3(Kdi) * clamped_dot_prod; // Diffuse intensity

    vec3 surface_to_viewer_eye = normalize (-position_eye);
    vec3 half_direction = normalize(direction_to_light_eye + surface_to_viewer_eye); // Halfway vector
    float dot_prod_specular = dot (normal_direction, half_direction);
    float specular_factor = pow(max(dot_prod_specular, 0.0), 2 * specular_exponent);

    // vec3 reflection_eye = reflect (-direction_to_light_eye, normal_direction);
    // vec3 surface_to_viewer_eye = normalize (-position_eye);
    // float dot_prod_specular = dot (reflection_eye, surface_to_viewer_eye);
    // dot_prod_specular = float(abs(dot_prod)==dot_prod) * max (dot_prod_specular, 0.0);
    // float specular_factor = pow (dot_prod_specular, 16);

    vec3 Is = Ls * vec3(Ksi) * specular_factor;    // specular intensity
    vec4 color = vec4(lighting_factor * (Is + Id) + Ia + (1.0-lighting_factor) * vec3(Kdi), Kdi.a);
    outColor = mix(vec4(1,1,1,1), texture(tex, texcoordi), texture_factor) * color;
    if (fixed_color != vec4(0.0)) outColor = fixed_color;

    // if (!gl_FrontFacing) {
    //  discard;
    // }

    if(!first_pass)
    {
      vec2 tex_coord = vec2(float(gl_FragCoord.x)/width, float(gl_FragCoord.y)/height);
      float max_depth = texture(depth_texture,tex_coord).r;
      if(gl_FragCoord.z <= max_depth)
      {
        discard;
      }
    }
  }
)";

class DepthPeelingPlugin : public igl::opengl::glfw::ViewerPlugin {

public:
  DepthPeelingPlugin() {
    viewer = nullptr;
  }

  virtual void init(igl::opengl::glfw::Viewer *_viewer) override {
    assert(_viewer);
    ViewerPlugin::init(_viewer);

    // Initialize textures
    post_resize(viewer->core.viewport[2], viewer->core.viewport[3]);

    // MeshGL buffers
    if (!data.meshgl.is_initialized) {
      data.meshgl.init();
    }
    // Rendering shader
    auto &id = data.meshgl.shader_mesh;
    if (id) {
      igl::opengl::destroy_shader_program(id);
    }
    igl::opengl::create_shader_program(mesh_vertex_shader_string,
      mesh_fragment_shader_string, {}, id);

    // Compositing shader
    V_vbo = (MatrixV(4,3)<<-1,-1,0,1,-1,0,1,1,0,-1,1,0).finished();
    F_vbo = (MatrixF(2,3)<< 0,1,2, 0,2,3).finished();
    igl::opengl::create_shader_program(tex_v_shader, tex_f_shader, {}, shader_square);
    init_buffers();
    bind_square(true);
  }

  virtual void shutdown() override {
    free_buffers();
  }

  virtual bool pre_draw() override {
    float w = viewer->core.viewport(2);
    float h = viewer->core.viewport(3);
    auto bg = viewer->core.background_color;

    // Clear framebuffer
    glClearColor(bg(0), bg(1), bg(2), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw once to set matrices
    bool old_show_faces = data.show_faces;
    bool old_show_lines = data.show_lines;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    data.show_faces = false;
    data.show_lines = false;
    viewer->core.draw(data);
    data.show_faces = old_show_faces;
    data.show_lines = old_show_lines;

    // Select program and attach uniforms
    glEnable(GL_DEPTH_TEST);
    glUseProgram(data.meshgl.shader_mesh);
    glUniform1f(glGetUniformLocation(data.meshgl.shader_mesh, "width"), w);
    glUniform1f(glGetUniformLocation(data.meshgl.shader_mesh, "height"), h);
    glBindVertexArray(data.meshgl.vao_mesh);
    glDisable(GL_BLEND);
    for (int pass = 0; pass < NUM_PASSES; ++pass) {
      const bool first_pass = (pass == 0);
      glUniform1i(glGetUniformLocation(data.meshgl.shader_mesh,"first_pass"),first_pass);
      if (!first_pass) {
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, dtex_id[pass-1]);
        glUniform1i(glGetUniformLocation(data.meshgl.shader_mesh,"depth_texture"), 1);
      }
      glBindFramebuffer(GL_FRAMEBUFFER, fbo_id[pass]);
      glClearColor(bg(0), bg(1), bg(2), 1.f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      draw_mesh();
      check_gl_error();
    }

    // Clean up and set to render to screen
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Get ready to draw quads
    bind_square(false);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Render final result as composite of all textures
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDepthFunc(GL_ALWAYS);
    glUniform1i(glGetUniformLocation(shader_square, "show_depth"), 0);
    GLint color_tex_loc = glGetUniformLocation(shader_square, "color_texture");
    GLint depth_tex_loc = glGetUniformLocation(shader_square, "depth_texture");
    for(int pass = NUM_PASSES-1; pass >= 0; --pass) {
      glUniform1i(color_tex_loc, 0);
      glActiveTexture(GL_TEXTURE0 + 0);
      glBindTexture(GL_TEXTURE_2D, tex_id[pass]);
      glUniform1i(depth_tex_loc, 1);
      glActiveTexture(GL_TEXTURE0 + 1);
      glBindTexture(GL_TEXTURE_2D, dtex_id[pass]);
      glDrawElements(GL_TRIANGLES, F_vbo.size(), GL_UNSIGNED_INT, 0);
    }
    glDepthFunc(GL_LESS);

    // Draw line mesh
    // old_show_faces = data.show_faces;
    // old_show_lines = data.show_lines;
    // data.show_faces = false;
    // data.show_lines = true;
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glUseProgram(data.meshgl.shader_mesh);
    // glBindVertexArray(data.meshgl.vao_mesh);
    // glUniform1i(glGetUniformLocation(data.meshgl.shader_mesh,"first_pass"),true);
    // draw_mesh();
    // data.show_faces = old_show_faces;
    // data.show_lines = old_show_lines;

    return false;
  }

  virtual void post_resize(int w, int h) override {
    for(size_t i = 0; i < NUM_PASSES; ++i) {
      if (viewer) {
        init_render_to_texture(w, h, tex_id[i], dtex_id[i], fbo_id[i]);
      }
    }
  }

  virtual bool key_pressed(unsigned int unicode_key,int modifiers) override {
    switch(unicode_key) {
      case 'F':
      case 'f':
      {
        data.set_face_based(!data.face_based);
        return true;
      }
      case 'I':
      case 'i':
      {
        data.dirty |= igl::opengl::MeshGL::DIRTY_NORMAL;
        data.invert_normals = !data.invert_normals;
        return true;
      }
      case 'L':
      case 'l':
      {
        data.show_lines = !data.show_lines;
        return true;
      }
      case 'T':
      case 't':
      {
        data.show_faces = !data.show_faces;
        return true;
      }
      return false;
    }
    return false;
  }

  igl::opengl::ViewerData data;

private:
  void init_buffers() {
    glGenVertexArrays(1, &vao_square);
    glBindVertexArray(vao_square);
    glGenBuffers(1, &vbo_V);
    glGenBuffers(1, &vbo_F);
    glBindVertexArray(0);
  }

  void free_buffers() {
    glDeleteVertexArrays(1, &vao_square);
    glDeleteBuffers(1, &vbo_V);
    glDeleteBuffers(1, &vbo_F);
  }

  void bind_square(bool refresh) {
    glBindVertexArray(vao_square);
    glUseProgram(shader_square);
    igl::opengl::bind_vertex_attrib_array(shader_square, "position", vbo_V, V_vbo, refresh);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_F);
    if (refresh) {
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned)*F_vbo.size(), F_vbo.data(), GL_DYNAMIC_DRAW);
    }
  }

  void draw_mesh() {
    GLint fixed_colori    = glGetUniformLocation(data.meshgl.shader_mesh, "fixed_color");
    GLint texture_factori = glGetUniformLocation(data.meshgl.shader_mesh, "texture_factor");

    // Render fill
    if (data.show_faces) {
      // Texture
      glUniform1f(texture_factori, data.show_texture ? 1.0f : 0.0f);
      data.meshgl.draw_mesh(true);
      glUniform1f(texture_factori, 0.0f);
    }

    // Render wireframe
    if (data.show_lines) {
      glLineWidth(data.line_width);
      glUniform4f(fixed_colori,
        data.line_color[0],
        data.line_color[1],
        data.line_color[2],
        data.line_color[3]);
      data.meshgl.draw_mesh(false);
      glUniform4f(fixed_colori, 0.0f, 0.0f, 0.0f, 0.0f);
    }
  }

  typedef Eigen::Matrix< float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixV;
  typedef Eigen::Matrix<GLuint, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixF;

  GLuint vao_square;
  GLuint shader_square;
  GLuint vbo_V;
  GLuint vbo_F;

  MatrixV V_vbo;
  MatrixF F_vbo;
};

int main(int argc, char * argv[]) {
  igl::opengl::glfw::Viewer viewer;
  igl::opengl::glfw::imgui::ImGuiMenu menu;
  DepthPeelingPlugin plugin;
  viewer.plugins.push_back(&plugin);
  viewer.plugins.push_back(&menu);
  viewer.resize(1024, 1024);

  // Read input mesh from file
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  if (argc == 1) {
    // Inline mesh of a cube
    V = (Eigen::MatrixXd(8,3)<<
      0.0,0.0,0.0,
      0.0,0.0,1.0,
      0.0,1.0,0.0,
      0.0,1.0,1.0,
      1.0,0.0,0.0,
      1.0,0.0,1.0,
      1.0,1.0,0.0,
      1.0,1.0,1.0).finished();
    F = (Eigen::MatrixXi(12,3)<<
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
  } else {
    igl::read_triangle_mesh(argv[1], V, F);
  }

  // Set mesh
  Eigen::RowVector4f color(igl::GOLD_DIFFUSE[0], igl::GOLD_DIFFUSE[1], igl::GOLD_DIFFUSE[2], 0.5);
  plugin.data.set_mesh(V, F);
  plugin.data.set_colors(color.cast<double>());
  plugin.data.line_color = Eigen::RowVector4f(0, 0, 0, 0.3);
  plugin.data.show_lines = false;
  plugin.data.line_width = 1;
  viewer.core.background_color << 1, 1, 1, 0;
  viewer.core.align_camera_center(V, F);
  viewer.core.set_rotation_type(igl::opengl::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);

  menu.callback_draw_viewer_menu = [&] () {
    if (ImGui::ColorEdit4("Mesh Color", color.data(),
      ImGuiColorEditFlags_NoInputs
      | ImGuiColorEditFlags_PickerHueWheel
      | ImGuiColorEditFlags_AlphaBar))
    {
      plugin.data.set_colors(color.cast<double>());
    }
  };

  viewer.launch();
}
