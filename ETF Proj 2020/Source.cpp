#include "Compiler.h"
#include "Machine.h"

int main()
{
	auto a = 1, b = 2;

	Compiler c;
	c.loadData("config.txt", "test.txt");
	c.compile();
	Machine m(&c);
	m.exec("test.imf");
	return 0;
}