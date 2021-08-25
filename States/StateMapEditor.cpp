#include "StateMapEditor.hpp"
#include "CoreWindow.hpp"
#include "DrawableFilenameText.hpp"
#include "TypeTimerObserver.hpp"
#include "FunctionTimerObserver.hpp"
#include "FunctionEventObserver.hpp"
#include "CoreLogger.hpp"
#include <functional>

void StateMapEditor::SetActiveMode(ActiveMode activeMode)
{
	if (m_activeMode != activeMode)
	{
		switch (m_activeMode)
		{
			case ActiveMode::EDGE:
				m_edgeSubmode = EdgeSubmode::GLUED_INSERT;
				m_insertEdge = false;
				m_removeEdge = false;
				m_textObservers[EDGE_SUBMODE_TEXT]->Notify();
				break;

			case ActiveMode::VEHICLE:
				m_vehicleSubmode = VehicleSubmode::INSERT;
				m_textObservers[VEHICLE_SUBMODE_TEXT]->Notify();
				break;
		}

		m_activeMode = activeMode;
		m_textObservers[ACTIVE_MODE_TEXT]->Notify();
	}
}

StateMapEditor::StateMapEditor() :
	m_drawableVehicle(nullptr),
	m_viewMovementTimer(0.0, 0.1),
	m_viewMovementOffset(20.0),
	m_viewMovement(800.0),
	m_viewMinMovement(200.0),
	m_viewMaxMovement(1800.0)
{
	m_activeMode = ActiveMode::EDGE;
	m_edgeSubmode = EdgeSubmode::GLUED_INSERT;
	m_vehicleSubmode = VehicleSubmode::INSERT;
	m_line[0].color = sf::Color::White;
	m_line[1].color = sf::Color::White;
	m_edges.reserve(1024);
	m_insertEdge = false;
	m_removeEdge = false;
	m_upToDate = false;
	m_vehiclePositioned = false;

	auto windowSize = CoreWindow::GetSize();
	auto maxSize = m_drawableMapBuilder.GetMaxAllowedMapArea();
	auto allowedShapePosition = sf::Vector2f(windowSize.x / 20.0f, windowSize.y / 20.0f);
	auto maxViewSize = m_drawableMapBuilder.GetMaxAllowedViewArea();
	m_allowedAreaShape.setFillColor(sf::Color(255, 255, 255, 0));
	m_allowedAreaShape.setOutlineColor(sf::Color(255, 255, 255, 64));
	m_allowedAreaShape.setOutlineThickness(2);
	m_allowedAreaShape.setSize(maxSize);
	m_allowedAreaShape.setPosition(allowedShapePosition);
	m_allowedViewAreaShape.setSize(maxViewSize);
	m_allowedViewAreaShape.setPosition(allowedShapePosition.x - (maxViewSize.x - maxSize.x) / 2, allowedShapePosition.y - (maxViewSize.y - maxSize.y) / 2);
	
	if (m_drawableVehicleBuilder.CreateDummy())
		m_drawableVehicle = m_drawableVehicleBuilder.Get();
	else
		CoreLogger::PrintError("Cannot create Drawable Vehicle dummy!");

	if (m_drawableMapBuilder.CreateDummy())
	{
		m_edges = m_drawableMapBuilder.GetEdges();
		m_vehiclePositioned = true;
		auto data = m_drawableMapBuilder.GetVehicle();
		m_drawableVehicle->SetCenter(data.first);
		m_drawableVehicle->SetAngle(data.second);
		m_drawableVehicle->Update();
	}
	else
		CoreLogger::PrintError("Cannot create Drawable Map dummy!");

	m_texts.resize(TEXT_COUNT, nullptr);
	m_textObservers.resize(TEXT_COUNT, nullptr);
}

StateMapEditor::~StateMapEditor()
{
	delete m_drawableVehicle;
	for (auto& text : m_texts)
		delete text;
	for (auto& observer : m_textObservers)
		delete observer;
}

void StateMapEditor::Reload()
{
	// Reset internal states
	m_activeMode = ActiveMode::EDGE;
	m_edgeSubmode = EdgeSubmode::GLUED_INSERT;
	m_vehicleSubmode = VehicleSubmode::INSERT;
	m_insertEdge = false;
	m_removeEdge = false;
	m_edgeBeggining = sf::Vector2f(0.0, 0.0);
	m_upToDate = false;
	delete m_drawableVehicle;
	m_drawableVehicle = m_drawableVehicleBuilder.Get();
	m_viewMovementTimer.Reset();

	if (m_drawableMapBuilder.CreateDummy())
	{
		m_edges = m_drawableMapBuilder.GetEdges();
		m_vehiclePositioned = true;
		auto data = m_drawableMapBuilder.GetVehicle();
		if (m_drawableVehicle)
		{
			m_drawableVehicle->SetCenter(data.first);
			m_drawableVehicle->SetAngle(data.second);
			m_drawableVehicle->Update();
		}
	}
	else
	{
		m_edges.clear();
		m_vehiclePositioned = false;
		CoreLogger::PrintError("Cannot create Drawable Map dummy!");
	}

	// Reset texts
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

void StateMapEditor::Capture()
{
	if (CoreWindow::GetEvent().type == sf::Event::MouseButtonPressed)
	{
		sf::Vector2f correctPosition = CoreWindow::GetMousePosition() + CoreWindow::GetViewOffset();
		if (DrawableMath::IsPointInsideRectangle(m_allowedAreaShape.getSize(), m_allowedAreaShape.getPosition(), correctPosition))
		{
			switch (m_activeMode)
			{
			case ActiveMode::EDGE:
			{
				switch (m_edgeSubmode)
				{
					case EdgeSubmode::GLUED_INSERT:
					{
						if (m_insertEdge)
						{
							if (!m_edges.empty())
							{
								size_t size = m_edges.size() - 1;
								Edge temporaryEdge = { m_edgeBeggining, correctPosition };
								sf::Vector2f intersectionPoint;
								for (size_t i = 0; i < size; ++i)
								{
									if (DrawableMath::GetIntersectionPoint(m_edges[i], temporaryEdge, intersectionPoint))
									{
										auto segment = DrawableMath::Distance(m_edges[i][0], intersectionPoint);
										auto length = DrawableMath::Distance(m_edges[i]);
										double percentage = segment / length;
										if (percentage > 0.01 && percentage < 0.5)
											correctPosition = m_edges[i][0];
										else if (percentage > 0.5 && percentage < 0.99)
											correctPosition = m_edges[i][1];
										else
											continue;
										m_insertEdge = false;
										break;
									}
								}
							}

							Edge newEdge;
							newEdge[0] = m_edgeBeggining;
							newEdge[1] = correctPosition;
							m_edges.push_back(newEdge);
							m_upToDate = false;
						}
						else
							m_insertEdge = true;

						m_edgeBeggining = correctPosition;
						break;
					}
					case EdgeSubmode::REMOVE:
					{
						if (m_removeEdge)
						{
							size_t size = m_edges.size();
							for (size_t i = 0; i < size; ++i)
							{
								if (DrawableMath::Intersect(m_edges[i], m_edgeBeggining, correctPosition))
								{
									m_upToDate = false;
									m_edges.erase(m_edges.begin() + i);
									--size;
									--i;
								}
							}
						}
						else
							m_edgeBeggining = correctPosition;

						m_removeEdge = !m_removeEdge;
						break;
					}
					}
					break;
				}
				case ActiveMode::VEHICLE:
				{
					switch (m_vehicleSubmode)
					{
					case VehicleSubmode::INSERT:
					{
						m_vehiclePositioned = true;
						m_drawableVehicle->SetCenter(correctPosition);
						m_drawableVehicle->Update();
						m_upToDate = false;
						break;
					}
					case VehicleSubmode::REMOVE:
					{
						if (m_drawableVehicle->Inside(correctPosition))
						{
							m_vehiclePositioned = false;
							m_upToDate = false;
						}
						break;
					}
					default:
						break;
					}

					break;
				}

				default:
					break;
			}
		}
	}

	m_texts[FILENAME_TEXT]->Capture();
}

void StateMapEditor::Update()
{
	auto* filenameText = static_cast<DrawableFilenameText<true, true>*>(m_texts[FILENAME_TEXT]);
	if (filenameText->IsReading())
	{
		bool success = m_drawableMapBuilder.Load(filenameText->GetFilename());
		auto status = m_drawableMapBuilder.GetLastOperationStatus();
		filenameText->ShowStatusText();
		if (success)
		{
			filenameText->SetSuccessStatusText(status.second);
			m_edges = m_drawableMapBuilder.GetEdges();
			m_vehiclePositioned = true;
			auto data = m_drawableMapBuilder.GetVehicle();
			m_drawableVehicle->SetCenter(data.first);
			m_drawableVehicle->SetAngle(data.second);
			m_drawableVehicle->Update();
			m_upToDate = true;
		}
		else
			filenameText->SetErrorStatusText(status.second);
	}
	else if (filenameText->IsWriting())
	{
		if (!m_upToDate)
		{
			m_drawableMapBuilder.Clear();
			if (m_vehiclePositioned)
				m_drawableMapBuilder.AddVehicle(m_drawableVehicle->GetAngle(), m_drawableVehicle->GetCenter());
			for (auto& i : m_edges)
				m_drawableMapBuilder.AddEdge(i);
			bool success = m_drawableMapBuilder.Save(filenameText->GetFilename());
			std::string message;
			auto status = m_drawableMapBuilder.GetLastOperationStatus();
			filenameText->ShowStatusText();
			if (success)
				filenameText->SetSuccessStatusText(status.second);
			else
				filenameText->SetErrorStatusText(status.second);
			m_upToDate = success;
		}
	}
	else if (!filenameText->IsRenaming())
	{
		switch (m_activeMode)
		{
			case ActiveMode::EDGE:
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::F2))
					SetActiveMode(ActiveMode::VEHICLE);
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
					sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt))
				{
					if (!m_edges.empty() && m_edgeSubmode == EdgeSubmode::GLUED_INSERT)
					{
						sf::Vector2f correctPosition = CoreWindow::GetMousePosition() + CoreWindow::GetViewOffset();
						m_edgeBeggining = correctPosition;
						m_insertEdge = true;

						// Find closest point to mouse position
						size_t index = 0;
						double distance = std::numeric_limits<double>::max();
						for (auto& edge : m_edges)
						{
							double beginningDistance = DrawableMath::Distance(correctPosition, edge[0]);
							if (beginningDistance < distance)
							{
								distance = beginningDistance;
								m_edgeBeggining = edge[0];
							}

							double endDistance = DrawableMath::Distance(correctPosition, edge[1]);
							if (endDistance < distance)
							{
								distance = endDistance;
								m_edgeBeggining = edge[1];
							}
						}
					}
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
				{
					m_insertEdge = false;
					m_removeEdge = false;
				}
				else
				{
					EdgeSubmode mode = m_edgeSubmode;
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1) ||
						sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad1))
					{
						mode = EdgeSubmode::GLUED_INSERT;
					}
					else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2) ||
						sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad2))
					{
						mode = EdgeSubmode::REMOVE;
					}

					if (m_edgeSubmode != mode)
					{
						m_edgeSubmode = mode;
						m_insertEdge = false;
						m_removeEdge = false;
						m_textObservers[EDGE_SUBMODE_TEXT]->Notify();
					}
				}

				break;
			}
			case ActiveMode::VEHICLE:
			{
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::F1))
					SetActiveMode(ActiveMode::EDGE);
				else
				{
					VehicleSubmode mode = m_vehicleSubmode;
					if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1) ||
						sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad1))
					{
						mode = VehicleSubmode::INSERT;
					}
					else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2) ||
						sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad2))
					{
						mode = VehicleSubmode::REMOVE;
					}
					else if (m_vehiclePositioned)
					{
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))
						{
							m_drawableVehicle->Rotate(0.0);
							m_drawableVehicle->Update();
						}
						else if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))
						{
							m_drawableVehicle->Rotate(1.0);
							m_drawableVehicle->Update();
						}
					}

					if (m_vehicleSubmode != mode)
					{
						m_vehicleSubmode = mode;
						m_textObservers[VEHICLE_SUBMODE_TEXT]->Notify();
					}
				}

				break;
			}
			default:
				break;
		}

		float elapsedTime = float(CoreWindow::GetElapsedTime());
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Add))
		{
			if (m_viewMovementTimer.Update())
			{
				m_viewMovement += m_viewMovementOffset;
				if (m_viewMovement > m_viewMaxMovement)
					m_viewMovement = m_viewMaxMovement;
			}
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract))
		{
			if (m_viewMovementTimer.Update())
			{
				m_viewMovement -= m_viewMovementOffset;
				if (m_viewMovement < m_viewMinMovement)
					m_viewMovement = m_viewMinMovement;
			}
		}

		auto& view = CoreWindow::GetView();
		auto viewPosition = view.getCenter() - (CoreWindow::GetSize() / 2.0f);
		float moveOffset = static_cast<float>(m_viewMovement * elapsedTime);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			view.move(sf::Vector2f(-moveOffset, 0));
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			view.move(sf::Vector2f(moveOffset, 0));

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			view.move(sf::Vector2f(0, -moveOffset));
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			view.move(sf::Vector2f(0, moveOffset));

		view = CoreWindow::GetView();
		viewPosition = view.getCenter() - (CoreWindow::GetSize() / 2.0f);
		float left = m_allowedViewAreaShape.getPosition().x;
		float right = m_allowedViewAreaShape.getPosition().x + m_allowedViewAreaShape.getSize().x;
		float top = m_allowedViewAreaShape.getPosition().y;
		float bot = m_allowedViewAreaShape.getPosition().y + m_allowedViewAreaShape.getSize().y;

		if (viewPosition.x < left)
			view.move(sf::Vector2f(left - viewPosition.x, 0));
		else if (viewPosition.x + CoreWindow::GetSize().x > right)
			view.move(sf::Vector2f(-(viewPosition.x + CoreWindow::GetSize().x - right), 0));

		if (viewPosition.y < top)
			view.move(sf::Vector2f(0, top - viewPosition.y));
		else if (viewPosition.y + CoreWindow::GetSize().y > bot)
			view.move(sf::Vector2f(0, -(viewPosition.y + CoreWindow::GetSize().y - bot)));

		CoreWindow::GetRenderWindow().setView(view);
	}

	for (const auto& text : m_texts)
		text->Update();
}

bool StateMapEditor::Load()
{
	// Set strigs map
	m_activeModeMap[ActiveMode::EDGE] = "Edge mode";
	m_activeModeMap[ActiveMode::VEHICLE] = "Vehicle mode";
	m_edgeSubmodeMap[EdgeSubmode::GLUED_INSERT] = "Glued insert mode";
	m_edgeSubmodeMap[EdgeSubmode::REMOVE] = "Remove mode";
	m_vehicleSubmodeMap[VehicleSubmode::INSERT] = "Insert mode";
	m_vehicleSubmodeMap[VehicleSubmode::REMOVE] = "Remove mode";

	// Create texts
	m_texts[ACTIVE_MODE_TEXT] = new DrawableTripleText;
	m_texts[MOVEMENT_TEXT] = new DrawableTripleText;
	m_texts[VIEW_OFFSET_X_TEXT] = new DrawableTripleText;
	m_texts[VIEW_OFFSET_Y_TEXT] = new DrawableTripleText;
	m_texts[FILENAME_TEXT] = new DrawableFilenameText<true, true>;
	m_texts[EDGE_SUBMODE_TEXT] = new DrawableTripleText;
	m_texts[EDGE_COUNT_TEXT] = new DrawableDoubleText;
	m_texts[VEHICLE_SUBMODE_TEXT] = new DrawableTripleText;
	m_texts[VEHICLE_ANGLE_TEXT] = new DrawableTripleText;

	// Create observers
	m_textObservers[ACTIVE_MODE_TEXT] = new FunctionEventObserver<std::string>([&] { return m_activeModeMap[m_activeMode]; });
	m_textObservers[MOVEMENT_TEXT] = new TypeTimerObserver<double, int>(m_viewMovement, 0.05);
	m_textObservers[VIEW_OFFSET_X_TEXT] = new FunctionTimerObserver<std::string>([&] { return std::to_string(int(CoreWindow::GetViewOffset().x)); }, 0.05);
	m_textObservers[VIEW_OFFSET_Y_TEXT] = new FunctionTimerObserver<std::string>([&] { return std::to_string(int(CoreWindow::GetViewOffset().y)); }, 0.05);
	m_textObservers[FILENAME_TEXT] = nullptr;
	m_textObservers[EDGE_SUBMODE_TEXT] = new FunctionEventObserver<std::string>([&] { return m_edgeSubmodeMap[m_edgeSubmode]; });
	m_textObservers[EDGE_COUNT_TEXT] = new FunctionTimerObserver<std::string>([&] { return std::to_string(m_edges.size()); }, 0.5);
	m_textObservers[VEHICLE_SUBMODE_TEXT] = new FunctionEventObserver<std::string>([&] { return m_vehicleSubmodeMap[m_vehicleSubmode]; });
	m_textObservers[VEHICLE_ANGLE_TEXT] = new FunctionTimerObserver<std::string>([&] {
											double angle = double((long long)(m_drawableVehicle->GetAngle()) % 360);
											return std::to_string(DrawableMath::CastAtan2ToFullAngle(angle)); },
											0.2);

	// Set texts strings
	m_texts[ACTIVE_MODE_TEXT]->SetStrings({ "Active mode:", "",  "| [F1] [F2]" });
	m_texts[MOVEMENT_TEXT]->SetStrings({ "Movement:", "", "| [+] [-]" });
	m_texts[VIEW_OFFSET_X_TEXT]->SetStrings({ "View offset x:", "", "| [A] [D]" });
	m_texts[VIEW_OFFSET_Y_TEXT]->SetStrings({ "View offset y:", "", "| [W] [S]" });
	m_texts[EDGE_SUBMODE_TEXT]->SetStrings({ "Current mode:", "", "| [1] [2] [RMB] [Alt] [Esc]" });
	m_texts[EDGE_COUNT_TEXT]->SetStrings({ "Edge count:" });
	m_texts[VEHICLE_SUBMODE_TEXT]->SetStrings({ "Current mode:", "", "| [1] [2] [RMB]" });
	m_texts[VEHICLE_ANGLE_TEXT]->SetStrings({ "Vehicle angle:", "", "| [Z] [X]" });

	// Set text observers
	for (size_t i = 0; i < TEXT_COUNT; ++i)
		m_texts[i]->SetObserver(m_textObservers[i]);
	
	// Set text positions
	m_texts[ACTIVE_MODE_TEXT]->SetPosition({ FontContext::Component(0), {0}, {3}, {7} });
	m_texts[MOVEMENT_TEXT]->SetPosition({ FontContext::Component(1), {0}, {3}, {7} });
	m_texts[VIEW_OFFSET_X_TEXT]->SetPosition({ FontContext::Component(2), {0}, {3}, {7} });
	m_texts[VIEW_OFFSET_Y_TEXT]->SetPosition({ FontContext::Component(3), {0}, {3}, {7} });
	m_texts[FILENAME_TEXT]->SetPosition({ FontContext::Component(4), {0}, {3}, {7}, {16} });
	m_texts[EDGE_SUBMODE_TEXT]->SetPosition({ FontContext::Component(1, true), {0}, {3}, {7} });
	m_texts[EDGE_COUNT_TEXT]->SetPosition({ FontContext::Component(2, true), {0}, {3} });
	m_texts[VEHICLE_SUBMODE_TEXT]->SetPosition({ FontContext::Component(1, true), {0}, {3}, {7} });
	m_texts[VEHICLE_ANGLE_TEXT]->SetPosition({ FontContext::Component(2, true), {0}, {3}, {7} });

	CoreLogger::PrintSuccess("State \"Map Editor\" dependencies loaded correctly");
	return true;
}

void StateMapEditor::Draw()
{
	if (m_vehiclePositioned)
	{
		m_drawableVehicle->DrawBody();
		m_drawableVehicle->DrawBeams();
	}

	m_line[0].color = sf::Color::White;
	m_line[1].color = m_line[0].color;
	for (const auto& i : m_edges)
	{
		m_line[0].position = i[0];
		m_line[1].position = i[1];
		CoreWindow::GetRenderWindow().draw(m_line.data(), m_line.size(), sf::Lines);
	}

	CoreWindow::GetRenderWindow().draw(m_allowedAreaShape);

	switch (m_activeMode)
	{
		case ActiveMode::EDGE:
		{
			if (m_insertEdge)
			{
				m_line[0].position = m_edgeBeggining;
				m_line[1].position = CoreWindow::GetMousePosition() + CoreWindow::GetViewOffset();
				m_line[0].color = sf::Color::White;
				m_line[1].color = m_line[0].color;
				CoreWindow::GetRenderWindow().draw(m_line.data(), m_line.size(), sf::Lines);
			}
			else if (m_removeEdge)
			{
				m_line[0].position = m_edgeBeggining;
				m_line[1].position = CoreWindow::GetMousePosition() + CoreWindow::GetViewOffset();
				m_line[0].color = sf::Color::Red;
				m_line[1].color = m_line[0].color;
				CoreWindow::GetRenderWindow().draw(m_line.data(), m_line.size(), sf::Lines);
			}

			m_texts[EDGE_SUBMODE_TEXT]->Draw();
			m_texts[EDGE_COUNT_TEXT]->Draw();
			break;
		}
		case ActiveMode::VEHICLE:
		{
			m_texts[VEHICLE_SUBMODE_TEXT]->Draw();
			m_texts[VEHICLE_ANGLE_TEXT]->Draw();
			break;
		}
		default:
			break;
	}


	m_texts[ACTIVE_MODE_TEXT]->Draw();
	m_texts[MOVEMENT_TEXT]->Draw();
	m_texts[VIEW_OFFSET_X_TEXT]->Draw();
	m_texts[VIEW_OFFSET_Y_TEXT]->Draw();
	m_texts[FILENAME_TEXT]->Draw();
}
