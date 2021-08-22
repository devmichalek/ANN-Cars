#pragma once
#include <map>
#include <fstream>

class AbstractBuilder
{
	// Validate internal fields
	virtual bool ValidateInternal() = 0;

	// Clears internal fields
	virtual void ClearInternal() = 0;

	// Sets internal fields from loaded file
	virtual bool LoadInternal(std::ifstream& input) = 0;

	// Saves internal fields into the file
	virtual bool SaveInternal(std::ofstream& output) = 0;

	// Create specific builder's dummy object
	virtual void CreateDummyInternal() = 0;

	// File reading mode
	std::ios_base::openmode m_inputMode;

	// File writing mode
	std::ios_base::openmode m_outputMode;

protected:

	enum
	{
		ERROR_UNKNOWN,
		SUCCESS_LOAD_COMPLETED,
		SUCCESS_SAVE_COMPLETED,
		SUCCESS_VALIDATION_PASSED,
		ERROR_EMPTY_FILENAME_CANNOT_OPEN_FILE_FOR_READING,
		ERROR_CANNOT_OPEN_FILE_FOR_READING,
		ERROR_EMPTY_FILENAME_CANNOT_OPEN_FILE_FOR_WRITING,
		ERROR_CANNOT_OPEN_FILE_FOR_WRITING,
		LAST_ENUM_OPERATION_INDEX
	};

	std::map<size_t, std::string> m_operationsMap;
	size_t m_lastOperationStatus;
	bool m_validated;

	// Validates internal fields
	bool Validate()
	{
		if (m_validated)
		{
			m_lastOperationStatus = SUCCESS_VALIDATION_PASSED;
			return true;
		}

		if (!ValidateInternal())
			return false;

		m_lastOperationStatus = SUCCESS_VALIDATION_PASSED;
		m_validated = true;
		return true;
	}

	AbstractBuilder(std::ios_base::openmode inputMode, std::ios_base::openmode outputMode) :
		m_inputMode(inputMode),
		m_outputMode(outputMode)
	{
		m_operationsMap[ERROR_UNKNOWN] = "Error: last status is unknown!";
		m_operationsMap[SUCCESS_LOAD_COMPLETED] = "Success: correctly opened file!";
		m_operationsMap[SUCCESS_SAVE_COMPLETED] = "Success: correctly saved file!";
		m_operationsMap[SUCCESS_VALIDATION_PASSED] = "Success: validation process has passed with no errors!";
		m_operationsMap[ERROR_EMPTY_FILENAME_CANNOT_OPEN_FILE_FOR_READING] = "Error: filename is empty, cannot open file for reading!";
		m_operationsMap[ERROR_CANNOT_OPEN_FILE_FOR_READING] = "Error: cannot open file for reading!";
		m_operationsMap[ERROR_EMPTY_FILENAME_CANNOT_OPEN_FILE_FOR_WRITING] = "Error: filename is empty, cannot open file for writing!";
		m_operationsMap[ERROR_CANNOT_OPEN_FILE_FOR_WRITING] = "Error: cannot open file for writing!";
		m_lastOperationStatus = ERROR_UNKNOWN;
		m_validated = false;
	}

public:

	virtual ~AbstractBuilder()
	{
	}

	// Creates dummy
	bool CreateDummy()
	{
		// Clear data
		Clear();

		// Call internal implementation
		CreateDummyInternal();

		// Validate
		return Validate();
	}

	// Clears internal fields
	void Clear()
	{
		m_lastOperationStatus = ERROR_UNKNOWN;
		m_validated = false;
		ClearInternal();
	}

	// Loads specific builder's data to file
	bool Load(std::string filename)
	{
		// Clear data
		Clear();

		// Check if filename is not empty
		if (filename.empty())
		{
			m_lastOperationStatus = ERROR_EMPTY_FILENAME_CANNOT_OPEN_FILE_FOR_READING;
			return false;
		}

		// Check if file can be opened for reading
		std::ifstream input(filename, m_inputMode);
		if (!input.is_open())
		{
			m_lastOperationStatus = ERROR_CANNOT_OPEN_FILE_FOR_READING;
			return false;
		}

		if (!LoadInternal(input))
			return false;

		m_lastOperationStatus = SUCCESS_LOAD_COMPLETED;
		m_validated = true;
		return true;
	}

	// Saves specific builder's data to file
	bool Save(std::string filename)
	{
		// Check if filename is not empty
		if (filename.empty())
		{
			m_lastOperationStatus = ERROR_EMPTY_FILENAME_CANNOT_OPEN_FILE_FOR_WRITING;
			return false;
		}

		// Validate
		if (!Validate())
			return false;

		// Check if file can be opened for writing
		std::ofstream output(filename, m_outputMode);
		if (!output.is_open())
		{
			m_lastOperationStatus = ERROR_CANNOT_OPEN_FILE_FOR_WRITING;
			return false;
		}

		if (!SaveInternal(output))
			return false;

		// Clear data
		Clear();

		m_lastOperationStatus = SUCCESS_SAVE_COMPLETED;
		return true;
	}

	// Returns last operation status
	// Returns true in case of success and false in case of failure
	std::pair<bool, std::string> GetLastOperationStatus()
	{
		std::string message = m_operationsMap[m_lastOperationStatus];
		switch (m_lastOperationStatus)
		{
			case SUCCESS_SAVE_COMPLETED:
			case SUCCESS_LOAD_COMPLETED:
			case SUCCESS_VALIDATION_PASSED:
				return std::make_pair(true, message);
			default:
				return std::make_pair(false, message);
		}

		return std::make_pair(false, message);
	}
};
