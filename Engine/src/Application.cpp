#include "Application.h"

#include <iostream>

Application::Application()
{

}

Application::~Application()
{

}

void Application::Run()
{
	std::cout << "This call is from the DLL!";
}