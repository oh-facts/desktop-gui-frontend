/* date = July 18th 2024 10:07 am */

#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#define R_DEBUG 1

enum R_OPENGL_INST_BUFFER
{
	R_OPENGL_INST_BUFFER_UI,
	R_OPENGL_INST_BUFFER_COUNT,
};

enum R_OPENGL_SHADER_PROG
{
	R_OPENGL_SHADER_PROG_UI,
	R_OPENGL_SHADER_PROG_COUNT,
};

struct R_Opengl_state
{
	GLuint shader_prog[R_OPENGL_SHADER_PROG_COUNT];
	GLuint inst_buffer[R_OPENGL_INST_BUFFER_COUNT];
};

global R_Opengl_state r_opengl_state;

internal void APIENTRY glDebugOutput(GLenum source, 
																		 GLenum type, 
																		 unsigned int id, 
																		 GLenum severity, 
																		 GLsizei length, 
																		 const char *message, 
																		 const void *userParam);


internal void check_compile_errors(GLuint shader, const char *type);
internal void check_link_errors(GLuint shader, const char *type);
internal GLuint r_opengl_make_shader_program(char *vertexShaderSource, char *fragmentShaderSource);
internal GLuint r_opengl_make_buffer(size_t size);

internal void r_opengl_init();
internal R_Handle r_opengl_alloc_texture(void *data, i32 w, i32 h, i32 n, R_Texture_params *p);
internal void r_opengl_submit(R_Pass_list *list);

global u32 sprite_draw_indices[] = {
  0,1,3,
  1,2,3
};

global char* r_vs_ui_src =
R"(
#version 450 core

struct Vertex 
{
	vec2 pos;
	vec2 uv;
};

struct TextObject
{
	vec2 pos;
	vec2 scale;
vec4 color;
uvec2 sprite_id;
uvec2 padd;
};

layout (std430, binding = 0) buffer ssbo {
	 mat4 proj;
TextObject objects[];
};

out vec4 col;
out vec2 tex;
flat out uvec2 texId;

void main()
{
	
	TextObject obj = objects[gl_InstanceID];

	Vertex vertices[] = {
		{{ obj.pos.x + obj.scale.x,  obj.pos.y}, {1,0}},
		{{ obj.pos.x + obj.scale.x,  obj.pos.y + obj.scale.y}, {1,1}},
		{{ obj.pos.x, obj.pos.y + obj.scale.y}, {0,1}},
		{{ obj.pos.x,  obj.pos.y}, {0,0}},
};
	Vertex vertex = vertices[gl_VertexID];
	
texId = obj.sprite_id;
col = obj.color;
tex = vertex.uv;
	gl_Position =  vec4(vertex.pos, 0.5, 1.0) * proj;// * obj.model;
}


)"
;

global char* r_fs_ui_src = 
R"(
	#version 450 core
	#extension GL_ARB_bindless_texture: require
	
	in vec4 col;
	in vec2 tex;
	flat in uvec2 texId;
 
	out vec4 FragColor;
	void main()
	{
		vec4 tex_col = texture(sampler2D(texId), tex);
#if 1
		if(tex_col.a < 0.01f)
		{
			discard;
		}
#endif

		FragColor =  tex_col * col;
	}
)"
;

#endif //RENDER_OPENGL_H