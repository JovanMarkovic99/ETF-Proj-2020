#pragma once
// for std::sort
#include <algorithm>
#include "Memory.h"
#include "Compiler.h"

class Machine
{
public:
	Machine(const Compiler* c) : m_compiler(c) {}

	void exec(std::string test_path)
	{
		// Get instruction
		std::vector<std::string> input;
		std::ifstream f_test(test_path);
		std::string line;
		while (std::getline(f_test, line))
			input.push_back(line);
		f_test.close();

		std::vector<std::string> output(input.size());

		// A sorted vector of write times, first = to what time, second = number of writes to that time
		std::vector<std::pair<size_t, size_t>> writes_schedule;

		// A time map of when the values are set
		std::unordered_map<std::string, size_t> time_map;

		Memory memory;
		for (size_t i = 0; i < input.size(); ++i)
		{
			output[i].append("[" + std::to_string(i + 1) + "]" + "\t(");

			// = ---------------------------------------------
			size_t pos = 0;
			if ((pos = input[i].find("=")) != std::string::npos)
			{
				// Variable to write to
				std::string to_write = input[i].substr(pos + 2, input[i].find(" ", pos + 2) - pos - 2);

				// Variable/token/constant to read from
				std::string to_read = input[i].substr(input[i].find(" ", pos + 2) + 1, input[i].length() - input[i].find(" ", pos + 2) - 1);
				
				double val_to_write;
				// Minimum starting time of the operation
				size_t minimum_time = 0;

				if (isalpha(to_read[0]))
				{
					// Variable/token
					minimum_time = time_map[to_read];
					val_to_write = memory.get(to_read);
				}
				else
					// Constant
					val_to_write = std::stod(to_read);

				memory.set(to_write, val_to_write);

				// Find time
				if (writes_schedule.size() == 0)
					writes_schedule.emplace_back(minimum_time + m_compiler->m_time_equals, 1);
				else
				{
					auto iter = binarySearch(writes_schedule, minimum_time + m_compiler->m_time_equals);

					while (true)
					{
						// Insert at i (at the start)
						if (minimum_time + m_compiler->m_time_equals < writes_schedule[iter].first)
						{
							// Success

							// No overlap
							if (minimum_time + 2 * m_compiler->m_time_equals <= writes_schedule[iter].first)
							{
								writes_schedule.insert(writes_schedule.begin(), { minimum_time + m_compiler->m_time_equals, 1 }); 
								break;
							}
							else if (writes_schedule[iter].second < m_compiler->m_num_parallel)
							{
								writes_schedule.insert(writes_schedule.begin(), { minimum_time + m_compiler->m_time_equals, writes_schedule[iter].second + 1 });
								break;
							}

							// Failure
							minimum_time = writes_schedule[iter].first;
						}

						// Update at i
						if (minimum_time + m_compiler->m_time_equals == writes_schedule[iter].first)
						{
							// Forward success
							if (writes_schedule[iter].second < m_compiler->m_num_parallel)
							{
								// Backwards
								auto j = iter - 1;
								while (j != SIZE_MAX && writes_schedule[j].first > minimum_time && writes_schedule[j].second < m_compiler->m_num_parallel)
									--j;

								// Success
								if (j == SIZE_MAX || writes_schedule[j].first <= minimum_time)
								{
									// Update schedule
									while (iter != j)
										++writes_schedule[iter--].second;
									break;
								}
							}

							// Failure
							if (iter != 0 && writes_schedule[iter - 1].first > minimum_time)
								minimum_time = writes_schedule[iter - 1].first;
							else
								minimum_time = writes_schedule[iter].first;
						}

						// Insert after i
						if (minimum_time + m_compiler->m_time_equals > writes_schedule[iter].first)
						{
							// Forward success
							if (iter + 1 == writes_schedule.size() || minimum_time + 2 * m_compiler->m_time_equals < writes_schedule[iter + 1].first ||
								writes_schedule[iter + 1].second < m_compiler->m_num_parallel)
							{
								// Backwards
								auto j = iter;
								while (j != SIZE_MAX && writes_schedule[j].first > minimum_time && writes_schedule[j].second < m_compiler->m_num_parallel)
									--j;

								// Success
								if (j == SIZE_MAX || writes_schedule[j].first <= minimum_time)
								{
									// Calculate number of writes to that time
									auto j2 = 1;
									if (iter + 1 != writes_schedule.size() && minimum_time + 2 * m_compiler->m_time_equals < writes_schedule[iter + 1].first)
										j2 += writes_schedule[iter + 1].second;

									// Update schedule
									writes_schedule.insert(writes_schedule.begin() + iter + 1, { minimum_time + m_compiler->m_time_equals, j2 });
									while (iter != j)
										++writes_schedule[iter--].second;
									break;
								}
							}

							// Failure
							minimum_time = writes_schedule[iter].first;
							if (iter + 1 != writes_schedule.size())
								++iter;
						}
					}
				}

				time_map[to_write] = minimum_time + m_compiler->m_time_equals;
				output[i].append(std::to_string(time_map[to_write] - m_compiler->m_time_equals) + "-" + std::to_string(time_map[to_write]) 
					+ ")ns");
			}

			// --------------------------------------------- =

			// tokens ----------------------------------------
			else
			{
				// Parse operation
				pos = input[i].find(' ') + 1;
				char op = input[i][pos];

				// Parse token
				auto pos_2 = input[i].find(' ', pos + 2);
				std::string token = input[i].substr(pos + 2, pos_2 - pos - 2);
				
				// Parse operands
				pos = input[i].find(' ', pos_2 + 1);
				std::string op1 = input[i].substr(pos_2 + 1, pos - pos_2 - 1);
				std::string op2 = input[i].substr(pos + 1, input[i].length() - pos - 1);

				size_t time = 0;

				double val1;
				// Variable/token
				if (isalpha(op1[0]))
				{
					time = time_map[op1];
					val1 = memory.get(op1);
				}
				// Constant
				else
					val1 = std::stod(op1);

				double val2;
				// Variable/token
				if (isalpha(op2[0]))
				{
					if (time < time_map[op2])
						time = time_map[op2];
					val2 = memory.get(op2);
				}
				// Constant
				else
					val2 = std::stod(op2);

				Operation* oper = util::getOperation(op);
				memory.set(token, oper->evaluate(val1, val2));
				delete oper;

				time_map[token] = time + findDelay(op);

				output[i].append(std::to_string(time) + "-" + std::to_string(time + findDelay(op))
					+ ")ns");

				// ---------------------------------------- tokens
			}
		}

		// Sort the output
		auto func = [](const std::string& a, const std::string& b)->bool 
		{ 
			double a1 = std::stod(a.substr(a.find('(') + 1, a.find('-') - a.find('(') - 1));
			double a2 = std::stod(a.substr(a.find('-') + 1, a.find(')') - a.find('-') - 1));
			double b1 = std::stod(b.substr(b.find('(') + 1, b.find('-') - b.find('(') - 1));
			double b2 = std::stod(b.substr(b.find('-') + 1, b.find(')') - b.find('-') - 1));
			return (a1 == b1) ? a2 < b2 : a1 < b1;
		};
		sort(output.begin(), output.end(), func);

		// Write output to .log
		std::ofstream output_file(test_path.substr(0, test_path.find(".")) + ".log");
		for (size_t i = 0; i < output.size(); ++i)
			output_file << output[i] << std::endl;
		output_file.close();

		// Write memory to .mem
		memory.dumpMemory(test_path.substr(0, test_path.find(".")) + ".mem");
	}

private:

	static size_t binarySearch(std::vector<std::pair<size_t, size_t>> vec, size_t val)
	{
		size_t low = 0, high = vec.size() - 1, pivot;
		while (pivot = (high + low) / 2, high - low > 1)
			if (vec[pivot].first > val)
				high = pivot;
			else
				low = pivot;
		if (vec[low].first >= val)
			return low;
		if (vec[high].first <= val)
			return high;
		return low;
	}

	size_t findDelay(char op) const
	{
		switch (op)
		{
		case '+':
			return m_compiler->m_time_add;
		case '*':
			return m_compiler->m_time_multiply;
		case '^':
			return m_compiler->m_time_power;
		}
		return 0;
	}

	const Compiler* m_compiler;
};