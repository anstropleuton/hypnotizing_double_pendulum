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

#include "game.hpp"

#include <cmath>

 // Vector2Double, 2 double precision component vector
struct Vector2Double {
	double x;
	double y;

	Vector2Double() = default;
	Vector2Double(double x, double y) : x(x), y(y) {}

	// Convert to Vector2
	operator Vector2() const
	{
		return Vector2{ (float)x, (float)y };
	}
};

// Settings, all the configuration
struct SimulationSettings {

	// Simulation settings
	double gravity;
	double fixedDeltaTime;
	double trajectoryAlphaPower;

	// Requires simulation reset for these
	std::size_t pendulumsJoined;
	std::size_t joinedPendulumsCount;
	std::size_t trajectoryPoints;

	// Pendulum config (also needs reset)
	double pendulumLength;
	double pendulumMass;

	// Pendulum color settings
	float pendulumColorSaturation;
	float pendulumColorValue;

	// Reset when pendulums diverged (average distance) more than threshold
	double resetThreshold;
	std::size_t resetSamples;
	double resetFadeTime;

	SimulationSettings()
	{
		gravity = 0.981;
		fixedDeltaTime = 0.1666667;
		trajectoryAlphaPower = 2.5;

		pendulumsJoined = 2;
		joinedPendulumsCount = 1000;
		trajectoryPoints = 50;

		pendulumLength = 150.0;
		pendulumMass = 10.0;

		pendulumColorSaturation = 0.5f;
		pendulumColorValue = 1.0f;

		resetThreshold = 10.0;
		resetSamples = 100;
		resetFadeTime = 2.5;
	}

	// Load settings from file, return true if simulation needs reset
	bool LoadSettings(std::string_view filename);

	// Create a file with current settings
	void SaveSettings(std::string_view filename)
	{
		auto data = R"(
; Simulation settings
gravity %f
fixedDeltaTime %f
trajectoryAlphaPower %f

; Requires simulation reset for these
pendulumsJoined %zu
joinedPendulumsCount %zu
trajectoryPoints %zu

; Pendulum config (also needs reset)
pendulumLength %f
pendulumMass %f

; Pendulum color settings
pendulumColorSaturation %f
pendulumColorValue %f

; Reset when pendulums diverged (average distance) more than threshold
resetThreshold %f
resetSamples %zu
resetFadeTime %zu
		)";

		auto formatted = TextFormat(data,
			gravity,
			fixedDeltaTime,
			trajectoryAlphaPower,
			pendulumsJoined,
			joinedPendulumsCount,
			trajectoryPoints,
			pendulumLength,
			pendulumMass,
			pendulumColorSaturation,
			pendulumColorValue,
			resetThreshold,
			resetSamples,
			resetFadeTime
		);

		// Ray, why does it not take const char* instead of char* ?
		SaveFileText(std::string(filename).c_str(), (char*)formatted);
	}
};

// Global settings
extern SimulationSettings settings;

// Pendulum, a single connectable prendulum
struct Pendulum {

	Vector2Double position; // End position
	Vector2Double velocity; // End velocity

	double length;
	double mass;

	double angle;
	double angularVelocity;
	double angularAcceleration;

	// Apparently `Pendulum() = default;` is not enough for VS 2022
	Pendulum() : position(0.0, 0.0), velocity(0.0, 0.0), length(0.0), mass(0.0),
		angle(0.0), angularVelocity(0.0), angularAcceleration(0.0)
	{
	}

	// Construct from length, mass, initial angle and color
	Pendulum(double length, double mass, double initialAngle)
		: position(0.0, 0.0), velocity(0.0, 0.0), length(length), mass(mass),
		angle(initialAngle), angularVelocity(0.0), angularAcceleration(0.0)
	{
	}
};

// JoinedPendulum, multi-pendulum simulation
struct JoinedPendulum {
	std::vector<Pendulum> pendulums;

	// Trajectory
	std::vector<Vector2Double> trajectories;
	std::size_t trajectoryIndex;

	JoinedPendulum() : trajectories(), trajectoryIndex(0)
	{
	}

	// Construct from size, lengths, masses, initial angles and colors
	JoinedPendulum(
		std::size_t size,
		std::vector<double> lengths,
		std::vector<double> masses,
		std::vector<double> initialAngles,
		std::size_t trajectoriesSize
	);

	// Update pendulums
	void Update();

	// Draw last pendulum trajectories
	void DrawTrajectory(Color color) const;

	// Draw all pendulums (lines)
	void DrawPendulums(Color color) const;
};

// Simulation pendulums
extern std::vector<JoinedPendulum> pendulums;

// Initialize pendulums
void InitializePendulums(int resets = 0);

// Update pendulums
void UpdatePendulums();

// Draw pendulum trajectories
void DrawPendulumTrajectories(float alpha = 1.0f, bool debug = false);

// Get divergence (average distance for samples)
double GetDivergence();
