#include "Compiler.h"
#include "Machine.h"
#include <iostream>

int main()
{
	Compiler c;
	c.loadData("config.txt", "test.txt");
	c.compile();
	Machine m(c);
	m.exec("test.imf");
	return 0;
}