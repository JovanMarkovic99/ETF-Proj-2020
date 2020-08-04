#pragma once
#include <unordered_map>

class Memory
{
public:
	Memory() {}
	Memory(const Memory&) = default;
	Memory(Memory&&) = default;
	Memory& operator=(const Memory&) = default;
	Memory& operator=(Memory&&) = default;
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

	void dumpMemory(std::string file_path)
	{
		std::ofstream file(file_path);
		for (auto i = m_memory_pool.begin(); i != m_memory_pool.end(); ++i)
			if (i->first[0] != 't' || !(i->first.length() > 1 && isdigit(i->first[1])))
				file << i->first << " = " << i->second << std::endl;
		file.close();
	}

private:
	std::unordered_map<std::string, double> m_memory_pool;
};