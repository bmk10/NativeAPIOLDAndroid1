/*
* Copyright (C) 2010 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <malloc.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

/**
* Our saved state data.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
	int32_t key;
};

/**
* Shared state for our app.
*/
struct engine {
	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};


// Can be anything if using abstract namespace
#define SOCKET_NAME "serverSocket"
#define BUFFER_SIZE 16

static int data_socket;
static struct sockaddr_un server_addr;

void setupClient(void) {
	char socket_name[108]; // 108 sun_path length max

	data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (data_socket < 0) {
		LOGI("socket: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// NDK needs abstract namespace by leading with '\0'
	// Ya I was like WTF! too... http://www.toptip.ca/2013/01/unix-domain-socket-with-abstract-socket.html?m=1
	// Note you don't need to unlink() the socket then
	memcpy(&socket_name[0], "\0", 1);
	strcpy(&socket_name[1], SOCKET_NAME);

	// clear for safty
	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX; // Unix Domain instead of AF_INET IP domain
	strncpy(server_addr.sun_path, socket_name, sizeof(server_addr.sun_path) - 1); // 108 char max

	// Assuming only one init connection for demo
	int ret = connect(data_socket, (const struct sockaddr*)&server_addr, sizeof(struct sockaddr_un));
	if (ret < 0) {
		LOGI("connect: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	LOGI("Client Setup Complete");
}

void sendColor(uint8_t color) {
	int ret;
	uint8_t buffer[BUFFER_SIZE];

	LOGI("Color: %d", color);
	memcpy(buffer, &color, sizeof(uint8_t));
	LOGI("Buffer: %d", buffer[0]);

	ret = write(data_socket, buffer, BUFFER_SIZE);
	if (ret < 0) {
		LOGI("write: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ret = read(data_socket, buffer, BUFFER_SIZE);
	if (ret < 0) {
		LOGI("read: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	LOGI("Return: %s", (char*)buffer);
}

void getSomeStaticInitStuff()
{
	char szHostname[128];
	strcpy(szHostname, "intmain.xyz");

	//android_getaddrinfofornetwork(szHostname);//nneed api 23


}
/**
* Initialize an EGL context for the current display.
*/
static int engine_init_display(struct engine* engine) {
	// initialize OpenGL ES and EGL

	/*
	* Here specify the attributes of the desired configuration.
	* Below, we select an EGLConfig with at least 8 bits per color
	* component compatible with on-screen windows
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* Here, the application chooses the configuration it desires. In this
	* sample, we have a very simplified selection process, where we pick
	* the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	* guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	* As soon as we picked a EGLConfig, we can safely reconfigure the
	* ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;




	// Initialize GL state.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);

	return 0;
}


/**
* Just the current frame in the display.
*/
void DrawCrap()
{
	GLfloat vertices[] = { -1, -1, 0, // bottom left corner
					  -1,  1, 0, // top left corner
					   1,  1, 0, // top right corner
					   1, -1, 0 }; // bottom right corner

	GLubyte indices[] = { 0,1,2, // first triangle (bottom left - top left - top right)
						 0,2,3 }; // second triangle (bottom left - top right - bottom right)
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	glVertexPointer(3, GL_FLOAT, 0, vertices);

	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
	glDrawElements(GL_TRIANGLES, 4, GL_UNSIGNED_BYTE, indices);
}
void colored_rect(GLfloat left, GLfloat bottom, GLfloat right, GLfloat top, GLfloat R, GLfloat G, GLfloat B, GLfloat AlphaA = 0.9f)
{
	GLfloat rect[] = {
		left, bottom,
		right, bottom,
		right, top,
		left, top
	};
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor4f(R, G, B, AlphaA);
	glVertexPointer(2, GL_FLOAT, 0, rect);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

}
// GLfloat vVertices[] = {
void DrawNumberOne()
{
	GLfloat vColor1[] = { 1.0f,0.0f,0.0f, 0.8f };
	/*
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		vColor1[0],
		vColor1[1], vColor1[2], vColor1[3]);
*/
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.4f, -0.20f, 0.425f, -0.08f,
		1.0f,
		0.0f, 0.0f, 1.0f);
}

void DrawNumberOne(int iFontSize)
{
	GLfloat vColor1[] = { 1.0f,0.0f,0.0f, 0.8f };
	/*
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		vColor1[0],
		vColor1[1], vColor1[2], vColor1[3]);
*/
	float fLocalFontSize = (float)iFontSize;
	//float yInc = 0.06f;
	float yInc = 0.12f;
	float xInc = 0.025f;

	float xStart = 0.4f;
	float yStart = -0.06f;
	float xStartOffSet = xStart + xInc;
	float yStartOffSet = yStart + yInc;

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);

	float xStart2 = xStart;
	float yStart2 = yStart - yInc - (yInc / 6);
	float xStart2OffSet = xStart2 + xInc;
	float yStart2OffSet = yStart2 + yInc;
	colored_rect(xStart2, yStart2, xStart2OffSet, yStart2OffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);
}



void DrawNumberTwo(int iFontSize)
{
	GLfloat vColor1[] = { 1.0f,0.0f,0.0f, 0.8f };
	/*
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		vColor1[0],
		vColor1[1], vColor1[2], vColor1[3]);
*/
	float fLocalFontSize = (float)iFontSize;
	//float yInc = 0.06f;
	float yInc = 0.12f;
	float xInc = 0.025f;

	float xStart = 0.4f;
	float yStart = -0.06f;
	float xStartOffSet = xStart + xInc;
	float yStartOffSet = yStart + yInc;

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);

	float xStart2 = xStart;
	float yStart2 = yStart - yInc - (yInc / 6);
	float xStart2OffSet = xStart2 + xInc;
	float yStart2OffSet = yStart2 + yInc;
	colored_rect(xStart2, yStart2, xStart2OffSet, yStart2OffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);
}

void DrawNumberThree(int iFontSize)
{
	GLfloat vColor1[] = { 1.0f,0.0f,0.0f, 0.8f };
	/*
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		vColor1[0],
		vColor1[1], vColor1[2], vColor1[3]);
*/
	float fLocalFontSize = (float)iFontSize;
	//float yInc = 0.06f;
	float yInc = 0.12f;
	float xInc = 0.025f;

	float xStart = 0.4f;
	float yStart = -0.06f;
	float xStartOffSet = xStart + xInc;
	float yStartOffSet = yStart + yInc;

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);

	float xStart2 = xStart;
	float yStart2 = yStart - yInc - (yInc / 6);
	float xStart2OffSet = xStart2 + xInc;
	float yStart2OffSet = yStart2 + yInc;
	colored_rect(xStart2, yStart2, xStart2OffSet, yStart2OffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);
}


void DrawNumberEight(int iFontSize)
{
	GLfloat vColor1[] = { 1.0f,0.0f,0.0f, 0.8f };
	/*
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		vColor1[0],
		vColor1[1], vColor1[2], vColor1[3]);
*/
	float fLocalFontSize = (float)iFontSize;
	//float yInc = 0.06f;
	float yInc = 0.12f;
	float xInc = 0.025f;

	float xStart = 0.4f;
	float yStart = -0.06f;
	float xStartOffSet = xStart + xInc;
	float yStartOffSet = yStart + yInc;

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);

	float xStart2 = xStart;
	float yStart2 = yStart - yInc - (yInc / 6);
	float xStart2OffSet = xStart2 + xInc;
	float yStart2OffSet = yStart2 + yInc;
	colored_rect(xStart2, yStart2, xStart2OffSet, yStart2OffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);

	// 7 segment
	colored_rect(0.58f, -0.06f, 0.61f, 0.06f,  // Left Top
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.58f, -0.20f, 0.61f, -0.08f, // Left Bottom
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.61f, 0.06f, 0.73f, 0.08f, // Top
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.61f, -0.08f, 0.73f, -0.06f, // Mid
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.61f, -0.22f, 0.73f, -0.20f, // Botton
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.73f, -0.06f, 0.76f, 0.06f,  // Right Top
		1.0f,
		0.0f, 0.0f, 1.0f);
}

void Draw7SegmentTopLeft(int iFontSize, float xfStart, float yfStart)
{


	// 7 segment
	colored_rect(0.58f, -0.06f, 0.61f, 0.06f,  // Left Top
		1.0f,
		0.0f, 0.0f, 1.0f);




}
void Draw7SegmentBottomLeft(int iFontSize, float xfStart, float yfStart)
{

	colored_rect(0.58f, -0.20f, 0.61f, -0.08f, // Left Bottom
		1.0f,
		0.0f, 0.0f, 1.0f);
}
void Draw7SegmentTop(int iFontSize, float xfStart, float yfStart)
{

	colored_rect(0.61f, 0.06f, 0.73f, 0.08f, // Top
		1.0f,
		0.0f, 0.0f, 1.0f);
}
void Draw7SegmentMid(int iFontSize, float xfStart, float yfStart)
{

	colored_rect(0.61f, -0.08f, 0.73f, -0.06f, // Mid
		1.0f,
		0.0f, 0.0f, 1.0f);
}
void Draw7SegmentBottom(int iFontSize, float xfStart, float yfStart)
{

	colored_rect(0.61f, -0.22f, 0.73f, -0.20f, // Botton
		1.0f,
		0.0f, 0.0f, 1.0f);
}


void Draw7SegmentTopRight(int iFontSize, float xfStart, float yfStart)
{

	colored_rect(0.73f, -0.06f, 0.76f, 0.06f,  // Right Top
		1.0f,
		0.0f, 0.0f, 1.0f);
}

void Draw7SegmentBottomRight(int iFontSize, float xfStart, float yStart)
{

	colored_rect(0.73f, -0.20f, 0.76f, -0.08f,  // Right Bottom
		1.0f,
		0.0f, 0.0f, 1.0f);

}

//////////////////


void Draw7SegmentTopLeft2(int iFontSize, float xfStart, float yfStart)
{

	// 7 segment, say start is xleft:0.57f ytop:0.09f ybottom:-0.23f xright:0.77f
// math : calculate difference, and font type percent, pixel, and mywiehgt.
// xsize = 0.20f width     // font size 20
// ysize = 0.32f height    // font size 20
// ratio 1.6. on regular letter.
// make ratio variable. for x and y
// 0.01f // goshafont widthratio
// 0.016f // goshafont height ratio.
	float fLocalFontSize = (float)iFontSize;
	float xwidthratio = 0.01f * fLocalFontSize;
	float ywidthratio = 0.016f * fLocalFontSize;



	//float yInc = 0.06f;
	//float yInc = 0.12f;
	//float xInc = 0.025f;
	float xletterratio = (0.0015f * fLocalFontSize);
	float yletterratio = 0.006f * fLocalFontSize;
	float xblankwidth = 0.002f * fLocalFontSize;
	float yblankwidth = 0.002f * fLocalFontSize;

	float yInc = yletterratio;
	float xInc = xletterratio;


	float floatincpixel;

	//float xStart = 0.4f;
	//float yStart = -0.06f;
	float xStart = xfStart + xblankwidth;
	float yStart = yfStart + yblankwidth;
	float xStartOffSet = xStart + xInc + xblankwidth;
	float yStartOffSet = yStart + yInc + yblankwidth;

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		1.0f,
		1.0f, 0.0f, 1.0f);

	// 7 segment old call for left top , top left
	//colored_rect(0.58f, -0.06f, 0.61f, 0.06f,  // Left Top
	//	1.0f,
	//	0.0f, 0.0f, 1.0f);




}
void Draw7SegmentBottomLeft2(int iFontSize, float xfStart, float yfStart)
{

	// 7 segment, say start is xleft:0.57f ytop:0.09f ybottom:-0.23f xright:0.77f
// math : calculate difference, and font type percent, pixel, and mywiehgt.
// xsize = 0.20f width     // font size 20
// ysize = 0.32f height    // font size 20
// ratio 1.6. on regular letter.
// make ratio variable. for x and y
// 0.01f // goshafont widthratio
// 0.016f // goshafont height ratio.
	float fLocalFontSize = (float)iFontSize;
	float xwidthratio = 0.01f * fLocalFontSize;
	float ywidthratio = 0.016f * fLocalFontSize;



	//float yInc = 0.06f;
	//float yInc = 0.12f;
	//float xInc = 0.025f;
	float xletterratio = (0.0015f * fLocalFontSize);
	float yletterratio = 0.006f * fLocalFontSize;
	float xblankwidth = 0.002f * fLocalFontSize;
	float yblankwidth = 0.002f * fLocalFontSize;

	float yInc = yletterratio;
	float xInc = xletterratio;


	float floatincpixel;

	//float xStart = 0.4f;
	//float yStart = -0.06f;
	float xStart = xfStart + xblankwidth;
	float yStart = yfStart + yblankwidth + yletterratio + xblankwidth + xblankwidth;
	float xStartOffSet = xStart + xInc + xblankwidth;
	float yStartOffSet = yStart + yInc + yblankwidth; //  yfStart + yblankwidth + yletterratio + xblankwidth + xblankwidth

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		//colored_rect(0.58f, -0.20f, 0.61f, -0.08f, // Left Bottom

		1.0f,
		0.0f, 0.0f, 1.0f);


}

void Draw7SegmentTop2(int iFontSize, float xfStart, float yfStart)
{
	// 7 segment, say start is xleft:0.57f ytop:0.09f ybottom:-0.23f xright:0.77f
// math : calculate difference, and font type percent, pixel, and mywiehgt.
// xsize = 0.20f width     // font size 20
// ysize = 0.32f height    // font size 20
// ratio 1.6. on regular letter.
// make ratio variable. for x and y
// 0.01f // goshafont widthratio
// 0.016f // goshafont height ratio.
	float fLocalFontSize = (float)iFontSize;
	float xwidthratio = 0.01f * fLocalFontSize;
	float ywidthratio = 0.016f * fLocalFontSize;



	//float yInc = 0.06f;
	//float yInc = 0.12f;
	//float xInc = 0.025f;
	float xletterratio = (0.0015f * fLocalFontSize);
	float yletterratio = 0.006f * fLocalFontSize;
	float xblankwidth = 0.002f * fLocalFontSize;
	float yblankwidth = 0.002f * fLocalFontSize;

	float yInc = xletterratio + (xblankwidth / 8);// +(2 * xblankwidth);
	//float xInc = (yletterratio*2) + xblankwidth;  // ok
	//float xInc = (xletterratio * 2) + xblankwidth; //wierd but compact
	float xInc = (yletterratio)+xblankwidth;

	//float xInc2 = (xletterratio ) + xblankwidth;

	float floatincpixel;

	//float xStart = 0.4f;
	//float yStart = -0.06f;
	float xStart = xfStart + xletterratio + (2 * xblankwidth);
	//float yStart = yfStart + ywidthratio - xletterratio;
	float yStart = yfStart + (yblankwidth * 4) + (yletterratio * 2);
	//yStart = yStart + yInc ;
	float xStartOffSet = xStart + xInc + xblankwidth;
	float yStartOffSet = yStart + yInc;


	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet, // Top
		1.0f,
		0.0f, 1.0f, 1.0f);

}
void Draw7SegmentMid2(int iFontSize, float xfStart, float yfStart)
{

	// 7 segment, say start is xleft:0.57f ytop:0.09f ybottom:-0.23f xright:0.77f
// math : calculate difference, and font type percent, pixel, and mywiehgt.
// xsize = 0.20f width     // font size 20
// ysize = 0.32f height    // font size 20
// ratio 1.6. on regular letter.
// make ratio variable. for x and y
// 0.01f // goshafont widthratio
// 0.016f // goshafont height ratio.
	float fLocalFontSize = (float)iFontSize;
	float xwidthratio = 0.01f * fLocalFontSize;
	float ywidthratio = 0.016f * fLocalFontSize;



	//float yInc = 0.06f;
	//float yInc = 0.12f;
	//float xInc = 0.025f;
	float xletterratio = (0.0015f * fLocalFontSize);
	float yletterratio = 0.006f * fLocalFontSize;
	float xblankwidth = 0.002f * fLocalFontSize;
	float yblankwidth = 0.002f * fLocalFontSize;

	float yInc = xletterratio + (xblankwidth / 8);// +(2 * xblankwidth);
	//float xInc = (yletterratio*2) + xblankwidth;  // ok
	//float xInc = (xletterratio * 2) + xblankwidth; //wierd but compact
	float xInc = (yletterratio)+xblankwidth;

	//float xInc2 = (xletterratio ) + xblankwidth;

	float floatincpixel;

	//float xStart = 0.4f;
	//float yStart = -0.06f;
	float xStart = xfStart + xletterratio + (2 * xblankwidth);
	//float yStart = yfStart + ywidthratio - xletterratio;
	float yStart = yfStart + (yblankwidth * 2) + (yletterratio * 1);
	//yStart = yStart + yInc ;
	float xStartOffSet = xStart + xInc + xblankwidth;
	float yStartOffSet = yStart + yInc;


	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet, // Top
		0.0f,
		1.0f, 1.0f, 1.0f);

	//	colored_rect(0.61f, -0.08f, 0.73f, -0.06f, // Mid
		//	1.0f,
		//	0.0f, 0.0f, 1.0f);
}
void Draw7SegmentBottom2(int iFontSize, float xfStart, float yfStart)
{

	// 7 segment, say start is xleft:0.57f ytop:0.09f ybottom:-0.23f xright:0.77f
// math : calculate difference, and font type percent, pixel, and mywiehgt.
// xsize = 0.20f width     // font size 20
// ysize = 0.32f height    // font size 20
// ratio 1.6. on regular letter.
// make ratio variable. for x and y
// 0.01f // goshafont widthratio
// 0.016f // goshafont height ratio.
	float fLocalFontSize = (float)iFontSize;
	float xwidthratio = 0.01f * fLocalFontSize;
	float ywidthratio = 0.016f * fLocalFontSize;



	//float yInc = 0.06f;
	//float yInc = 0.12f;
	//float xInc = 0.025f;
	float xletterratio = (0.0015f * fLocalFontSize);
	float yletterratio = 0.006f * fLocalFontSize;
	float xblankwidth = 0.002f * fLocalFontSize;
	float yblankwidth = 0.002f * fLocalFontSize;

	float yInc = xletterratio + (xblankwidth / 8);// +(2 * xblankwidth);
	//float xInc = (yletterratio*2) + xblankwidth;  // ok
	//float xInc = (xletterratio * 2) + xblankwidth; //wierd but compact
	float xInc = (yletterratio)+xblankwidth;

	//float xInc2 = (xletterratio ) + xblankwidth;

	float floatincpixel;

	//float xStart = 0.4f;
	//float yStart = -0.06f;
	float xStart = xfStart + xletterratio + (2 * xblankwidth);
	//float yStart = yfStart + ywidthratio - xletterratio;
	float yStart = yfStart; // +(yblankwidth * 2) + (yletterratio * 1);
	//yStart = yStart + yInc ;
	float xStartOffSet = xStart + xInc + xblankwidth;
	float yStartOffSet = yStart + yInc;


	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet, // Bottom
		0.0f,
		1.0f, 0.0f, 1.0f);

	//colored_rect(0.61f, -0.22f, 0.73f, -0.20f, // Botton
	//	1.0f,
	//	0.0f, 0.0f, 1.0f);
}


void Draw7SegmentTopRight2(int iFontSize, float xfStart, float yfStart)
{

	colored_rect(0.73f, -0.06f, 0.76f, 0.06f,  // Right Top
		1.0f,
		0.0f, 0.0f, 1.0f);
}

void Draw7SegmentBottomRight2(int iFontSize, float xfStart, float yStart)
{

	colored_rect(0.73f, -0.20f, 0.76f, -0.08f,  // Right Bottom
		1.0f,
		0.0f, 0.0f, 1.0f);

}

void DrawNumberOnePos(int iFontSize)
{
	GLfloat vColor1[] = { 1.0f,0.0f,0.0f, 0.8f };
	/*
	colored_rect(0.4f, -0.06f, 0.425f, 0.06f,
		vColor1[0],
		vColor1[1], vColor1[2], vColor1[3]);
*/
	float fLocalFontSize = (float)iFontSize;
	//float yInc = 0.06f;
	float yInc = 0.12f;
	float xInc = 0.025f;

	float xStart = 0.4f;
	float yStart = -0.06f;
	float xStartOffSet = xStart + xInc;
	float yStartOffSet = yStart + yInc;

	colored_rect(xStart, yStart, xStartOffSet, yStartOffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);

	float xStart2 = xStart;
	float yStart2 = yStart - yInc - (yInc / 6);
	float xStart2OffSet = xStart2 + xInc;
	float yStart2OffSet = yStart2 + yInc;
	colored_rect(xStart2, yStart2, xStart2OffSet, yStart2OffSet,
		1.0f,
		0.0f, 0.0f, 1.0f);
}


static void Draw(struct engine* esContext)
{
	//	UserData *userData = esContext->userData;
	GLfloat vVertices[] = { 0.0f, 0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f };

	// Set the viewport
	//glViewport(0, 0, esContext->width, esContext->height);

	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
	// Use the program object
	//glUseProgram(userData->programObject);
	// Load the vertex data
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
//	glEnableVertexAttribArray(0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vVertices);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	eglSwapBuffers(esContext->display, esContext->surface);
}

// GEorgiy
//__socketcall int bind(int __fd, const struct sockaddr* __addr, socklen_t __addr_length);
//__socketcall int connect(int __fd, const struct sockaddr* __addr, socklen_t __addr_length);
//__socketcall int getpeername(int __fd, struct sockaddr* __addr, socklen_t* __addr_length);
//__socketcall int getsockname(int __fd, struct sockaddr* __addr, socklen_t* __addr_length);
//__socketcall int getsockopt(int __fd, int __level, int __option, void* __value, socklen_t* __value_length);
//__socketcall int listen(int __fd, int __backlog);

int GetHttpResponseHead(int sock, char* buf, int size)
{
	int i;
	char* code, * status;
	memset(buf, 0, size);
	for (i = 0; i < size - 1; i++) {
		if (recv(sock, buf + i, 1, 0) != 1) {
			perror("recv error");
			return -1;
		}
		if (strstr(buf, "\r\n\r\n"))
			break;
	}
	if (i >= size - 1) {
		puts("Http response head too long.");
		return -2;
	}
	code = strstr(buf, " 200 ");
	status = strstr(buf, "\r\n");
	if (!code || code > status) {
		*status = 0;
		printf("Bad http response:\"%s\"\n", buf);
		return -3;
	}
	return i;
}
static char szMemoryReturn[131072];

int HttpGet(const char* server, const char* url)
{
	//__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "Enter HttpGet function!");
	int sock = socket(PF_INET, SOCK_STREAM, 0);
//	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "%d", sock);
	struct sockaddr_in peerAddr;
	char buf[2048];
	int ret;
	//const char *filename;
	//FILE *fp=NULL;
	peerAddr.sin_family = AF_INET;
	peerAddr.sin_port = htons(80);


	struct hostent* hostInfo = gethostbyname(server);

	if (hostInfo)
	{
		memcpy((char*)(&peerAddr.sin_addr), hostInfo->h_addr,  hostInfo->h_length);
	//	//	bcopy(hostInfo->h_addr, (char*)(&remoteAddress.sin_addr), hostInfo->h_length);

//	//	peerAddr.sin_addr.s_addr = hostInfo->h_addr;
//		//peerAddr.sin_addr.s_addr = *(struct in_addr*)hostInfo->;
	//	peerAddr.sin_addr = *(struct in_addr*)hostInfo->h_addr;
	}
	else
	{
	peerAddr.sin_addr.s_addr = inet_addr(server);
	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "hello, this is my number %i log str1!", peerAddr.sin_addr);
	char str[INET_ADDRSTRLEN];

	// store this IP address in sa:
	inet_pton(AF_INET, "3.22.30.40", &(peerAddr.sin_addr));
	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "hello, this is my number %i log str2!", peerAddr.sin_addr);
	// now get it back and print it
	inet_ntop(AF_INET, &(peerAddr.sin_addr), str, INET_ADDRSTRLEN);
	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "hello, this is my number %s log str3!", str);
	//printf("%s\n", str); // prints "192.0.2.33"

	}

	ret = connect(sock, (struct sockaddr*)&peerAddr, sizeof(peerAddr));
	if (ret != 0) {
		perror("connect failed");
		close(sock);
		return -1;
	}

	//memcpy(buf,)

	sprintf(buf,"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", url, server);
	//	"Accept: */*\r\n"
	//	"User-Agent: haxgosha@intmain.xyz\r\n"
	//	"Host: %s\r\n"
	//	"Connection: Close\r\n\r\n",
	//	url, server);
	//string request = "GET /andriod_cloud_webservice.php?f=JChatSend_Msg&NID=922188940&NT=2&t="+ sendText + " HTTP/1.1\r\nHost: " + server +	"\r\nConnection: Close\r\n\r\n";
	// 2048

	send(sock, buf, strlen(buf), 0);
	if (GetHttpResponseHead(sock, buf, sizeof(buf)) < 1) {
		close(sock);
		return -1;
	}
	//filename=strrchr(url,'/')+1;
	//if(!filename[0])
		//filename="index.html";
	//fp=fopen(filename,"wb");
	//if(fp)
	//{
	int iCounter = 0;
	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "Enter HttpGet function's while!");
	while ((ret = recv(sock, buf, sizeof(buf), 0)) > 0)
	{
		//fwrite(buf,ret,1,fp);
		__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "hello, this is my number %s log message!", buf);
		if(iCounter < 131070)
		{ 
	//	szMemoryReturn[iCounter++] = buf[0];
			for (int i = 0; i < sizeof(buf); i++)
			{
				szMemoryReturn[iCounter+i] = buf[i];

			}

			iCounter = iCounter + sizeof(buf);

		}


	}
	szMemoryReturn[iCounter++] = '\0';
	//}
	//fclose(fp);
	shutdown(sock, SHUT_RDWR);
	close(sock);
	return 0;
}

int mouseArracyRect(float iMouseX, float iMouseY)
{


	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 100; j++)
		{
			float xStartmem = -0.50;
			float yStartmem = -0.50;

			float xStartmemInc = 1.0/100;
			float xStartmemInc2 = 1.0 / 200;

			bool bIsHoverButtonArrayButtonInstance = false;

			
			float StartX = xStartmem + (xStartmemInc * i );
			float StartXEnd = StartX + ( xStartmemInc) - (xStartmemInc / 6);// ((xStartmemInc)*i * 2);
		//	float StartXEnd = StartX + xStartmemInc - ();// ((xStartmemInc)*i * 2);

			float StartY = yStartmem + (xStartmemInc * j);
			float StartYEnd = StartY + ( xStartmemInc) - (xStartmemInc2 / 6);// ((xStartmemInc)*i * 2);
			// 2nd try equation to fix hover
		//	iMouseY = 0.5 - iMouseY ;
		//	iMouseX = 0.5 - iMouseX ;
		//	iMouseY =  iMouseY + 0.5;
		//	iMouseX =  iMouseX - 0.5;

			///////////////////////////////
			//
			//          X,Y
			//          0.4,0.2
			//
			//     -0.5        0.    0.5
			//
			//
			//
			//

	//		float iMouseStartX = StartX + 0.5;
		//	float iMouseStartXEnd = StartXEnd + 0.5;
		//	float iMouseStartY = StartY + 0.5;
		//	float iMouseStartYEnd = StartYEnd + 0.5;
		//	iMouseY =-(iMouseY) ;
		//		iMouseX = iMouseX -0.5;

			if (iMouseX > StartX && iMouseX < StartXEnd && iMouseY > StartY  && iMouseY < StartYEnd)
			{
				bIsHoverButtonArrayButtonInstance = true;

			}
			if(bIsHoverButtonArrayButtonInstance)
			{
			colored_rect(StartX, StartY, StartXEnd, StartYEnd,  // Left Top
				0.0f,
				1.0f, 1.0f, 1.0f);
			}
			else
			{
				colored_rect(StartX, StartY, StartXEnd, StartYEnd,  // Left Top
					1.0f,
					0.0f, 1.0f, 1.0f);

			}

		}
	}
	return 0;



}
int intmain(void)
{
	char szServer[512] = { 0 };
	char* head = NULL;
	char* tail = NULL;

	//strcpy(szServer,"https://github.com/ricardoquesada/android-ndk/blob/master/usr/include/arpa/inet.h")
	// https://github.com/bmk10/piopengl1/blob/main/android-socket-initpage-github

	//need ssl?//strcpy(szServer, "https://github.com/bmk10/piopengl1/blob/main/android-socket-initpage-github");
	// "http://192.168.0.156:57501/c.php"
	strcpy(szServer, "http://intmain.xyz/login.php");



	head = (char*)strstr(szServer, "//");
	if (!head) {
		puts("Bad url format");
		return -1;
	}

	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "head%s!", head);
	head += 2;
	tail = strchr(head, '/');
	if (!tail) {
		__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "head2%s!", head);
		return HttpGet(head, "/");
	}
	else if (tail - head > sizeof(szServer) - 1) {
		puts("Bad url format");
		return -1;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "tail!%s", tail);
		memcpy(szServer, head, tail - head);
		//memcpy(szServer, head, tail - head);
		szServer[tail - head] = '\0';

		return HttpGet(szServer, tail);
	}

	return 0;
}
//"http://192.168.7.222/index.html"
int mmain(void)
{
	__android_log_print(ANDROID_LOG_INFO, "-----from--jni-------", "Enter mmain function!");
	char* head, * tail;
	char server[128] = { 0 };
	/*  if(argc<2){
			printf("Usage:%s url\n",argv[0]);
			return -1;
		}
		if(strlen(argv[1])>1500){
			puts("Url too long.");
			return -1;
		}*/
		//head=strstr("http://192.168.10.101/gi/1.html","//");
	head = (char *)strstr("http://192.168.0.156/", "//");
	if (!head) {
		puts("Bad url format");
		return -1;
	}
	head += 2;
	tail = strchr(head, '/');
	if (!tail) {
		return HttpGet(head, "/");
	}
	else if (tail - head > sizeof(server) - 1) {
		puts("Bad url format");
		return -1;
	}
	else {
		memcpy(server, head, tail - head);
		return HttpGet(server, tail);
	}
}
/**
* Just the current frame in the display.
*/
static void engine_draw_frame_0(struct engine* engine) {
	if (engine->display == NULL) {
		// No display.
		return;
	}

	// Just fill the screen with a color.
	glClearColor(((float)engine->state.x) / engine->width, engine->state.angle,
		((float)engine->state.x) / engine->height, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(engine->display, engine->surface);
}
static void engine_draw_frame(struct engine* engine) {
	if (engine->display == NULL) {
		// No display.
		return;
	}

	// Just fill the screen with a color.
	glClearColor(((float)engine->state.x) / engine->width, engine->state.angle,
		((float)engine->state.y) / engine->height, 1);
	glClear(GL_COLOR_BUFFER_BIT);


	bool bIsHoverButton = false;
	bool bIsHoverButton1 = false;
	bool bIsHoverButton2 = false;
	bool bIsHoverButton3 = false;
	bool bIsHoverButton4 = false;
	bool bIsHoverButton5 = false;
	bool bIsHoverButton6 = false;
	bool bIsHoverButton7 = false;
	bool bIsHoverButton8 = false;
	bool bIsHoverButton9 = false;
	//	bool bIsHoverButton10 = false;
		////[REPLACE]]HOVER_BUTTON_BLOCK////
		// where we start INIT OF STACK CODE VARIABLES
		// 1111111111111111111111111111111
		///////////////////////////////////
	
	//intmain();




	float MouseX = ((float)engine->state.x) / engine->width;
	//MouseX = MouseX + 0.5f; // (engine->width / 2);
	float MouseY = ((float)engine->state.y) / engine->height;

	//if (MouseX > 0.1f &&  MouseX < 0.2f && MouseY > 0.1f && MouseY < 0.2f)
	if (MouseX < 1.5f && MouseX > 0.1f)
	{
		bIsHoverButton = true;
		DrawNumberEight(2);
	}
	else
	{
		bIsHoverButton = false;
		DrawNumberTwo(2);
	}

	if (bIsHoverButton)
	{
		colored_rect(0.1f, 0.1f, 0.2f, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(0.1f, 0.1f, 0.2f, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}

	if (MouseY < 0.2f && MouseY > 0.1f)
	{
		bIsHoverButton1 = true;

	}
	else
	{
		bIsHoverButton1 = false;
	}

	if (MouseY < 0.2f && MouseY > 0.1f)
	{
		bIsHoverButton2 = true;

	}
	else
	{
		bIsHoverButton2 = false;
	}


	if (MouseY < 1.0f && MouseY > 0.5f)
	{
		bIsHoverButton3 = true;

	}
	else
	{
		bIsHoverButton3 = false;
	}


	if (MouseX < 1.0f && MouseX > 0.9f)
	{
		bIsHoverButton4 = true;

	}
	else
	{
		bIsHoverButton4 = false;
	}


	if (MouseY < 0.9f && MouseY > 0.8f)
	{
		bIsHoverButton5 = true;

	}
	else
	{
		bIsHoverButton5 = false;
	}
	if (MouseX < 1.0f && MouseX > 0.5f)
	{
		bIsHoverButton6 = true;

	}
	else
	{
		bIsHoverButton6 = false;
	}

	if (bIsHoverButton)
	{
		colored_rect(0.1f, 0.1f, 0.2f, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(0.1f, 0.1f, 0.2f, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}

	if (bIsHoverButton1)
	{

		colored_rect(0.2f, 0.1f, 0.3f, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(0.2f, 0.1f, 0.3f, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton2)
	{

		colored_rect(0.4f, 0.1f, 0.5f, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(0.4f, 0.1f, 0.5f, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton3)
	{

		colored_rect(0.6, 0.1f, 0.8, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(0.6, 0.1f, 0.8, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton4)
	{

		colored_rect(0.9, 0.1f, 0.95, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(0.9, 0.1f, 0.95, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton5)
	{

		colored_rect(5, 0.1f, 5.1, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(5, 0.1f, 5.1, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton6)
	{

		colored_rect(6, 0.1f, 6.1, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(6, 0.1f, 6.1, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton7)
	{

		colored_rect(7, 0.1f, 7.1, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(7, 0.1f, 7.1, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton8)
	{

		colored_rect(8, 0.1f, 8.1, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(8, 0.1f, 8.1, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	if (bIsHoverButton9)
	{

		colored_rect(9, 0.1f, 9.1, 0.2f, 0.5f, 1.0f, 1.0f, 0.9f);
	}
	else
	{
		colored_rect(9, 0.1f, 9.1, 0.2f, 1.0f, 0.0f, 0.2f, 0.9f);
	}
	/* // 2222
	colored_rect(0.0f, 0.3f, 0.09f, 0.4f,
		1.0f, 0.0f, 0.2f, 0.9f);

	colored_rect(0.1f, 0.3f, 0.19f, 0.4f,
		1.0f, 0.0f, 0.2f, 0.9f);

	colored_rect(0.2f, 0.3f, 0.29f, 0.4f,
		1.0f, 0.0f, 0.2f, 0.9f);

	colored_rect(0.3f, 0.3f, 0.39f, 0.4f,
		1.0f, 0.0f, 0.2f, 0.9f);

	colored_rect(0.4f, 0.3f, 0.49f, 0.4f,
		1.0f, 0.0f, 0.2f, 0.9f);
	*/


	// colored_rect(0.1f, 0.1f, 0.2f, 0.2f, 0.5f, 1.0f, 0.2f, 0.9f);

	// colored_rect(0.3f, 0.1f, 0.4f, 0.2f, 0.5f, 1.0f, 0.2f, 0.9f);

//	 colored_rect(0.2f, 0.1f, 0.3f, 0.12f, 0.5f, 1.0f, 0.2f, 0.9f);

	// colored_rect(-0.2f, -1.0f, -0.1f, -0.9f, 0.0f, 0.0f, 1.0f, 0.9f);
	 /*
	 colored_rect(0.2f, 0.205f, -0.1f, 0.2f,
		 0.0f,
		 1.0f, 0.0f, 0.0f);

	 colored_rect(0.4f, 0.405f, -0.1f, 0.2f, 0.0f,
		 1.0f, 0.0f, 0.0f);

	 colored_rect(0.4f, 0.405f, -0.1f, 0.2f, 0.0f,
		 1.0f, 0.0f, 1.0f);
	 */
	 // DrawNumberOne(1);

	Draw7SegmentTopLeft2(10, 0.5f, 0.1f);
	Draw7SegmentBottomLeft2(10, 0.5f, 0.1f);
	Draw7SegmentTop2(10, 0.5f, 0.1f);
	Draw7SegmentMid2(10, 0.5f, 0.1f);
	Draw7SegmentBottom2(10, 0.5f, 0.1f);


	// 7 segment, say start is xleft:0.57f ytop:0.09f ybottom:-0.23f xright:0.77f
	// math : calculate difference, and font type percent, pixel, and mywiehgt.
	// xsize = 0.20f width     // font size 4
	// ysize = 0.32f height    // font size 4
	// ratio 1.6. on regular letter.
	// make ratio variable. for x and y
	// 0.01f // goshafont widthratio
	// 0.016f // goshafont height ratio.
	float lengthBar = strlen(szMemoryReturn) / 10000.0f;
int iTemp =	mouseArracyRect( MouseX, MouseY);

	colored_rect(0.58f, -0.56f, 0.61f+ lengthBar, 0.66f,  // Left Top
		1.0f,
		0.0f, 1.0f, 1.0f);

	
	for (int i = 0; i < strlen(szMemoryReturn); i++)
	{
		float xStartmem = -0.45;
		float yStartmem = -0.15;
		float xStartmemInc = 0.010;

		float StartX = xStartmem + (xStartmemInc * i *2);
		float StartXEnd = StartX + (2*xStartmemInc) - (xStartmemInc/6);// ((xStartmemInc)*i * 2);
	//	float StartXEnd = StartX + xStartmemInc - ();// ((xStartmemInc)*i * 2);

		colored_rect(StartX, yStartmem, StartXEnd, yStartmem+ xStartmemInc,  // Left Top
			0.0f,
			1.0f, 0.0f, 1.0f);

	}
	colored_rect(0.58f, -0.06f, 0.61f, 0.06f,  // Left Top
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.58f, -0.20f, 0.61f, -0.08f, // Left Bottom
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.61f, 0.06f, 0.73f, 0.08f, // Top
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.61f, -0.08f, 0.73f, -0.06f, // Mid
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.61f, -0.22f, 0.73f, -0.20f, // Botton
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.73f, -0.06f, 0.76f, 0.06f,  // Right Top
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.73f, -0.20f, 0.76f, -0.08f,  // Right Bottom
		1.0f,
		0.0f, 0.0f, 1.0f);

	colored_rect(0.77f, -0.22f, 0.80f, -0.25f,  // Right Bottom Dot
		1.0f,
		0.0f, 0.0f, 1.0f);

	DrawCrap();

	eglSwapBuffers(engine->display, engine->surface);
}
/**
* Tear down the EGL context currently associated with the display.
*/
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

/**
* Process the next input event.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
	{
		engine->state.key = AKeyEvent_getKeyCode(event);
		return 2;
	}
	return 0;
}

/**
* Process the next main command.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// The system has asked us to save our current state.  Do so.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// When our app gains focus, we start monitoring the accelerometer.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_enableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
			// We'd like to get 60 events per second (in microseconds).
			ASensorEventQueue_setEventRate(engine->sensorEventQueue,
				engine->accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// When our app loses focus, we stop monitoring the accelerometer.
		// This is to avoid consuming battery while not being used.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_disableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
		}
		// Also stop animating.
		engine->animating = 0;
		engine_draw_frame(engine);
		break;
	}
}

/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
static int Intmain = 111;

void android_main(struct android_app* state) {
	struct engine engine;

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;
	//old

	//new

	//HttpGet(const char* server, const char* url);
	//int Intmain =intmain();
	szMemoryReturn[0] = '\0';

	Intmain = intmain();

	//new
	
	//old
	// Prepare to monitor accelerometer
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
		ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
		state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// We are starting with a previous saved state; restore from it.
		engine.state = *(struct saved_state*)state->savedState;
	}

	engine.animating = 1;

	// loop waiting for stuff to do.

	while (1) {
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
			(void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL) {
				source->process(state, source);
			}

			// If a sensor has data, process it now.
			if (ident == LOOPER_ID_USER) {
				if (engine.accelerometerSensor != NULL) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
						&event, 1) > 0) {
						LOGI("accelerometer: x=%f y=%f z=%f",
							event.acceleration.x, event.acceleration.y,
							event.acceleration.z);
					}
				}
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// Done with events; draw next animation frame.
			engine.state.angle += .01f;
			if (engine.state.angle > 1) {
				engine.state.angle = 0;
			}

			// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			engine_draw_frame(&engine);
		}
	}
}
