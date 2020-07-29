#pragma once
#include <vector>
#include <fstream>
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
	Compiler() {}
	~Compiler() {}

	void loadData(std::string config_path, std::string test_path)
	{
		// Read config ------------

		std::ifstream f_config(config_path);
		std::string line;
		while (std::getline(f_config, line))
		{
			// Remove whitespaces
			size_t i = 0;
			while ((i = line.find(' ', i)) != std::string::npos)
				line.erase(i, 1);

			// Parse first word
			std::string label = line.substr(0, line.find("="));
			// Parse second word
			std::string result = line.substr(line.find("=") + 1, line.length() - line.find("=") - 1);

			// e.g. compilation = simple => label = compilation; result = simple

			if (label == LABEL_COMPILATION)
				m_simple_compilation = result == "simple";
			else
			{
				size_t r = std::stoi(result);

				if (label == LABEL_TIME_EQUALS)
					m_time_equals = r;
				else if (label == LABEL_TIME_ADD)
					m_time_add = r;
				else if (label == LABEL_TIME_MULTIPLY)
					m_time_multiply = r;
				else if (label == LABEL_TIME_POWER)
					m_time_power = r;
				else if (label == LABEL_NUM_PARALLEL)
					m_num_parallel = r;
			}
		}
		f_config.close();

		// ------------ Read config

		// Read test ------------	

		std::ifstream f_test(test_path);
		m_input.clear();
		while (std::getline(f_test, line))
		{
			// Remove whitespaces
			size_t i = 0;
			while ((i = line.find(' ', i)) != std::string::npos)
				line.erase(i, 1);

			m_input.push_back(line);
		}
		f_test.close();

		// ------------ Read test
	}

	void compile();

private:
	bool m_simple_compilation;
	size_t m_time_equals, m_time_add, m_time_multiply, m_time_power, m_num_parallel;

	std::vector<std::string> m_input;
};

const std::string Compiler::LABEL_TIME_EQUALS = "Tw";
const std::string Compiler::LABEL_TIME_ADD = "Ta";
const std::string Compiler::LABEL_TIME_MULTIPLY = "Tm";
const std::string Compiler::LABEL_TIME_POWER = "Te";
const std::string Compiler::LABEL_NUM_PARALLEL = "Nw";
const std::string Compiler::LABEL_COMPILATION = "compilation";