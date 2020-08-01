#pragma once
#include <vector>
#include <stack>
#include <fstream>
#include <string>
#include <tuple>
#include "utility.h"

using namespace util;

class Compiler
{
public:
	Compiler() {}
	~Compiler() {}

	void loadData(std::string config_path, std::string test_path)
	{
		m_test_path = test_path;

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
		auto output = inputToPostfix();
		createSyntaxTrees(output);
		createIMFFile();
		deleteSyntaxTrees();
	}

private:
	bool m_simple_compilation;
	size_t m_time_equals, m_time_add, m_time_multiply, m_time_power, m_num_parallel;

	std::vector<std::string> m_input;
	std::vector<NodeType*> m_syntax_trees;

	std::string m_test_path;

	static const std::string LABEL_TIME_EQUALS;
	static const std::string LABEL_TIME_ADD;
	static const std::string LABEL_TIME_MULTIPLY;
	static const std::string LABEL_TIME_POWER;
	static const std::string LABEL_NUM_PARALLEL;
	static const std::string LABEL_COMPILATION;


	std::vector<std::string> inputToPostfix() const
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

		return output;
	}

	void createSyntaxTrees(const std::vector<std::string>& postfix_expressions)
	{
		m_syntax_trees.reserve(postfix_expressions.size());
		for (size_t i = 0; i < postfix_expressions.size(); ++i)
		{

			std::stack<NodeType*> stack;

			for (size_t j = 0; j < postfix_expressions[i].length(); ++j)
				if (!isOperation(postfix_expressions[i][j]))
				{
					std::string value;
					value.push_back(postfix_expressions[i][j++]);
					while (postfix_expressions[i][j] != ' ')
						value.push_back(postfix_expressions[i][j++]);
					stack.push(NodeType::newNode(value));
				}
				else
				{
					NodeType* t = NodeType::newNode(postfix_expressions[i][j]);
					t->m_right = stack.top();
					stack.pop();
					t->m_left = stack.top();
					stack.pop();
					stack.push(t);
				}

			m_syntax_trees.push_back(stack.top());
		}
	}

	void createIMFFile() const
	{
		size_t line_num = 0;
		size_t token_num = 1;
		std::ofstream imf_file(m_test_path.substr(0, m_test_path.find(".")) + ".imf");
		for (size_t i = 0; i < m_syntax_trees.size(); ++i)
		{
			if (m_syntax_trees[i]->m_type != NodeType::Type::OPERATION)
			{
				imf_file << '[' << line_num << "] = " << getVariableOnLine(i) << " "
					<< *reinterpret_cast<std::string*>(m_syntax_trees[i]->m_value) << std::endl;
				++line_num;
			}
			else
			{
				std::stack<std::string> imf_stack;
				// Pair of token numbers and their appropriate nodes
				std::stack<std::pair<size_t, NodeType*>>  node_stack;
				imf_stack.push(" = " + getVariableOnLine(i) + " t" + std::to_string(token_num));
				node_stack.emplace(token_num++, m_syntax_trees[i]);

				while (!node_stack.empty())
				{
					size_t token;
					NodeType* node;
					std::tie(token, node) = node_stack.top();
					node_stack.pop();
					// str = " * tn " || " + tn " || " ^ tn "
					std::string str = " " + std::string(1, reinterpret_cast<Operation*>(node->m_value)->label()) + " t" + std::to_string(token) + " ";

					// Check left
					if (node->m_left->m_type != NodeType::Type::OPERATION)
						str.append(*reinterpret_cast<std::string*>(node->m_left->m_value) + " ");
					else
					{
						str.append("t" + std::to_string(token_num) + " ");
						node_stack.emplace(token_num++ , node->m_left);
					}

					// Check right
					if (node->m_right->m_type != NodeType::Type::OPERATION)
						str.append(*reinterpret_cast<std::string*>(node->m_right->m_value));
					else
					{
						str.append("t" + std::to_string(token_num));
						node_stack.emplace(token_num++, node->m_right);
					}

					imf_stack.push(str);
				}

				// Empty stack
				while (!imf_stack.empty())
				{
					imf_file << '[' << line_num++ << "]" << imf_stack.top() << std::endl;
					imf_stack.pop();
				}
			}
		}
		imf_file.close();
	}

	void deleteSyntaxTrees()
	{
		for (size_t i = 0; i < m_syntax_trees.size(); ++i)
		{
			std::stack<NodeType*> stack;
			stack.push(m_syntax_trees[i]);
			while (!stack.empty())
			{
				auto node = stack.top();
				if (node->m_left != nullptr)
				{
					stack.push(node->m_left);
					stack.push(node->m_right);
				}
				delete node;
			}
		}
	}

	std::string getVariableOnLine(size_t line_num) const { return m_input[line_num].substr(0, m_input[line_num].find("=")); }
};

const std::string Compiler::LABEL_TIME_EQUALS = "Tw";
const std::string Compiler::LABEL_TIME_ADD = "Ta";
const std::string Compiler::LABEL_TIME_MULTIPLY = "Tm";
const std::string Compiler::LABEL_TIME_POWER = "Te";
const std::string Compiler::LABEL_NUM_PARALLEL = "Nw";
const std::string Compiler::LABEL_COMPILATION = "compilation";