#include "egl.h"

EGLint eglGetError(void)
{
	return IMGeglGetError();
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
	return IMGeglGetDisplay(display_id);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	return IMGeglInitialize(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
	return IMGeglTerminate(dpy);
}

const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
	return IMGeglQueryString(dpy, name);
}

void (* eglGetProcAddress(const char *procname))(void)
{
	return IMGeglGetProcAddress(procname);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	return IMGeglGetConfigs(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
	/* We have two issues to work around here.
	 *
	 * The first is about EGL_RECORDABLE_ANDROID:
	 * When the following conditions are met...
	 * 1. EGL_PBUFFER_BIT is set in EGL_SURFACE_TYPE
	 * 2. EGL_OPENGL_ES2_BIT is set in EGL_RENDERABLE_TYPE
	 * ...EGL_RECORDABLE_ANDROID is forced to EGL_FALSE.
	 * Why this happens is unknown, but the end result is no configs get returned.
	 * Code meeting the above conditions seems to work fine without EGL_RECORDABLE_ANDROID,
	 * so in this shim we'll drop it on their behalf so they actually get an EGLConfig.
	 *
	 * The second is about EGL_FRAMEBUFFER_TARGET_ANDROID:
	 * There is not a single damn user of this anywhere except surfaceflinger.
	 * What's happening is our drivers don't recognize it, and so don't return
	 * a config. End result is it has to retry eglChooseConfig 3 times. It's
	 * not actually a requirement though, the issue is simply the drivers failing
	 * since they don't *recognize* the attribute.
	 * So we will drop this for surfaceflinger to avoid 2 extra calls to eglChooseConfig.
	 * Considering this is only used when surfaceflinger starts up, we'll use the compiler
	 * hints to indicate it's rare, to optimize for basically every other call to us.
	 */
	bool renderabletype_es2 = false, surfacetype_pbuffer = false;
	int attrib_count = 0, recordable_val_pos = -1, fbtarget_pos = -1;

	/* attrib_list is terminated by EGL_NONE key */
	while( attrib_list[attrib_count++] != EGL_NONE )
	{
		if( attrib_list[attrib_count-1] == EGL_RENDERABLE_TYPE )
		{
			/* if EGL_RENDERABLE_TYPE is specified, usually EGL_OPENGL_ES2_BIT is set. */
			if( (attrib_list[attrib_count] & EGL_OPENGL_ES2_BIT) != 0 )
				renderabletype_es2 = true;
		}
		else if( attrib_list[attrib_count-1] == EGL_SURFACE_TYPE )
		{
			/* the pbuffer bit seems to be rarely specified though. */
			if( (attrib_list[attrib_count] & EGL_PBUFFER_BIT) != 0 )
				surfacetype_pbuffer = true;
		}
		else if( attrib_list[attrib_count-1] == EGL_RECORDABLE_ANDROID )
		{
			/* It is generally useless to specify EGL_RECORDABLE_ANDROID
			 * as something other than EGL_TRUE; expect that to be the case */
			if( CC_LIKELY( attrib_list[attrib_count] == EGL_TRUE ) )
				recordable_val_pos = attrib_count;
		}
		else if( CC_UNLIKELY( attrib_list[attrib_count-1] == EGL_FRAMEBUFFER_TARGET_ANDROID ) )
		{
			fbtarget_pos = attrib_count - 1;
		}

		/* the array is k/v pairs; for every key we can skip an iteration */
		++attrib_count;
	}

	if( recordable_val_pos != -1 && (!surfacetype_pbuffer || !renderabletype_es2) )
		recordable_val_pos = -1;

	/* It appears that surfaceflinger is snappier without a GL ES2 config.
	 * For now, lets avoid fixing this for GL ES2. End result is surfaceflinger
	 * will still avoid *one* extra call to eglChooseConfig which is nice. */
#ifndef FB_TARGET_FIX_EGL2_ALSO
	if( CC_UNLIKELY( fbtarget_pos != -1 ) && renderabletype_es2 )
		fbtarget_pos = -1;
#endif

	if( recordable_val_pos != -1 || CC_UNLIKELY( fbtarget_pos != -1 ) )
	{
		EGLint override_attrib_list[attrib_count];
		memcpy( override_attrib_list, attrib_list, attrib_count * sizeof(EGLint) );

		if( CC_LIKELY( recordable_val_pos != -1 ) )
			override_attrib_list[recordable_val_pos] = EGL_DONT_CARE;

		if( CC_UNLIKELY( fbtarget_pos != -1 ) )
		{
			memmove( &override_attrib_list[fbtarget_pos],
			         &override_attrib_list[fbtarget_pos+2],
			         (attrib_count - fbtarget_pos - 2) * sizeof(EGLint) );
		}

		return IMGeglChooseConfig(dpy, override_attrib_list, configs, config_size, num_config);
	}

	return IMGeglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
	return IMGeglGetConfigAttrib(dpy, config, attribute, value);
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
	return IMGeglCreateWindowSurface(dpy, config, win, attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
	return IMGeglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
	return IMGeglCreatePbufferSurface(dpy, config, attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	return IMGeglDestroySurface(dpy, surface);
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
	return IMGeglQuerySurface(dpy, surface, attribute, value);
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
	return IMGeglCreateContext(dpy, config, share_context, attrib_list);
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	return IMGeglDestroyContext(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	return IMGeglMakeCurrent(dpy, draw, read, ctx);
}

EGLContext eglGetCurrentContext(void)
{
	return IMGeglGetCurrentContext();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
	return IMGeglGetCurrentSurface(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void)
{
	return IMGeglGetCurrentDisplay();
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
	return IMGeglQueryContext(dpy, ctx, attribute, value);
}

EGLBoolean eglWaitGL(void)
{
	return IMGeglWaitGL();
}

EGLBoolean eglWaitNative(EGLint engine)
{
	return IMGeglWaitNative(engine);
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface draw)
{
	return IMGeglSwapBuffers(dpy, draw);
}

EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
	return IMGeglCopyBuffers(dpy, surface, target);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	return IMGeglSwapInterval(dpy, interval);
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
	return IMGeglSurfaceAttrib(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return IMGeglBindTexImage(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	return IMGeglReleaseTexImage(dpy, surface, buffer);
}

EGLSurface eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
	return IMGeglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
}

EGLBoolean eglBindAPI(EGLenum api)
{
	return IMGeglBindAPI(api);
}

EGLenum eglQueryAPI(void)
{
	return IMGeglQueryAPI();
}

EGLBoolean eglWaitClient(void)
{
	return IMGeglWaitClient();
}

EGLBoolean eglReleaseThread(void)
{
	return IMGeglReleaseThread();
}
