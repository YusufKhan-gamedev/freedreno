/*
 * Copyright (c) 2012 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "test-util-3d.h"

#define test_enable_disable(cap, action) do {			\
		/* start with cap disable, to account			\
		 * for differences in which caps are			\
		 * enabled/disabled:							\
		 */												\
		GCHK(glDisable(cap));							\
		GCHK(glFlush());								\
		RD_START("enable", "%s (%s)", #cap, #action);	\
		GCHK(action);									\
		GCHK(glEnable(cap));							\
		GCHK(glFlush());								\
		GCHK(glClear(GL_COLOR_BUFFER_BIT));				\
		GCHK(glFlush());								\
		GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));	\
		GCHK(glFlush());								\
		RD_END();										\
		RD_START("disable", "%s", #cap);				\
		GCHK(glDisable(cap));							\
		GCHK(glFlush());								\
		GCHK(glClear(GL_COLOR_BUFFER_BIT));				\
		GCHK(glFlush());								\
		GCHK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));	\
		GCHK(glFlush());								\
		RD_END();										\
	} while(0)

void _glPolygonOffset(GLfloat factor, GLfloat units)
{
	GLfloat f, u;
	DEBUG_MSG("glPolygonOffset(%f, %f);", factor, units);
	glPolygonOffset(factor, units);
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &f);
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &u);
	DEBUG_MSG("actual glPolygonOffset: factor=%f, units=%f", f, u);
}


int main(int argc, char *argv[])
{
	int i;
	GLfloat quad_color[] = {
			1.0, 0.0, 0.0, 1.0,
	};
	GLfloat vertices[] = {
			-0.45, -0.75, 0.0,
			+0.45, -0.75, 0.0,
			-0.45, +0.75, 0.0,
			+0.45, +0.75, 0.0,
	};

	EGLint const config_attribute_list[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_DEPTH_SIZE, 8,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	EGLint const pbuffer_attribute_list[] = {
		EGL_WIDTH, 64,
		EGL_HEIGHT, 64,
		EGL_LARGEST_PBUFFER, EGL_TRUE,
		EGL_NONE
	};

	const EGLint context_attribute_list[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLDisplay display;
	EGLConfig config;
	EGLint num_config;
	EGLContext context;
	EGLSurface surface;
	GLuint program;
	GLint width, height;
	int uniform_location;

	const char *vertex_shader_source =
		"attribute vec4 aPosition;    \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_Position = aPosition; \n"
		"}                            \n";
	const char *fragment_shader_source =
		"precision highp float;       \n"
		"uniform vec4 uColor;         \n"
		"                             \n"
		"void main()                  \n"
		"{                            \n"
		"    gl_FragColor = uColor;   \n"
		"}                            \n";
	TEST_START();

	display = get_display();

	/* get an appropriate EGL frame buffer configuration */
	ECHK(eglChooseConfig(display, config_attribute_list, &config, 1, &num_config));
	DEBUG_MSG("num_config: %d", num_config);

	/* create an EGL rendering context */
	ECHK(context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute_list));

	ECHK(surface = eglCreatePbufferSurface(display, config, pbuffer_attribute_list));

	ECHK(eglQuerySurface(display, surface, EGL_WIDTH, &width));
	ECHK(eglQuerySurface(display, surface, EGL_HEIGHT, &height));

	DEBUG_MSG("PBuffer: %dx%d", width, height);

	/* connect the context to the surface */
	ECHK(eglMakeCurrent(display, surface, surface, context));
	GCHK(glFlush());

	program = get_program(vertex_shader_source, fragment_shader_source);

	GCHK(glBindAttribLocation(program, 0, "aPosition"));

	link_program(program);

	GCHK(glViewport(0, 0, width, height));
	GCHK(glFlush());

	GCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices));
	GCHK(glFlush());
	GCHK(glEnableVertexAttribArray(0));
	GCHK(glFlush());

	/* now set up our uniform. */
	GCHK(uniform_location = glGetUniformLocation(program, "uColor"));

	GCHK(glUniform4fv(uniform_location, 1, quad_color));

//	TEST(test_enable_disable(GL_TEXTURE_2D));
	TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(-15.0, -30.0)));
	TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(20.0, 320.0)));
	TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(1.0, -512.0)));
	TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(-13.33, -41.25)));
	TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(-1.333333, -41.25333)));
	for (i = -64; i < 64; i++)
		TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(3.14159265359, i * 8.333)));
	for (i = -64; i < 64; i++)
		TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(i * 0.1333, 3.14159265359)));
	TEST(test_enable_disable(GL_POLYGON_OFFSET_FILL, _glPolygonOffset(0.0,0.0)));
	TEST(test_enable_disable(GL_SCISSOR_TEST, glScissor(10, 10, width - 20, height - 20)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_ZERO, GL_ZERO)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_ZERO, GL_ONE)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_ONE, GL_ZERO)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_DST_COLOR, GL_DST_COLOR)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_DST_COLOR)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_CONSTANT_COLOR)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA)));
	TEST(test_enable_disable(GL_BLEND, glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR)));
	TEST(test_enable_disable(GL_SAMPLE_ALPHA_TO_COVERAGE, (void)0));
//	TEST(test_enable_disable(GL_ALPHA_TEST_QCOM, glAlphaFuncQCOM(GL_ALWAYS, 0.5)));
//	TEST(test_enable_disable(GL_ALPHA_TEST_QCOM, glAlphaFuncQCOM(GL_LEQUAL, 0.5)));
	TEST(test_enable_disable(GL_SAMPLE_COVERAGE, (void)0));
	TEST(test_enable_disable(GL_STENCIL_TEST, (void)0));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_NEVER)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_LESS)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_EQUAL)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_LEQUAL)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_GREATER)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_NOTEQUAL)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_GEQUAL)));
	TEST(test_enable_disable(GL_DEPTH_TEST, glDepthFunc(GL_ALWAYS)));
	TEST(test_enable_disable(GL_DITHER, (void)0));
	TEST_END();

	ECHK(eglTerminate(display));

	return 0;
}

