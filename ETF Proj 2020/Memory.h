#pragma once
#include <unordered_map>

class Memory
{
public:
	Memory() {}
	~Memory() {}

	void set(std::string var_name, double val)
	{
		m_memory_pool[var_name] = val;
	}

	double get(std::string var_name) const
	{
		auto iter = m_memory_pool.find(var_name);
		if (iter == m_memory_pool.end())
			throw std::exception("No value in memory");

		return iter->second;
	}

private:
	std::unordered_map<std::string, double> m_memory_pool;
};