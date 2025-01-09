/*
 *  @author  Anstro Pleuton (https://github.com/anstropleuton)
 *  @brief   Main game header file.
 *
 *  @copyright  Copyright (C) 2024 Anstro Pleuton
 *
 *  Hypnotizing Double Pendulum simulates thousands of Double Pendulum with
 *  trajectories to create fancy visually pleasing animations.
 *
 *  This software is licensed under the terms of MIT License.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#pragma once

#include <cmath>
#include <vector>
#include <stdexcept>
#include <algorithm>

#include "raylib.h"
#include "raymath.h"

// InputEvent, input event abstraction
struct InputEvent {

	// The event cause type
	enum class What {
		Unknown,

		KeyboardButtonPressed,
		KeyboardButtonReleased,
		KeyboardButtonDown,
		KeyboardButtonUp,
		KeyboardButtonPressedRepeat,

		MouseMoveUp,
		MouseMoveDown,
		MouseMoveLeft,
		MouseMoveRight,

		MouseButtonPressed,
		MouseButtonReleased,
		MouseButtonDown,
		MouseButtonUp,

		MouseWheelMoveUp,
		MouseWheelMoveDown,

		AlwaysHappening,
		NeverHappening
	} what;

	// The event cause source
	int which;

	InputEvent() = default;
	InputEvent(What what, int which = 0) : what(what), which(which) {}

	// Returns true if event is happening
	bool Happened() const
	{
		switch (what)
		{
		case What::KeyboardButtonPressed: return IsKeyPressed(which);
		case What::KeyboardButtonReleased: return IsKeyReleased(which);
		case What::KeyboardButtonDown: return IsKeyDown(which);
		case What::KeyboardButtonUp: return IsKeyUp(which);
		case What::KeyboardButtonPressedRepeat: return IsKeyPressedRepeat(which);
		case What::MouseButtonPressed: return IsMouseButtonPressed(which);
		case What::MouseButtonReleased: return IsMouseButtonReleased(which);
		case What::MouseButtonDown: return IsMouseButtonDown(which);
		case What::MouseButtonUp: return IsMouseButtonUp(which);
		case What::AlwaysHappening: return true;
		case What::NeverHappening: return false;

		case What::MouseMoveUp: return GetMouseDelta().y < 0.0f;
		case What::MouseMoveDown: return GetMouseDelta().y > 0.0f;
		case What::MouseMoveLeft: return GetMouseDelta().x < 0.0f;
		case What::MouseMoveRight: return GetMouseDelta().x > 0.0f;
		case What::MouseWheelMoveUp: return GetMouseWheelMove() > 0.0f;
		case What::MouseWheelMoveDown: return GetMouseWheelMove() < 0.0f;
		default: throw std::runtime_error("Unknown event cause");
		}
	}

	// Returns the value from which event is happening
	float HowMuch() const
	{
		switch (what)
		{
		case What::KeyboardButtonPressed:
		case What::KeyboardButtonReleased:
		case What::KeyboardButtonDown:
		case What::KeyboardButtonUp:
		case What::KeyboardButtonPressedRepeat:
		case What::MouseButtonPressed:
		case What::MouseButtonReleased:
		case What::MouseButtonDown:
		case What::MouseButtonUp:
		case What::AlwaysHappening:
		case What::NeverHappening:
			return Happened() ? 1.0f : 0.0f;

		case What::MouseMoveUp: return -GetMouseDelta().y;
		case What::MouseMoveDown: return GetMouseDelta().y;
		case What::MouseMoveLeft: return -GetMouseDelta().x;
		case What::MouseMoveRight: return GetMouseDelta().x;
		case What::MouseWheelMoveUp: return GetMouseWheelMove();
		case What::MouseWheelMoveDown: return -GetMouseWheelMove();
		default: throw std::runtime_error("Unknown event cause");
		}
	}
};

// InputEvents, combines multiple boolean input events with logical expression
struct InputEvents {

	// Relationship, how multiple event triggers should be considered "Happened"
	enum class Relationship {
		Unknown,
		And,
		Or,
		Xor,
		Nand,
		Nor,
		Xnor
	} relationship = Relationship::And;

	// Accumulation, how to combine the "How much" value of each events
	// Note: Not suitable with use of any relationship that is not And or not Or
	enum class Accumulation {
		Unknown,
		Add,
		Multiply
	} accumulation = Accumulation::Add;

	// All the events
	std::vector<InputEvent> events;

	InputEvents() = default;
	InputEvents(Relationship relationship, Accumulation accumulation, const std::vector<InputEvent>& events) : relationship(relationship), accumulation(accumulation), events(events) {}

	// Check if this collection event has happened, based on relationship
	bool Happened() const
	{
		// No events to evaluate
		if (events.empty())
		{
			return false;
		}

		switch (relationship) {
		case Relationship::And:
			// All events must happen
			return std::all_of(events.begin(), events.end(), [](const InputEvent& event) {
				return event.Happened();
				});

		case Relationship::Or:
			// At least one event must happen
			return std::any_of(events.begin(), events.end(), [](const InputEvent& event) {
				return event.Happened();
				});

		case Relationship::Xor:
			// Exactly one event must happen
			return std::count_if(events.begin(), events.end(), [](const InputEvent& event) {
				return event.Happened();
				}) == 1;

		case Relationship::Nand:
			// Not all events should happen (negation of And)
			return !std::all_of(events.begin(), events.end(), [](const InputEvent& event) {
				return event.Happened();
				});

		case Relationship::Nor:
			// None of the events should happen (negation of Or)
			return std::none_of(events.begin(), events.end(), [](const InputEvent& event) {
				return event.Happened();
				});

		case Relationship::Xnor:
			// All or none of the events should happen
		{
			int count = std::count_if(events.begin(), events.end(), [](const InputEvent& event) {
				return event.Happened();
				});
			return count == 0 || count == events.size();
		}
		default:
			throw std::runtime_error("Unknown relationship");
		}
	}

	// The total of this collection event
	float HowMuch() const
	{
		float value = 0.0f;

		// No events to evaluate
		if (events.empty())
		{
			return value;
		}

		for (auto& event : events)
		{
			// Accumulate value from all events
			switch (accumulation)
			{
			case Accumulation::Add: value += event.HowMuch(); break;
			case Accumulation::Multiply: value *= event.HowMuch(); break;
			default: throw std::runtime_error("Unknown accumulation");
			}
		}

		return value;
	}
};

// FreeCamera2D, allows you to freely look around in the 2D world
struct FreeCamera2D {

	// Main camera
	Camera2D camera;

	// Zoom is externally controlled
	// camera.zoom is actually 2 ^ cameraZoom
	float cameraZoom;

	// Camera Config
	float zoomSpeed;
	float panSpeed;

	// Camera controls
	InputEvents panLeft;
	InputEvents panRight;
	InputEvents panUp;
	InputEvents panDown;

	InputEvents zoomIn;
	InputEvents zoomOut;

	FreeCamera2D()
	{
		camera = Camera2D{};
		cameraZoom = 0.0f;

		zoomSpeed = 0.1f;
		panSpeed = 1.0f;

		panLeft.events.emplace_back(InputEvent(InputEvent::What::MouseMoveLeft));
		panRight.events.emplace_back(InputEvent(InputEvent::What::MouseMoveRight));
		panUp.events.emplace_back(InputEvent(InputEvent::What::MouseMoveUp));
		panDown.events.emplace_back(InputEvent(InputEvent::What::MouseMoveDown));

		// Must also hold middle mouse button
		panLeft.events.emplace_back(InputEvent(InputEvent::What::MouseButtonDown, MOUSE_BUTTON_MIDDLE));
		panRight.events.emplace_back(InputEvent(InputEvent::What::MouseButtonDown, MOUSE_BUTTON_MIDDLE));
		panUp.events.emplace_back(InputEvent(InputEvent::What::MouseButtonDown, MOUSE_BUTTON_MIDDLE));
		panDown.events.emplace_back(InputEvent(InputEvent::What::MouseButtonDown, MOUSE_BUTTON_MIDDLE));

		zoomIn.events.emplace_back(InputEvent(InputEvent::What::MouseWheelMoveUp));
		zoomOut.events.emplace_back(InputEvent(InputEvent::What::MouseWheelMoveDown));
	}

	// Update camera using events
	void Update()
	{
		camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };

		float oldZoom = camera.zoom;
		Vector2 mousePosition = Vector2Subtract(GetMousePosition(), camera.offset);

		if (zoomIn.Happened())
		{
			cameraZoom += zoomIn.HowMuch() * zoomSpeed;
		}
		if (zoomOut.Happened())
		{
			cameraZoom -= zoomOut.HowMuch() * zoomSpeed;
		}

		camera.zoom = std::pow(2.0f, cameraZoom);

		if (zoomIn.Happened() || zoomOut.Happened())
		{
			camera.target.x += mousePosition.x / oldZoom;
			camera.target.x -= mousePosition.x / camera.zoom;
			camera.target.y += mousePosition.y / oldZoom;
			camera.target.y -= mousePosition.y / camera.zoom;
		}

		if (panUp.Happened())
		{
			camera.target.y += panUp.HowMuch() * panSpeed / camera.zoom;
		}
		if (panDown.Happened())
		{
			camera.target.y -= panDown.HowMuch() * panSpeed / camera.zoom;
		}
		if (panLeft.Happened())
		{
			camera.target.x += panLeft.HowMuch() * panSpeed / camera.zoom;
		}
		if (panRight.Happened())
		{
			camera.target.x -= panRight.HowMuch() * panSpeed / camera.zoom;
		}
	}

	// Shorthand for BeginMode2D(this->camera)
	void BeginMode2D() const
	{
		::BeginMode2D(camera);
	}

	// Shorthand for EndMode2D(this->camera)
	void EndMode2D() const
	{
		::EndMode2D();
	}

	// Returns this->camera
	operator Camera2D() const
	{
		return camera;
	}
};
