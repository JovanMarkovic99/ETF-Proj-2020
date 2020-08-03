#pragma once
#include "Memory.h"
#include "Compiler.h"

class Machine
{
public:
	Machine(const Compiler& c) : m_compiler(c) {}

	void exec(std::string test_path)
	{
		std::vector<std::string> input;
		std::ifstream f_test(test_path);
		std::string line;
		while (std::getline(f_test, line))
			input.push_back(line);
		f_test.close();

		std::vector<std::string> output(input.size());
		// A sorted vector of write times, first = to what time, second = number of writes to that time
		std::vector<std::pair<size_t, size_t>> writes_schedule;
		// A map of when the values are set
		std::unordered_map<std::string, size_t> time_map;
		Memory memory;
		for (size_t i = 0; i < input.size(); ++i)
		{
			output[i].append("[" + std::to_string(i) + "]" + "\t(");

			// = ---------------------------------------------
			// TODO : Refactor
			size_t j = 0;
			if ((j = input[i].find("=")) != std::string::npos)
			{
				std::string to_write = input[i].substr(j + 2, input[i].find(" ", j + 2) - j - 2);
				std::string to_read = input[i].substr(input[i].find(" ", j + 2) + 1, input[i].length() - input[i].find(" ", j + 2) - 1);
				
				double val_to_write;
				size_t minimum_time = 0;

				// Variable/token
				if (isalpha(to_read[0]))
				{
					if (time_map.find(to_write) != time_map.end())
						minimum_time = time_map[to_read];
					val_to_write = memory.get(to_read);
				}
				// Constant
				else
					val_to_write = std::stod(to_read);

				if (writes_schedule.size() == 0)
					writes_schedule.emplace_back(minimum_time + m_compiler.m_time_equals, 1);
				else
				{
					auto i = binarySearch(writes_schedule, minimum_time + m_compiler.m_time_equals);

					while (true)
					{
						// Insert at i (at the start)
						if (minimum_time + m_compiler.m_time_equals < writes_schedule[i].first)
						{
							// Success
							if (minimum_time + 2 * m_compiler.m_time_equals < writes_schedule[i].first || writes_schedule[i].second < m_compiler.m_num_parallel)
							{
								writes_schedule.insert(writes_schedule.begin(), { minimum_time + m_compiler.m_time_equals, writes_schedule[i].second + 1 });
								break;
							}
							// Failure
							minimum_time = writes_schedule[i].first - m_compiler.m_time_equals;
						}

						// Update at i
						if (minimum_time + m_compiler.m_time_equals == writes_schedule[i].first)
						{
							// Forward success
							if (minimum_time + 2 * m_compiler.m_time_equals < writes_schedule[i].first || writes_schedule[i].second < m_compiler.m_num_parallel)
							{
								auto j = i - 1;
								while (j != SIZE_MAX && writes_schedule[j].first > minimum_time && writes_schedule[j].second < m_compiler.m_num_parallel)
									--j;
								// Success
								if (j == SIZE_MAX || writes_schedule[j].first <= minimum_time)
								{
									while (i != j)
										++writes_schedule[i--].second;
									break;
								}
							}
							// Failure
							bool prev = false;
							if (i != 0 && writes_schedule[i - 1].first > minimum_time)
								minimum_time = writes_schedule[i - 1].first, prev = true;
							if (!prev || writes_schedule[i].first < minimum_time)
								minimum_time = writes_schedule[i].first, prev = false;
							if (i + 1 != writes_schedule.size() && writes_schedule[i + 1].first < minimum_time)
								minimum_time = writes_schedule[i + 1].first, prev = false;
							if (prev && i > 0)
								--i;
							else if (!prev && i + 1 < writes_schedule.size())
								++i;
							continue;
						}

						// Insert after i
						if (minimum_time + m_compiler.m_time_equals > writes_schedule[i].first)
						{
							// Forward success
							if (i + 1 == writes_schedule.size() || minimum_time + 2 * m_compiler.m_time_equals < writes_schedule[i + 1].first ||
								writes_schedule[i + 1].second < m_compiler.m_num_parallel)
							{
								auto j = i;
								while (j != SIZE_MAX && writes_schedule[j].first > minimum_time && writes_schedule[j].second < m_compiler.m_num_parallel)
									--j;
								// Success
								if (j == SIZE_MAX || writes_schedule[j].first <= minimum_time)
								{
									writes_schedule.insert(writes_schedule.begin() + i + 1, { minimum_time + m_compiler.m_time_equals, i - j + 1 });
									while (i != j)
										++writes_schedule[i--].second;
									break;
								}
							}
							// Failure
							bool prev = false;
							if (i != 0 && writes_schedule[i - 1].first > minimum_time)
								minimum_time = writes_schedule[i - 1].first, prev = true;
							if (!prev || writes_schedule[i].first < minimum_time)
								minimum_time = writes_schedule[i].first, prev = false;
							if (i + 1 != writes_schedule.size() && writes_schedule[i + 1].first < minimum_time)
								minimum_time = writes_schedule[i + 1].first, prev = false;
							if (prev && i > 0)
								--i;
							else if (!prev && i + 1 < writes_schedule.size())
								++i;
							continue;
						}
					}
				}

				time_map[to_write] = minimum_time + m_compiler.m_time_equals;
				output[i].append(std::to_string(time_map[to_write] - m_compiler.m_time_equals) + "-" + std::to_string(time_map[to_write]) 
					+ ")ns");
			}

			// --------------------------------------------- =


		}
	}

private:
	static size_t binarySearch(std::vector<std::pair<size_t, size_t>> vec, size_t val)
	{
		size_t low = 0, high = vec.size(), pivot;
		while (pivot = (high + low) / 2, high - low > 1)
			if (vec[pivot].first > val)
				high = pivot;
			else
				low = pivot;
		if (vec[low].first == val)
			return low;
		if (vec[high].first == val)
			return high;
		return low;
	}

	Compiler m_compiler;
};