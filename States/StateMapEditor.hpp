#pragma once
#include "StateInterface.hpp"
#include "MapBuilder.hpp"
#include "VehicleBuilder.hpp"
#include "ContinuousTimer.hpp"
#include "Property.hpp"

class DoubleText;
class ObserverInterface;

class StateMapEditor final :
	public StateInterface
{
public:

	StateMapEditor(const StateMapEditor&) = delete;

	const StateMapEditor& operator=(const StateMapEditor&) = delete;

	StateMapEditor();

	~StateMapEditor();

	void Reload() override;

	void Capture() override;

	void Update() override;

	bool Load() override;

	void Draw() override;

private:

	// Called on edge insertion
	void InsertEdge(sf::Vector2f edgeEndPoint);

	// Called on edge removal
	void RemoveEdge(sf::Vector2f edgeEndPoint);

	// Modes
	enum
	{
		EDGE_MODE,
		VEHICLE_MODE,
		MODES_COUNT
	};
	std::array<std::string, MODES_COUNT> m_modeStrings;
	size_t m_mode;

	// Edge submodes
	enum
	{
		GLUED_INSERT_EDGE_SUBMODE,
		REMOVE_EDGE_SUBMODE,
		EDGE_SUBMODE_COUNT
	};
	std::array<std::string, EDGE_SUBMODE_COUNT> m_edgeSubmodeStrings;
	size_t m_edgeSubmode;

	VehiclePrototype* m_vehiclePrototype;
	MapPrototype m_mapPrototype;
	bool m_insertEdge;
	bool m_removeEdge;
	sf::Vector2f m_edgeBeggining;
	bool m_upToDate;
	bool m_vehiclePositioned;
	sf::RectangleShape m_allowedMapAreaShape;
	sf::RectangleShape m_allowedViewAreaShape;

	// View movement
	ContinuousTimer m_viewMovementTimer;
	Property<double> m_viewMovement;

	// Builders
	MapBuilder m_mapBuilder;
	VehicleBuilder m_vehicleBuilder;

	// Texts and text observers
	enum
	{
		MODE_TEXT,
		MOVEMENT_TEXT,
		VIEW_OFFSET_X_TEXT,
		VIEW_OFFSET_Y_TEXT,
		FILENAME_TEXT,
		EDGE_SUBMODE_TEXT,
		EDGE_COUNT_TEXT,
		VEHICLE_POSITIONED_TEXT,
		VEHICLE_ANGLE_TEXT,
		TEXT_COUNT
	};
	std::vector<DoubleText*> m_texts;
	std::vector<ObserverInterface*> m_textObservers;
};
