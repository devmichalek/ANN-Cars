#pragma once
#include <math.h>
#include <array>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

using CarPoints = std::array<sf::Vector2i, 4>;

class DCar
{
	float m_angle;
	float m_speed;
	sf::Vector2f m_size;
	sf::Vector2f m_center;
	sf::ConvexShape m_convexShape;
	sf::CircleShape m_circleShape;

public:

	DCar(sf::Vector2f size, sf::Vector2f position = sf::Vector2f(0, 0)) : m_size(size), m_center(position)
	{
		m_angle = 0;
		m_speed = 0;
		m_convexShape.setPointCount(4);
		m_circleShape.setRadius(m_size.x / 20);
		m_circleShape.setFillColor(sf::Color::Red);
		update();
	}

	virtual ~DCar()
	{
	}

	// Sets color
	inline void setColor(const sf::Color color)
	{
		m_convexShape.setFillColor(color);
	}

	// Rotate car by specified value (-1; 1)
	void rotate(float rotationRatio)
	{
		m_angle += rotationRatio * 0.072f;
	}

	// Accelerate by specified value (0; 1)
	void accelerate(float value);

	// Brake by specified value (0; 1)
	void brake(float value);
	
	// Update car rotation and position
	void update();

	// Draws car
	void draw();

	// Returns car described in four points
	inline CarPoints getPoints()
	{
		return {
			static_cast<sf::Vector2i>(m_convexShape.getPoint(0)),
			static_cast<sf::Vector2i>(m_convexShape.getPoint(1)),
			static_cast<sf::Vector2i>(m_convexShape.getPoint(2)),
			static_cast<sf::Vector2i>(m_convexShape.getPoint(3))
		};
	}
};