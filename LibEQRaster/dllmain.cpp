// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "EQRasterImpl.h"

#define WIN32_LEAN_AND_MEAN
#define NOGDI

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

namespace eqtm{
	LIBEQRASTER_API IEQRaster* _stdcall CreateRasterObject(){
		IEQRaster* eraster = new EQRaster;
		return eraster;
	}

	LIBEQRASTER_API void _stdcall DestroyRasterObject(IEQRaster* eraster){
		eraster->release();
		delete eraster;
	}
}

#undef NOGDI
#undef WIN32_LEAN_AND_MEAN