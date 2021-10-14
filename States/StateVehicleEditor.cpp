#include "StateVehicleEditor.hpp"
#include "FunctionTimerObserver.hpp"
#include "VariableText.hpp"
#include "FilenameText.hpp"
#include "CoreLogger.hpp"
#include <functional>

StateVehicleEditor::StateVehicleEditor()
{
	m_modeStrings[MODE_VEHICLE_BODY] = "Vehicle body";
	m_modeStrings[MODE_VEHICLE_SENSORS] = "Vehicle sensors";
	m_mode = MODE_VEHICLE_BODY;
	m_vehicleSensorsSubmodeStrings[VEHICLE_SENSORS_INSERT] = "Inserting";
	m_vehicleSensorsSubmodeStrings[VEHICLE_SENSORS_REMOVE] = "Removing";
	m_vehicleSensorsSubmode = VEHICLE_SENSORS_INSERT;

	m_controlKeys[sf::Keyboard::F1] = CHANGE_TO_VEHICLE_BODY_MODE;
	m_controlKeys[sf::Keyboard::F2] = CHANGE_TO_VEHICLE_SENSORS_MODE;
	m_controlKeys[sf::Keyboard::Num1] = CHANGE_TO_INSERT_STATE;
	m_controlKeys[sf::Keyboard::Numpad1] = CHANGE_TO_INSERT_STATE;
	m_controlKeys[sf::Keyboard::Num2] = CHANGE_TO_REMOVE_STATE;
	m_controlKeys[sf::Keyboard::BackSpace] = REMOVE_LAST_VEHICLE_BODY_POINT;
	m_controlKeys[sf::Keyboard::Numpad2] = CHANGE_TO_REMOVE_STATE;
	m_controlKeys[sf::Keyboard::RAlt] = CHANGE_SENSOR;
	m_controlKeys[sf::Keyboard::LAlt] = CHANGE_SENSOR;
	m_controlKeys[sf::Keyboard::Z] = DEACREASE_SENSOR_ANGLE;
	m_controlKeys[sf::Keyboard::X] = INCREASE_SENSOR_ANGLE;
	m_controlKeys[sf::Keyboard::C] = DECREASE_SENSOR_MOTION_RANGE;
	m_controlKeys[sf::Keyboard::V] = INCREASE_SENSOR_MOTION_RANGE;

	for (size_t i = 0; i < CONTROL_KEYS_COUNT; ++i)
		m_pressedKeys[i] = false;

	auto maxVehicleSize = VehicleBuilder::GetMaxVehicleBodyBound();
	auto windowSize = CoreWindow::GetSize();
	m_allowedAreaShape.setFillColor(sf::Color(255, 255, 255, 0));
	m_allowedAreaShape.setOutlineColor(sf::Color(0, 0, 255, 64));
	m_allowedAreaShape.setOutlineThickness(2);
	m_allowedAreaShape.setSize(maxVehicleSize);
	m_allowedAreaShape.setPosition((windowSize.x / 2.0f - maxVehicleSize.x / 2.0f), (windowSize.y / 2.0f - maxVehicleSize.y / 2.0f));
	
	m_vehicleBuilder.CreateDummy();
	m_vehiclePrototype = m_vehicleBuilder.Get();

	m_currentSensorIndex = m_vehiclePrototype->GetNumberOfSensors() - 1;
	m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
	m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
	m_upToDate = false;

	// Set axes
	auto windowCenter = CoreWindow::GetCenter();
	m_yAxis[0].color = m_yAxis[1].color = m_xAxis[0].color = m_xAxis[1].color = sf::Color(0, 0, 255, 64);
	m_xAxis[0].position = sf::Vector2f(windowCenter.x, 0);
	m_xAxis[1].position = sf::Vector2f(windowCenter.x, windowSize.y);
	m_yAxis[0].position = sf::Vector2f(0, windowCenter.y);
	m_yAxis[1].position = sf::Vector2f(windowSize.x, windowCenter.y);

	m_texts.resize(TEXT_COUNT, nullptr);
	m_textObservers.resize(TEXT_COUNT, nullptr);
}

StateVehicleEditor::~StateVehicleEditor()
{
	delete m_vehiclePrototype;
	for (auto& text : m_texts)
		delete text;
	for (auto& observer : m_textObservers)
		delete observer;
}

void StateVehicleEditor::Reload()
{
	m_mode = MODE_VEHICLE_BODY;
	m_vehicleSensorsSubmode = VEHICLE_SENSORS_INSERT;

	// Reset pressed keys
	for (size_t i = 0; i < CONTROL_KEYS_COUNT; ++i)
		m_pressedKeys[i] = false;

	// Reset subjects of change
	m_vehicleBuilder.CreateDummy();
	delete m_vehiclePrototype;
	m_vehiclePrototype = m_vehicleBuilder.Get();

	m_currentSensorIndex = m_vehiclePrototype->GetNumberOfSensors() - 1;
	m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
	m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
	m_upToDate = false;

	// Reset texts and text observers
	for (size_t i = 0; i < TEXT_COUNT; ++i)
	{
		if (m_textObservers[i])
			m_textObservers[i]->Notify();
		m_texts[i]->Reset();
	}

	// Reset view
	auto& view = CoreWindow::GetView();
	auto viewOffset = CoreWindow::GetViewOffset();
	view.move(-viewOffset);
	CoreWindow::GetRenderWindow().setView(view);
}

void StateVehicleEditor::Capture()
{
	auto* filenameText = static_cast<FilenameText<true, true>*>(m_texts[FILENAME_TEXT]);
	filenameText->Capture();
	if (!filenameText->IsRenaming())
	{
		if (CoreWindow::GetEvent().type == sf::Event::KeyPressed)
		{
			auto eventKey = CoreWindow::GetEvent().key.code;
			auto iterator = m_controlKeys.find(eventKey);
			if (iterator != m_controlKeys.end() && !m_pressedKeys[iterator->second])
			{
				m_pressedKeys[iterator->second] = true;
				switch (iterator->second)
				{
					case CHANGE_TO_VEHICLE_BODY_MODE:
						m_mode = MODE_VEHICLE_BODY;
						m_vehicleSensorsSubmode = VEHICLE_SENSORS_INSERT;
						m_currentSensorIndex = 0;
						m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
						m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
						break;
					case CHANGE_TO_VEHICLE_SENSORS_MODE:
						m_mode = MODE_VEHICLE_SENSORS;
						m_vehicleSensorsSubmode = VEHICLE_SENSORS_INSERT;
						m_currentSensorIndex = 0;
						m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
						m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
						break;
					case CHANGE_TO_INSERT_STATE:
						m_vehicleSensorsSubmode = VEHICLE_SENSORS_INSERT;
						break;
					case CHANGE_TO_REMOVE_STATE:
						m_vehicleSensorsSubmode = VEHICLE_SENSORS_REMOVE;
						break;
					case REMOVE_LAST_VEHICLE_BODY_POINT:
						m_vehiclePrototype->RemoveLastBodyPoint();
						m_upToDate = false;
						break;
					case CHANGE_SENSOR:
						if (m_mode != MODE_VEHICLE_SENSORS)
							break;
						++m_currentSensorIndex;
						if (m_currentSensorIndex >= m_vehiclePrototype->GetNumberOfSensors())
							m_currentSensorIndex = 0;
						m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
						m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
						m_upToDate = false;
						break;
					case DEACREASE_SENSOR_ANGLE:
						if (m_mode != MODE_VEHICLE_SENSORS)
							break;
						if (m_vehiclePrototype->GetNumberOfSensors() > 0)
						{
							m_currentSensorAngle -= VehicleBuilder::GetDefaultSensorAngleOffset();
							if (m_currentSensorAngle < VehicleBuilder::GetMinSensorAngle())
								m_currentSensorAngle = VehicleBuilder::GetMaxSensorAngle() - VehicleBuilder::GetDefaultSensorAngleOffset();
							m_vehiclePrototype->SetSensorBeamAngle(m_currentSensorIndex, m_currentSensorAngle);
							m_upToDate = false;
						}
						break;
					case INCREASE_SENSOR_ANGLE:
						if (m_mode != MODE_VEHICLE_SENSORS)
							break;
						if (m_vehiclePrototype->GetNumberOfSensors() > 0)
						{
							m_currentSensorAngle += VehicleBuilder::GetDefaultSensorAngleOffset();
							if (m_currentSensorAngle > VehicleBuilder::GetMaxSensorAngle())
								m_currentSensorAngle = VehicleBuilder::GetMinSensorAngle() + VehicleBuilder::GetDefaultSensorAngleOffset();
							else if (m_currentSensorAngle == VehicleBuilder::GetMaxSensorAngle())
								m_currentSensorAngle = VehicleBuilder::GetMinSensorAngle();
							m_vehiclePrototype->SetSensorBeamAngle(m_currentSensorIndex, m_currentSensorAngle);
							m_upToDate = false;
						}
						break;
					case DECREASE_SENSOR_MOTION_RANGE:
						if (m_mode != MODE_VEHICLE_SENSORS)
							break;
						if (m_vehiclePrototype->GetNumberOfSensors() > 0)
						{
							m_currentSensorMotionRange -= VehicleBuilder::GetDefaultSensorMotionRange();
							if (m_currentSensorMotionRange < VehicleBuilder::GetMinSensorMotionRange())
								m_currentSensorMotionRange = VehicleBuilder::GetMinSensorMotionRange();
							m_vehiclePrototype->SetSensorMotionRange(m_currentSensorIndex, m_currentSensorMotionRange);
							m_upToDate = false;
						}
						break;
					case INCREASE_SENSOR_MOTION_RANGE:
						if (m_mode != MODE_VEHICLE_SENSORS)
							break;
						if (m_vehiclePrototype->GetNumberOfSensors() > 0)
						{
							m_currentSensorMotionRange += VehicleBuilder::GetDefaultSensorMotionRange();
							if (m_currentSensorMotionRange > VehicleBuilder::GetMaxSensorMotionRange())
								m_currentSensorMotionRange = VehicleBuilder::GetMaxSensorMotionRange();
							m_vehiclePrototype->SetSensorMotionRange(m_currentSensorIndex, m_currentSensorMotionRange);
							m_upToDate = false;
						}
						break;
				}
			}
		}
		else if (CoreWindow::GetEvent().type == sf::Event::KeyReleased)
		{
			auto eventKey = CoreWindow::GetEvent().key.code;
			auto iterator = m_controlKeys.find(eventKey);
			if (iterator != m_controlKeys.end())
				m_pressedKeys[iterator->second] = false;
		}
		else if (CoreWindow::GetEvent().type == sf::Event::MouseButtonPressed)
		{
			auto relativePosition = CoreWindow::GetMousePosition();
			if (DrawableMath::IsPointInsideRectangle(m_allowedAreaShape.getSize(), m_allowedAreaShape.getPosition(), relativePosition))
			{
				relativePosition -= CoreWindow::GetCenter();
				switch (m_mode)
				{
					case MODE_VEHICLE_BODY:
					{
						m_vehiclePrototype->AddBodyPoint(relativePosition);
						m_upToDate = false;
						break;
					}
					case MODE_VEHICLE_SENSORS:
					{
						switch (m_vehicleSensorsSubmode)
						{
							case VEHICLE_SENSORS_INSERT:
								m_vehiclePrototype->AddSensor(relativePosition,
															 VehicleBuilder::GetMinSensorAngle(),
															 VehicleBuilder::GetDefaultSensorMotionRange());
								m_currentSensorIndex = m_vehiclePrototype->GetNumberOfSensors() - 1;
								m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
								m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
								m_upToDate = false;
								break;
							case VEHICLE_SENSORS_REMOVE:
							{
								size_t index = (size_t)-1;
								if (m_vehiclePrototype->GetSensorIndex(index, relativePosition))
								{
									size_t currentLastIndex = m_vehiclePrototype->GetNumberOfSensors() - 1;
									if (m_currentSensorIndex == index && currentLastIndex == index)
									{
										if (currentLastIndex == 0)
										{
											m_currentSensorAngle = VehicleBuilder::GetMinSensorAngle();
											m_currentSensorMotionRange = VehicleBuilder::GetDefaultSensorMotionRange();
										}
										else
										{
											--m_currentSensorIndex;
											m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
											m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
										}
									}
									m_vehiclePrototype->RemoveSensor(index);
									m_upToDate = false;
								}
								break;
							}
						}

						break;
					}
				}
			}
		}
	}
	else
		m_upToDate = false;
}

void StateVehicleEditor::Update()
{
	auto* filenameText = static_cast<FilenameText<true, true>*>(m_texts[FILENAME_TEXT]);
	if (filenameText->IsWriting())
	{
		if (!m_upToDate)
		{
			m_vehicleBuilder.Clear();
			for (const auto& point : m_vehiclePrototype->GetBodyPoints())
				m_vehicleBuilder.AddVehicleBodyPoint(point);
			size_t numberOfSensors = m_vehiclePrototype->GetNumberOfSensors();
			for (size_t i = 0; i < numberOfSensors; ++i)
				m_vehicleBuilder.AddVehicleSensor(m_vehiclePrototype->GetSensorPoint(i),
												  m_vehiclePrototype->GetSensorBeamAngle(i),
												  m_vehiclePrototype->GetSensorMotionRange(i));

			bool success = m_vehicleBuilder.Save(filenameText->GetFilename());
			auto status = m_vehicleBuilder.GetLastOperationStatus();
			filenameText->ShowStatusText();
			if (success)
				filenameText->SetSuccessStatusText(status.second);
			else
				filenameText->SetErrorStatusText(status.second);
			m_upToDate = success;
		}
	}
	else if (filenameText->IsReading())
	{
		bool success = m_vehicleBuilder.Load(filenameText->GetFilename());
		auto status = m_vehicleBuilder.GetLastOperationStatus();
		filenameText->ShowStatusText();
		if (success)
		{
			filenameText->SetSuccessStatusText(status.second);
			delete m_vehiclePrototype;
			m_vehiclePrototype = m_vehicleBuilder.Get();
			m_currentSensorIndex = 0;
			m_currentSensorAngle = m_vehiclePrototype->GetSensorBeamAngle(m_currentSensorIndex);
			m_currentSensorMotionRange = m_vehiclePrototype->GetSensorMotionRange(m_currentSensorIndex);
			m_upToDate = true;
		}
		else
			filenameText->SetErrorStatusText(status.second);
	}

	m_vehiclePrototype->Update();

	for (const auto& text : m_texts)
		text->Update();
}

bool StateVehicleEditor::Load()
{
	// Create texts
	m_texts[BACK_TEXT] = new VariableText({ "Back" });
	m_texts[FRONT_TEXT] = new VariableText({ "Front" });
	m_texts[ACTIVE_MODE_TEXT] = new TripleText({ "Active mode:", "", "| [F1] [F2]" });
	m_texts[TOTAL_NUMBER_OF_EDGES_TEXT] = new TripleText({ "Total number of edges:", "", "| [RMB] [Backspace]" });
	m_texts[TOTAL_NUMBER_OF_SENSORS_TEXT] = new TripleText({ "Total number of sensors:", "", "| [RMB]" });
	m_texts[FILENAME_TEXT] = new FilenameText<true, true>("vehicle.bin");
	m_texts[VEHICLE_SENSORS_SUBMODE_TEXT] = new TripleText({ "Active submode:", "", "| [1] [2]" });
	m_texts[CURRENT_SENSOR_TEXT] = new TripleText({ "Current sensor:", "", "| [Alt]" });
	m_texts[CURRENT_SENSOR_ANGLE_TEXT] = new TripleText({ "Current sensor angle:", "", "| [Z] [X]" });
	m_texts[CURRENT_SENSOR_MOTION_RANGE_TEXT] = new TripleText({ "Current sensor motion range:", "", "| [C] [V]" });

	// Create observers
	m_textObservers[ACTIVE_MODE_TEXT] = new FunctionTimerObserver<std::string>([&] { return m_modeStrings[m_mode]; }, 0.2);
	m_textObservers[TOTAL_NUMBER_OF_EDGES_TEXT] = new FunctionTimerObserver<std::string>([&] { return std::to_string(m_vehiclePrototype->GetNumberOfBodyPoints()); }, 0.1);
	m_textObservers[TOTAL_NUMBER_OF_SENSORS_TEXT] = new FunctionTimerObserver<std::string>([&] { return std::to_string(m_vehiclePrototype->GetNumberOfSensors()); }, 0.1);
	m_textObservers[VEHICLE_SENSORS_SUBMODE_TEXT] = new FunctionTimerObserver<std::string>([&] { return m_vehicleSensorsSubmodeStrings[m_vehicleSensorsSubmode]; }, 0.2);
	m_textObservers[CURRENT_SENSOR_TEXT] = new FunctionTimerObserver<std::string>([&] { return m_vehiclePrototype->GetNumberOfSensors() > 0 ? "S" + std::to_string(m_currentSensorIndex) : "None"; }, 0.1);
	m_textObservers[CURRENT_SENSOR_ANGLE_TEXT] = new FunctionTimerObserver<std::string>([&] { return m_vehiclePrototype->GetNumberOfSensors() > 0 ? std::to_string(size_t(m_currentSensorAngle)) : "Unknown"; }, 0.1);
	m_textObservers[CURRENT_SENSOR_MOTION_RANGE_TEXT] = new FunctionTimerObserver<std::string>([&] { return m_vehiclePrototype->GetNumberOfSensors() > 0 ? std::to_string(m_currentSensorMotionRange) : "Unknown"; }, 0.1);

	// Set text observers
	for (size_t i = ACTIVE_MODE_TEXT; i < TEXT_COUNT; ++i)
		((DoubleText*)m_texts[i])->SetObserver(m_textObservers[i]);

	// Set text character size and rotation
	auto* backText = static_cast<VariableText*>(m_texts[BACK_TEXT]);
	auto* frontText = static_cast<VariableText*>(m_texts[FRONT_TEXT]);
	backText->SetCharacterSize(4);
	frontText->SetCharacterSize(4);
	backText->SetRotation(270.0f);
	frontText->SetRotation(90.0f);

	// Set text positions
	m_texts[BACK_TEXT]->SetPosition({ FontContext::Component(25), {2} });
	m_texts[FRONT_TEXT]->SetPosition({ FontContext::Component(18), {2, true} });
	m_texts[ACTIVE_MODE_TEXT]->SetPosition({ FontContext::Component(0), {0}, {5}, {9} });
	m_texts[TOTAL_NUMBER_OF_EDGES_TEXT]->SetPosition({ FontContext::Component(1), {0}, {5}, {9} });
	m_texts[TOTAL_NUMBER_OF_SENSORS_TEXT]->SetPosition({ FontContext::Component(2), {0}, {5}, {9} });
	m_texts[FILENAME_TEXT]->SetPosition({ FontContext::Component(3), {0}, {5}, {9}, {18} });
	m_texts[VEHICLE_SENSORS_SUBMODE_TEXT]->SetPosition({ FontContext::Component(4, true), {0}, {6}, {8} });
	m_texts[CURRENT_SENSOR_TEXT]->SetPosition({ FontContext::Component(3, true), {0}, {6}, {8} });
	m_texts[CURRENT_SENSOR_ANGLE_TEXT]->SetPosition({ FontContext::Component(2, true), {0}, {6}, {8} });
	m_texts[CURRENT_SENSOR_MOTION_RANGE_TEXT]->SetPosition({ FontContext::Component(1, true), {0}, {6}, {8} });

	CoreLogger::PrintSuccess("State \"Vehicle Editor\" dependencies loaded correctly");
	return true;
}

void StateVehicleEditor::Draw()
{
	m_vehiclePrototype->DrawBeams();
	m_vehiclePrototype->DrawBody();

	CoreWindow::GetRenderWindow().draw(m_xAxis.data(), m_xAxis.size(), sf::Lines);
	CoreWindow::GetRenderWindow().draw(m_yAxis.data(), m_xAxis.size(), sf::Lines);
	CoreWindow::GetRenderWindow().draw(m_allowedAreaShape);

	m_texts[BACK_TEXT]->Draw();
	m_texts[FRONT_TEXT]->Draw();
	m_texts[ACTIVE_MODE_TEXT]->Draw();
	m_texts[TOTAL_NUMBER_OF_EDGES_TEXT]->Draw();
	m_texts[TOTAL_NUMBER_OF_SENSORS_TEXT]->Draw();
	m_texts[FILENAME_TEXT]->Draw();
	
	if (m_mode == MODE_VEHICLE_SENSORS)
	{
		m_texts[VEHICLE_SENSORS_SUBMODE_TEXT]->Draw();
		m_texts[CURRENT_SENSOR_TEXT]->Draw();
		m_texts[CURRENT_SENSOR_ANGLE_TEXT]->Draw();
		m_texts[CURRENT_SENSOR_MOTION_RANGE_TEXT]->Draw();
	}
}
