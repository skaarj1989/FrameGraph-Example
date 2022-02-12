#version 460 core

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out vec2 v_TexCoord;

// glDrawArrays(GL_TRIANGLES, 3);
void main() {
  v_TexCoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  gl_Position = vec4(v_TexCoord * 2.0 - 1.0, 0.0, 1.0);
}
