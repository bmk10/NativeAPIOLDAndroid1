//
// pch.h
// Header for standard system include files.
//
// Used by the build system to generate the precompiled header. Note that no
// pch.cpp is needed and the pch.h is automatically included in all cpp files
// that are part of the project
//

#include <jni.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

//#include <GLES/gl.h>
#include <GLES2/gl2.h>



#include <android/sensor.h>

//new net

//#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <unistd.h>
#include <netinet/in.h> // sockaddr_in
#include  <arpa/inet.h> // inet_addr
//
#include <android/multinetwork.h> // pretty useless at the low api level.
#include <net/ethernet.h>

//new net

#include <android/log.h>
#include "android_native_app_glue.h"
