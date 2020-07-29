#include "Compiler.h"
#include <iostream>

int main()
{
	Compiler c;
	c.loadData("config.txt", "test.txt");
	c.compile();
	return 0;
}