#pragma once
#include <vector>
#include <stack>
#include <fstream>
#include <string>
#include <tuple>
#include "utility.h"

using namespace util;

class Machine;

class Compiler
{
public:
	Compiler() {}
	Compiler(const Compiler&) = delete;
	Compiler& operator=(const Compiler&) = delete;
	~Compiler() {}

	void loadData(std::string config_path, std::string test_path)
	{
		m_test_path = test_path;

		// Read config ------------

		std::ifstream f_config(config_path);
		std::string line;
		while (std::getline(f_config, line))
		{
			removeWhitespaces(line);

			// Parse first word
			std::string label = line.substr(0, line.find('='));
			// Parse second word
			std::string result = line.substr(line.find('=') + 1, line.length() - line.find('=') - 1);

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
			removeWhitespaces(line);
			m_input.push_back(line);
		}
		f_test.close();

		// ------------ Read test
	}

	void compile()
	{
		auto output = inputToPostfix();
		createSyntaxTrees(output);
		if (!m_simple_compilation)
		{
			optimizeSequentialOperations();
			optimizeTimeZeroOperations();
		}
		createIMFFile();
		deleteSyntaxTrees();
	}

	friend class Machine;

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
			std::string expression = m_input[i].substr(m_input[i].find('=') + 1,
				m_input[i].length() - m_input[i].find('=') - 1);

			std::string postfix_expr;
			std::string var_or_num;
			std::stack<Operation*> operation_stack;

			for (size_t j = 0; j < expression.length(); ++j)
				if (!isOperation(expression[j]))
					var_or_num.push_back(expression[j]);
				else
				{
					// Push variable or constant with space as the terminating character
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
						// Get the operation from the top of the stack and push it to the postfix expression
						Operation* op2 = operation_stack.top();
						operation_stack.pop();
						postfix_expr.push_back(op2->label());
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
					// Grab variable or constant
					std::string value;
					value.push_back(postfix_expressions[i][j++]);
					while (postfix_expressions[i][j] != ' ')
						value.push_back(postfix_expressions[i][j++]);
					stack.push(NodeType::newNode(value));
				}
				else
				{
					// Grab operation
					NodeType* t = NodeType::newNode(postfix_expressions[i][j]);
					t->m_right = stack.top();
					stack.pop();
					t->m_left = stack.top();
					stack.pop();
					stack.push(t);
				}

			// Push the root of the tree to the vector
			m_syntax_trees.push_back(stack.top());
		}
	}

	void createIMFFile() const
	{
		size_t line_num = 1;
		size_t token_num = 1;
		std::ofstream imf_file(m_test_path.substr(0, m_test_path.find(".")) + ".imf");

		for (size_t i = 0; i < m_syntax_trees.size(); ++i)
		{
			if (m_syntax_trees[i]->m_type != NodeType::Type::OPERATION)
			{
				// Parse pure variable/constant asignment
				imf_file << '[' << line_num << "] = " << getOutputVariable(i) << " "
					<< *reinterpret_cast<std::string*>(m_syntax_trees[i]->m_value) << std::endl;
				++line_num;
			}
			else
			{
				// Parse expression

				// Stack of instruction for the expression
				std::stack<std::string> imf_stack;

				// Pair of token numbers and their appropriate operation nodes
				std::stack<std::pair<size_t, NodeType*>>  node_stack;

				// Push last instruction (token numbers are reversed in each expression)
				imf_stack.push(" = " + getOutputVariable(i) + " t" + std::to_string(token_num));
				node_stack.emplace(token_num++, m_syntax_trees[i]);

				while (!node_stack.empty())
				{
					size_t token_num_current;
					NodeType* node;
					std::tie(token_num_current, node) = node_stack.top();
					node_stack.pop();

					// str = " * tn " || " + tn " || " ^ tn "
					std::string str = " " + std::string(1, reinterpret_cast<Operation*>(node->m_value)->label()) + " t" + std::to_string(token_num_current) + " ";

					// Check left
					if (node->m_left->m_type != NodeType::Type::OPERATION)
						// Variable/const
						str.append(*reinterpret_cast<std::string*>(node->m_left->m_value) + " ");
					else
					{
						// Operation; give the operation a token number and put it on the stack
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
				stack.pop();
				if (node->m_left != nullptr)
				{
					stack.push(node->m_left);
					stack.push(node->m_right);
				}
				delete node;
			}
		}
		m_syntax_trees.clear();
	}

	// Optimize two operations that are done sequentialy two times on variable/constants e.g. + t1 a tn; + t2 t1 b -> + t1 a b; + t2 t1 tn
	void optimizeSequentialOperations()
	{
		for (size_t i = 0; i < m_syntax_trees.size(); ++i)
		{
			std::stack<NodeType*> stack;
			
			if (m_syntax_trees[i]->m_type == NodeType::Type::OPERATION)
				stack.push(m_syntax_trees[i]);

			while (!stack.empty())
			{
				auto node = stack.top();
				stack.pop();

				// Operation at node
				auto op = reinterpret_cast<Operation*>(node->m_value)->label();
				
				// Left check
				if (node->m_left->m_type == NodeType::Type::OPERATION &&
					reinterpret_cast<Operation*>(node->m_left->m_value)->label() == op
					&& node->m_right->m_type != NodeType::Type::OPERATION)
				{
					if (node->m_left->m_right->m_type == NodeType::Type::OPERATION && node->m_left->m_left->m_type != NodeType::Type::OPERATION)
						std::swap(node->m_right, node->m_left->m_right);
					else if (node->m_left->m_left->m_type == NodeType::Type::OPERATION && node->m_left->m_right->m_type != NodeType::Type::OPERATION)
						std::swap(node->m_right, node->m_left->m_left);
				}
				// Right check
				else if (node->m_right->m_type == NodeType::Type::OPERATION &&
					reinterpret_cast<Operation*>(node->m_right->m_value)->label() == op
					&& node->m_left->m_type != NodeType::Type::OPERATION)
				{
					if (node->m_right->m_right->m_type == NodeType::Type::OPERATION && node->m_right->m_left->m_type != NodeType::Type::OPERATION)
						std::swap(node->m_left, node->m_right->m_right);
					else if (node->m_right->m_left->m_type == NodeType::Type::OPERATION && node->m_right->m_right->m_type != NodeType::Type::OPERATION)
						std::swap(node->m_left, node->m_right->m_left);
				}

				if (node->m_left->m_type == NodeType::Type::OPERATION)
					stack.push(node->m_left);
				if (node->m_right->m_type == NodeType::Type::OPERATION)
					stack.push(node->m_right);
			}
		}
	}

	// Swap variable operands with constant operands so that there are as much time zero operations,
	// i.e. operations that have constants as operands (operations that can run from 0ns)
	void optimizeTimeZeroOperations()
	{
		for (size_t i = 0; i < m_syntax_trees.size(); ++i)
		{
			std::stack<NodeType*> discovery_stack;
			if (m_syntax_trees[i]->m_type == NodeType::Type::OPERATION)
				discovery_stack.push(m_syntax_trees[i]);

			while (!discovery_stack.empty())
			{
				// free_constants = constants who's sibling is an operation, i.e. constants that can be freely swapped
				std::stack<std::reference_wrapper<NodeType*>> free_constants;
				
				// two_variable_pair = variable who's sibling is a variable, i.e. both need to be swapped
				// var_const_pair = variable who's sibling is a constant, i.e. one of them needs to be swapped
				std::stack<std::pair<std::reference_wrapper<NodeType*>, std::reference_wrapper<NodeType*>>> two_variable_pair, var_const_pair;

				// Explore tree for the node --------------------------------

				std::stack<NodeType*> stack;
				stack.push(discovery_stack.top());
				discovery_stack.pop();
				while (!stack.empty())
				{
					auto node = stack.top();
					stack.pop();

					auto op = reinterpret_cast<Operation*>(node->m_value)->label();

					// Check left
					if (node->m_left->m_type == NodeType::Type::OPERATION)
					{
						if (reinterpret_cast<Operation*>(node->m_left->m_value)->label() == op)
							stack.push(node->m_left);
						else
							discovery_stack.push(node->m_left);
					}

					else if (node->m_left->m_type == NodeType::Type::CONSTANT)
					{
						// Free constant
						if (node->m_right->m_type == NodeType::Type::OPERATION)
							free_constants.push(node->m_left);
						
						// Variable constant pair
						else if (node->m_right->m_type == NodeType::Type::VARIABLE)
							var_const_pair.push({ node->m_right, node->m_left } );
					}
					
					// Two variable pair
					else if (node->m_left->m_type == NodeType::Type::VARIABLE && node->m_right->m_type == NodeType::Type::VARIABLE)
						two_variable_pair.push({ node->m_left, node->m_right });

					// Check right
					if (node->m_right->m_type == NodeType::Type::OPERATION)
					{
						if (reinterpret_cast<Operation*>(node->m_right->m_value)->label() == op)
							stack.push(node->m_right);
						else
							discovery_stack.push(node->m_right);
					}

					else if (node->m_right->m_type == NodeType::Type::CONSTANT)
					{
						// Free constant
						if (node->m_left->m_type == NodeType::Type::OPERATION)
							free_constants.push(node->m_right);

						// Variable constant pair
						else if (node->m_left->m_type == NodeType::Type::VARIABLE)
							var_const_pair.push({ node->m_left, node->m_right });
					}
				}

				// --------------------------------- Explore tree for the node

				// Swap two free constants with a variable pair
				while (!two_variable_pair.empty() && free_constants.size() > 1)
				{
					auto& c1 = free_constants.top().get();
					free_constants.pop();
					auto& c2 = free_constants.top().get();
					free_constants.pop();
					std::swap(two_variable_pair.top().first.get(), c1);
					std::swap(two_variable_pair.top().second.get(), c2);
					two_variable_pair.pop();
				}

				// Swap variables and free constants
				while (!var_const_pair.empty() && !free_constants.empty())
				{
					std::swap(var_const_pair.top().first.get(), free_constants.top().get());
					var_const_pair.pop();
					free_constants.pop();
				}

				// Swap variables and constants from the pairs
				while (var_const_pair.size() > 1)
				{
					auto& v1 = var_const_pair.top().first.get();
					var_const_pair.pop();
					std::swap(v1, var_const_pair.top().second.get());
					var_const_pair.pop();
				}
				
			}
		}
	}

	std::string getOutputVariable(size_t line_num) const { return m_input[line_num].substr(0, m_input[line_num].find('=')); }

	static void removeWhitespaces(std::string& str)
	{
		size_t i = 0;
		while ((i = str.find(' ', i)) != std::string::npos)
			str.erase(i, 1);
	}
};

const std::string Compiler::LABEL_TIME_EQUALS = "Tw";
const std::string Compiler::LABEL_TIME_ADD = "Ta";
const std::string Compiler::LABEL_TIME_MULTIPLY = "Tm";
const std::string Compiler::LABEL_TIME_POWER = "Te";
const std::string Compiler::LABEL_NUM_PARALLEL = "Nw";
const std::string Compiler::LABEL_COMPILATION = "compilation";