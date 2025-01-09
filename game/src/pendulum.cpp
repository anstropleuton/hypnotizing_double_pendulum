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

#include "pendulum.hpp"

#include <ranges>
#include <string>
#include <regex>

SimulationSettings settings;
std::vector<JoinedPendulum> pendulums;

namespace stdr
{
    using namespace std::ranges;
    using namespace views;

    // Me hungry for concise syntax
    template<typename... Args>
    constexpr auto to_vs(Args&&... args)
    {
        return to<std::vector<std::string>>(std::forward<Args>(args)...);
    }
}

bool SimulationSettings::LoadSettings(std::string_view filename)
{
    auto tmp = LoadFileText(std::string(filename).c_str());
    auto text = std::string(tmp);
    UnloadFileText(tmp);

    bool needsReset = false;

    // Very basic parser
    auto lines = text | stdr::split('\n') | stdr::to_vs();
    for (std::size_t i = 0; i < lines.size(); i++)
    {
        // Remove extra whitespaces
        auto tmp0 = lines[i];
        auto tmp1 = std::regex_replace(tmp0, std::regex("\t"), " ");
        auto tmp2 = std::regex_replace(tmp1, std::regex("  +"), " ");
        auto tmp3 = std::regex_replace(tmp2, std::regex("^ +(.*?) +$"), "$1");
        auto line = tmp3;

        auto tokens = line | stdr::split(' ') | stdr::to_vs();
        if (tokens.empty()) continue;

        // Remove comments
        for (std::size_t j = 0; j < tokens.size(); j++)
        {
            auto& token = tokens[j];
            if (token == ";")
            {
                tokens = tokens | stdr::take(j) | stdr::to_vs();
                break;
            }
        }

        if (tokens.empty()) continue;

        if (tokens.size() != 2)
        {
            TraceLog(LOG_WARNING, TextFormat("Invalid settings line #%d: %s", i + 1, line.c_str()));
            continue;
        }

        try
        {
            // Dirty...
            if (tokens[0] == "gravity")
            {
                auto newGravity = std::stod(tokens[1]);
                if (gravity != newGravity)
                {
                    gravity = newGravity;
                }
            }
            else if (tokens[0] == "fixedDeltaTime")
            {
                auto newFixedDeltaTime = std::stod(tokens[1]);
                if (fixedDeltaTime != newFixedDeltaTime)
                {
                    fixedDeltaTime = newFixedDeltaTime;
                }
            }
            else if (tokens[0] == "trajectoryAlphaPower")
            {
                auto newTrajectoryAlphaPower = std::stod(tokens[1]);
                if (trajectoryAlphaPower != newTrajectoryAlphaPower)
                {
                    trajectoryAlphaPower = newTrajectoryAlphaPower;
                }
            }
            else if (tokens[0] == "pendulumsJoined")
            {
                auto newPendulumsJoined = std::stoul(tokens[1]);
                if (pendulumsJoined != newPendulumsJoined)
                {
                    pendulumsJoined = newPendulumsJoined;
                    needsReset = true;
                }
            }
            else if (tokens[0] == "joinedPendulumsCount")
            {
                auto newJoinedPendulumsCount = std::stoul(tokens[1]);
                if (joinedPendulumsCount != newJoinedPendulumsCount)
                {
                    joinedPendulumsCount = newJoinedPendulumsCount;
                    needsReset = true;
                }
            }
            else if (tokens[0] == "trajectoryPoints")
            {
                auto newTrajectoryPoints = std::stoul(tokens[1]);
                if (trajectoryPoints != newTrajectoryPoints)
                {
                    trajectoryPoints = newTrajectoryPoints;
                    needsReset = true;
                }
            }
            else if (tokens[0] == "pendulumLength")
            {
                auto newPendulumLength = std::stod(tokens[1]);
                if (pendulumLength != newPendulumLength)
                {
                    pendulumLength = newPendulumLength;
                    needsReset = true;
                }
            }
            else if (tokens[0] == "pendulumMass")
            {
                auto newPendulumMass = std::stod(tokens[1]);
                if (pendulumMass != newPendulumMass)
                {
                    pendulumMass = newPendulumMass;
                    needsReset = true;
                }
            }
            else if (tokens[0] == "resetThreshold")
            {
                auto newResetThreshold = std::stod(tokens[1]);
                if (resetThreshold != newResetThreshold)
                {
                    resetThreshold = newResetThreshold;
                }
            }
            else if (tokens[0] == "resetSamples")
            {
                auto newResetSamples = std::stod(tokens[1]);
                if (resetSamples != newResetSamples)
                {
                    resetSamples = newResetSamples;
                }
            }
            else if (tokens[0] == "resetFadeTime")
            {
                auto newResetFadeTime = std::stod(tokens[1]);
                if (resetFadeTime != newResetFadeTime)
                {
                    resetFadeTime = newResetFadeTime;
                }
            }
        }

        // Probably std::invalid_argument
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, TextFormat("Exception while loading settings line #%d: %s", i + 1, e.what()));
        }
    }

    return needsReset;
}


inline JoinedPendulum::JoinedPendulum(std::size_t size, std::vector<double> lengths, std::vector<double> masses, std::vector<double> initialAngles, std::size_t trajectoriesSize) : trajectories(), trajectoryIndex(0)
{
    // Check invalid sizes
    if (lengths.size() != size)
    {
        throw std::runtime_error("Invalid size provided for lengths");
    }
    if (masses.size() != size)
    {
        throw std::runtime_error("Invalid size provided for masses");
    }
    if (initialAngles.size() != size)
    {
        throw std::runtime_error("Invalid size provided for initialAngles");
    }

    // Initialize all pendulums
    pendulums.resize(size);
    for (std::size_t i = 0; i < pendulums.size(); i++)
    {
        pendulums[i] = Pendulum(lengths[i], masses[i], initialAngles[i]);
    }

    // Calculate position of each pendulum, as in "connected"
    for (std::size_t i = 1; i < pendulums.size(); i++)
    {
        Vector2Double pos_1 = pendulums[i - 1].position;
        double length = pendulums[i].length;
        double angle = pendulums[i].angle;

        pendulums[i].position = Vector2Double(
            pos_1.x + length * sin(angle),
            pos_1.y + length * cos(angle)
        );
    }

    trajectories.resize(trajectoriesSize);
}

void JoinedPendulum::Update()
{
    const auto n = pendulums.size();

    if (n == 0)
    {
        return;
    }

    // Single pendulum case
    if (n == 1)
    {
        auto& p = pendulums[0];

        // Update angle
        p.angularAcceleration = -settings.gravity / p.length * sin(p.angle);
        p.angularVelocity = p.angularAcceleration * settings.fixedDeltaTime;
        p.angle += p.angularVelocity * settings.fixedDeltaTime;

        // Update position
        p.position = Vector2Double(
            p.length * sin(p.angle),
            p.length * cos(p.angle)
        );

        return;
    }

    // Update angular acceleration
    for (std::size_t i = 0; i < n - 1; i++)
    {
        auto& p1 = pendulums[i];
        auto& p2 = pendulums[i + 1];

        // Who said not to use single to three letter names?
        double a1 = p1.angle;
        double a2 = p2.angle;
        double m1 = p1.mass;
        double m2 = p2.mass;
        double l1 = p1.length;
        double l2 = p2.length;

        // Sin and cos of angles
        double sa1 = sin(a1);
        double ca1 = cos(a1);
        double sa2 = sin(a2);
        double ca2 = cos(a2);

        // Gravity
        double g = settings.gravity;

        // What the fuck?
        double n1 = -g * (2.0 * m1 + m2) * sin(a1);
        double n2 = -m2 * g * sin(a1 - 2.0 * a2);
        double n3 = -2.0 * sin(a1 - a2) * m2;
        double n4 = p2.angularVelocity * p2.angularVelocity * l2
            + p1.angularVelocity * p1.angularVelocity * l1 * cos(a1 - a2);
        double d = l1 * (2.0 * m1 + m2 - m2 * cos(2.0 * a1 - 2.0 * a2));
        p1.angularAcceleration = (n1 + n2 + n3 * n4) / d;

        // Again?
        n1 = 2.0 * sin(a1 - a2);
        n2 = p1.angularVelocity * p1.angularVelocity * l1 * (m1 + m2);
        n3 = g * (m1 + m2) * cos(a1);
        n4 = p2.angularVelocity * p2.angularVelocity * l2 * m2 * cos(a1 - a2);
        d = l2 * (2.0 * m1 + m2 - m2 * cos(2.0 * a1 - 2.0 * a2));
        p2.angularAcceleration = (n1 * (n2 + n3 + n4)) / d;
    }

    // Update angle and position
    for (std::size_t i = 0; i < n; i++)
    {
        auto& p = pendulums[i];

        p.angularVelocity += p.angularAcceleration * settings.fixedDeltaTime;
        p.angle += p.angularVelocity * settings.fixedDeltaTime;

        // First pendulum anchored at center
        if (i == 0)
        {
            p.position = Vector2Double(
                p.length * sin(p.angle),
                p.length * cos(p.angle)
            );
        }
        else
        {
            // Previous pendulum
            auto& pp = pendulums[i - 1];
            p.position = Vector2Double(
                pp.position.x + p.length * sin(p.angle),
                pp.position.y + p.length * cos(p.angle)
            );
        }
    }

    // Capture last pendulum position as trajectory
    trajectories[trajectoryIndex] = pendulums[n - 1].position;
    trajectoryIndex = (trajectoryIndex + 1) % trajectories.size();
}

void JoinedPendulum::DrawTrajectory(Color color) const
{
    if (trajectories.empty())
    {
        return;
    }

    for (std::size_t i = 0; i < trajectories.size() - 1; i++)
    {
        auto& current = trajectories[(trajectoryIndex + i) % trajectories.size()];
        auto& next = trajectories[(trajectoryIndex + i + 1) % trajectories.size()];

        if (current.x == 0.0 || current.y == 0.0 ||
            next.x == 0.0 || next.y == 0.0)
        {
            continue;
        }

        double alpha = std::pow((double)(i + 1) / trajectories.size(), settings.trajectoryAlphaPower);
        if (alpha > 1.0) alpha = 1.0;
        if (alpha < 0.0) alpha = 0.0;
        Color fadedColor = color;
        fadedColor.a *= alpha;
        DrawLineV(current, next, fadedColor);
    }
}

void JoinedPendulum::DrawPendulums(Color color) const
{
    for (std::size_t i = 0; i < pendulums.size(); i++)
    {
        Vector2Double prev = Vector2Double();
        if (i != 0)
        {
            prev = pendulums[i - 1].position;
        }

        DrawLineV(prev, pendulums[i].position, color);
    }
}

void InitializePendulums(int resets)
{
    pendulums.clear();

    pendulums.resize(settings.joinedPendulumsCount);
    for (std::size_t i = 0; i < pendulums.size(); i++)
    {
        std::vector lengths(settings.pendulumsJoined, settings.pendulumLength);
        std::vector masses(settings.pendulumsJoined, settings.pendulumMass);
        std::vector initialAngles(settings.pendulumsJoined, (double)PI);
        initialAngles[0] = PI + 0.125 + (double)i / pendulums.size() * 0.0001;
        initialAngles[0] += std::fmod(resets * 0.5 + PI / 8.0, (double)PI / 4) - PI / 8.0;
        pendulums[i] = JoinedPendulum(settings.pendulumsJoined, lengths, masses, initialAngles, settings.trajectoryPoints);
    }
}

void UpdatePendulums()
{
    for (auto& p : pendulums)
    {
        p.Update();
    }
}

void DrawPendulumTrajectories(float alpha, bool debug)
{
    if (debug)
    {
        for (std::size_t i = 0; i < pendulums.size(); i++)
        {
            Color color = ColorFromHSV(i * 360.0f / pendulums.size() + GetTime() * 5.0f, settings.pendulumColorSaturation, settings.pendulumColorValue);
            if (alpha > 1.0) alpha = 1.0;
            if (alpha < 0.0) alpha = 0.0;
            color.a = (unsigned char)(alpha * 255);
            Color debugColor = color;
            debugColor.r *= 0.75f;
            debugColor.g *= 0.75f;
            debugColor.b *= 0.75f;
            pendulums[i].DrawPendulums(debugColor);
        }
    }
    for (std::size_t i = 0; i < pendulums.size(); i++)
    {
        Color color = ColorFromHSV(i * 360.0f / pendulums.size() + GetTime() * 5.0f, settings.pendulumColorSaturation, settings.pendulumColorValue);
        if (alpha > 1.0) alpha = 1.0;
        if (alpha < 0.0) alpha = 0.0;
        color.a = (unsigned char)(alpha * 255);
        pendulums[i].DrawTrajectory(color);
    }
}

double GetDivergence()
{
    if (pendulums.empty()) return 0.0;

    double divergence = 0.0;
    for (std::size_t s = 0; s < settings.resetSamples; s++)
    {
        std::size_t i = std::floor((float)s * pendulums.size() / (settings.resetSamples + 1));

        auto p1 = pendulums[i].pendulums.back().position;
        auto p2 = pendulums[i + 1].pendulums.back().position;

        Vector2Double l = Vector2Double(p1.x - p2.x, p1.y - p2.y);
        divergence += std::sqrt(l.x * l.x + l.y * l.y);
    }

    return divergence / settings.resetSamples;
}
