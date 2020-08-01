#pragma once

namespace util
{

	class Operation
	{
	public:
		virtual int priority() = 0;
		virtual char label() = 0;
		virtual int evaluate(int, int) = 0;
	};

	template <char Label, int Priority>
	class Op : public Operation
	{
	public:
		int priority() override { return Priority; }
		char label() override { return Label; }
	};

	// Adding operators ------------------

	template <>
	class Op<'+', 2> : public Operation
	{
		int priority() override { return 2; }
		char label() override { return '+'; }
		int evaluate(int a, int b) override { return a + b; }
	};

	template <>
	class Op<'*', 3> : public Operation
	{
		int priority() override { return 3; }
		char label() override { return '*'; }
		int evaluate(int a, int b) override { return a * b; }
	};

	template <>
	class Op<'^', 4> : public Operation
	{
		int priority() override { return 4; }
		char label() override { return '^'; }
		int evaluate(int a, int b) override { return pow(a, b); }
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

	// ------------------ Adding operators

	class NodeType
	{
	private:
		// Disable constructor unless used within the class
		NodeType() : m_value(nullptr), m_left(nullptr), m_right(nullptr) {}

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