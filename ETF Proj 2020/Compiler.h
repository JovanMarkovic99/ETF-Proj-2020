#pragma once
#include <string>

class Compiler
{
private:

	static const std::string LABEL_TIME_EQUALS;
	static const std::string LABEL_TIME_ADD;
	static const std::string LABEL_TIME_MULTIPLY;
	static const std::string LABEL_TIME_POWER;
	static const std::string LABEL_NUM_PARALLEL;
	static const std::string LABEL_COMPILATION;

public:
	Compiler();
	~Compiler();

	void loadData(std::string config_path, std::string test_path);
	void compile();
};

const std::string Compiler::LABEL_TIME_EQUALS = "Tw";
const std::string Compiler::LABEL_TIME_ADD = "Ta";
const std::string Compiler::LABEL_TIME_MULTIPLY = "Tm";
const std::string Compiler::LABEL_TIME_POWER = "Te";
const std::string Compiler::LABEL_NUM_PARALLEL = "Nw";
const std::string Compiler::LABEL_COMPILATION = "compilation";