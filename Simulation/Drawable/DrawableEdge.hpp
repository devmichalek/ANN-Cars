#pragma once
#include "DrawableInterface.hpp"
#include "DrawableMath.hpp"
#include "CoreWindow.hpp"

class DrawableEdge :
	public DrawableInterface
{
	EdgeShape m_edgeShape;

protected:

	DrawableEdge()
	{
		m_edgeShape[0].color = sf::Color::White;
		m_edgeShape[1].color = m_edgeShape[0].color;
	}

public:

	// Draws edge based on provided fixture
	void Draw(b2Fixture* fixture)
	{
		CoreWindow::GetRenderWindow().draw(m_edgeShape.data(), m_edgeShape.size(), sf::Lines);
	}

	// Sets edges position
	inline void SetPosition(Edge position)
	{
		m_edgeShape[0].position = position[0];
		m_edgeShape[1].position = position[1];
	}
};