#pragma once

namespace util
{

	class Operation
	{
	public:
		virtual int priority() = 0;
		virtual char label() = 0;
	};

	template <char Label, int Priority>
	class Op : public Operation
	{
	public:
		int priority() override { return Priority; }
		char label() override { return Label; }
	};

	using Add = Op<'+', 2>;
	using Multiply = Op<'*', 3>;
	using Power = Op<'^', 4>;

	constexpr bool isOperation(char c)
	{
		return c == '*' || c == '^' || c == '+';
	}

	constexpr Operation* getOperation(char c)
	{
		if (!isOperation(c))
			return nullptr;

		switch (c)
		{
		case '*':
			return new Multiply();
			break;
		case '^':
			return new Power();
			break;
		case '+':
			return new Add();
			break;
		default:
			return nullptr;
			break;
		}
	}

	class NodeType
	{
	private:
		// Disable constructor unless used within the class
		NodeType() : m_value(nullptr) {}

	public:
		enum class Type { OPERATION, CONSTANT, VARIABLE };

		// Can either be Operation*, std::string*
		void* m_value;
		Type m_type;

		NodeType* m_left;
		NodeType* m_right;

		~NodeType() { delete m_value; }

		static NodeType* newNode(std::string value)
		{
			NodeType* node = new NodeType();
			node->m_value = new std::string(value);
			if (isalpha(value[0]))
				node->m_type = Type::VARIABLE;
			else
				node->m_type = Type::CONSTANT;
			return node;
		}

		static NodeType* newNode(char value)
		{
			NodeType* node = new NodeType();
			node->m_value = getOperation(value);
			node->m_type = Type::OPERATION;
			return node;
		}
	};

}	// namespace util