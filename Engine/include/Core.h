#pragma once

#ifdef BUILD_DLL
#define APP_API __declspec(dllexport)
#else
#define APP_API __declspec(dllimport)
#endif