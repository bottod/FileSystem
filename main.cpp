#include "FileSystem.h"

int main(int argc, char* argv[])
{
	system("cls");	
	FileSystem MySystem(argv[1]);
	MySystem.InitSystem();
	return 0;
}