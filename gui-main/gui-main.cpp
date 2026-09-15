
#include "../lib-src/Model/Native.h"

#define UNICODE 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


// implement this
int uimain(int argc, char* argv[]);


int RealMain()
{
	ui::GlobalAppInit gai;
	return uimain(gai.argc, gai.argv);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return RealMain();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	return RealMain();
}

int main(int argc, char* argv[])
{
	return RealMain();
}
