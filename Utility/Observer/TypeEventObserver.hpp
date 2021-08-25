#pragma once
#include "EventObserver.hpp"

template<class Type, class CastType = Type>
class TypeEventObserver final :
	public EventObserver
{
public:

	TypeEventObserver(const Type& data, std::string prefix = "", std::string postfix = "") :
		m_data(data)
	{
		m_prefix = prefix;
		m_postfix = postfix;
	}

	~TypeEventObserver()
	{
	}

	std::string Read() override
	{
		if constexpr (std::is_same<Type, std::string>::value)
			return m_prefix + m_data + m_postfix;
		else
			return m_prefix + std::to_string(static_cast<const CastType>(m_data)) + m_postfix;
	}

private:

	const Type& m_data;
	std::string m_prefix;
	std::string m_postfix;
};