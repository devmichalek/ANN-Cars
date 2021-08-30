#pragma once
#include "StateInterface.hpp"
#include "DrawableMapBuilder.hpp"
#include "DrawableVehicleBuilder.hpp"
#include "ArtificialNeuralNetworkBuilder.hpp"
#include "DrawableMap.hpp"

class DrawableTripleText;
class ObserverIf;

class StateTesting final :
	public StateInterface
{
	enum
	{
		STOPPED_MODE,
		RUNNING_MODE,
		MODES_COUNT
	};
	std::array<std::string, MODES_COUNT> m_modeStrings;
	size_t m_mode;
	
	enum
	{
		MAP_FILENAME_TYPE,
		ANN_FILENAME_TYPE,
		VEHICLE_FILENAME_TYPE,
		FILENAME_TYPES_COUNT
	};
	std::array<std::string, FILENAME_TYPES_COUNT> m_filenameTypeStrings;
	size_t m_filenameType;

	// Control keys
	std::pair<sf::Keyboard::Key, bool> m_modeKey;
	std::pair<sf::Keyboard::Key, bool> m_filenameTypeKey;

	// Objects of test
	DrawableMap* m_drawableMap;
	DrawableVehicle* m_userVehicle;
	DrawableVehicleFactory m_drawableVehicleFactory;

	// Builders
	DrawableMapBuilder m_drawableMapBuilder;
	DrawableVehicleBuilder m_drawableVehicleBuilder;
	ArtificialNeuralNetworkBuilder m_artificialNeuralNetworkBuilder;

	// Texts and text observers
	enum
	{
		ACTIVE_MODE_TEXT,
		FITNESS_TEXT,
		FILENAME_TYPE_TEXT,
		FILENAME_TEXT,
		TEXT_COUNT
	};
	std::vector<DrawableTripleText*> m_texts;
	std::vector<ObserverIf*> m_textObservers;

public:

	StateTesting(const StateTesting&) = delete;

	const StateTesting& operator=(const StateTesting&) = delete;

	StateTesting();

	~StateTesting();

	void Reload() override;

	void Capture() override;

	void Update() override;

	bool Load() override;

	void Draw() override;
};
