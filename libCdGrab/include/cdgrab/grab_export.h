#pragma once

#ifdef _USRDLL
	#define GRAB_SHARED
#endif

#if defined(_WIN32) && defined(GRAB_SHARED)
	#if defined BUILD_GRABCD
		#define GRAB_EXPORT __declspec( dllexport )
		#pragma warning(disable: 4251)
	#else
		#define GRAB_EXPORT __declspec( dllimport )
		//#pragma comment(lib, "CdGrab.lib")
		#pragma warning(disable: 4251)
	#endif
#else
	#define GRAB_EXPORT
#endif
