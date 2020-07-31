#pragma once
#include <vector>
#include <stack>
#include <fstream>
#include <string>
#include "utility.h"

using namespace util;

class Compiler
{
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

	void compile()
	{
		std::vector<std::string> output(m_input.size());

		// Create postfix expressions ------------

		for (size_t i = 0; i < m_input.size(); ++i)
		{
			std::string expression = m_input[i].substr(m_input[i].find("=") + 1,
				m_input[i].length() - m_input[i].find("=") - 1);

			std::string postfix_expr;
			std::string var_or_num;
			std::stack<Operation*> operation_stack;

			for (size_t j = 0; j < expression.length(); ++j)
				if (!isOperation(expression[j]))
					var_or_num.push_back(expression[j]);
				else
				{
					if (var_or_num.size() > 0)
					{
						var_or_num.push_back(' ');
						postfix_expr.append(var_or_num);
						var_or_num.clear();
					}

					Operation* op = getOperation(expression[j]);
					// (there is a operator at the top of the operator stack)
					// and ((the operator at the top of the operator stack has greater precedence)
					// or (the operator at the top of the operator stack has equal precedence and the token is left associative))
					while (!operation_stack.empty() && (operation_stack.top()->priority() > op->priority() ||
						(operation_stack.top()->priority() == op->priority() && operation_stack.top()->label() != '^')))
					{
						Operation* op2 = operation_stack.top();
						operation_stack.pop();
						postfix_expr.append(1, op2->label());
						delete op2;
					}
					operation_stack.push(op);
				}

			if (var_or_num.size() > 0)
			{
				var_or_num.push_back(' ');
				postfix_expr.append(var_or_num);
			}

			while (!operation_stack.empty())
			{
				Operation* op = operation_stack.top();
				operation_stack.pop();
				postfix_expr.append(1, op->label());
				delete op;
			}

			output[i] = postfix_expr;
		}

		// -------------- Create postfix expressions

		// Create trees-----------------------------

		m_syntax_trees.clear();
		m_syntax_trees.reserve(output.size());
		for (size_t i = 0; i < output.size(); ++i)
		{

			std::stack<NodeType*> stack;

			for (size_t j = 0; j < output[i].length(); ++j)
				if (!isOperation(output[i][j]))
				{
					std::string value;
					value.push_back(output[i][j++]);
					while (output[i][j] != ' ')
						value.push_back(output[i][j++]);
					stack.push(NodeType::newNode(value));
				}
				else
				{
					NodeType* t = NodeType::newNode(output[i][j]);
					t->m_right = stack.top();
					stack.pop();
					t->m_left = stack.top();
					stack.pop();
					stack.push(t);
				}

			m_syntax_trees.push_back(stack.top());
		}

		// ---------------------------- Create trees
	}

private:
	bool m_simple_compilation;
	size_t m_time_equals, m_time_add, m_time_multiply, m_time_power, m_num_parallel;

	std::vector<std::string> m_input;
	std::vector<NodeType*> m_syntax_trees;

	static const std::string LABEL_TIME_EQUALS;
	static const std::string LABEL_TIME_ADD;
	static const std::string LABEL_TIME_MULTIPLY;
	static const std::string LABEL_TIME_POWER;
	static const std::string LABEL_NUM_PARALLEL;
	static const std::string LABEL_COMPILATION;
};

const std::string Compiler::LABEL_TIME_EQUALS = "Tw";
const std::string Compiler::LABEL_TIME_ADD = "Ta";
const std::string Compiler::LABEL_TIME_MULTIPLY = "Tm";
const std::string Compiler::LABEL_TIME_POWER = "Te";
const std::string Compiler::LABEL_NUM_PARALLEL = "Nw";
const std::string Compiler::LABEL_COMPILATION = "compilation";