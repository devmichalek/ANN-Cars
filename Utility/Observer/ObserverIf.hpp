#pragma once
#include <string>

class ObserverIf
{
public:

	virtual ~ObserverIf()
	{
	}

	// Reads observed value
	virtual std::string Read() = 0;

	// Returns true if value was marked as updated
	virtual bool Ready() = 0;

	// Resets observer
	virtual void Reset() = 0;

	// Notify observer that action took place
	virtual void Notify() = 0;
};