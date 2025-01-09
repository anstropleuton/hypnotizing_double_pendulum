/*
 *  @author  Anstro Pleuton (https://github.com/anstropleuton)
 *  @brief   Main game source file.
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

#include "raylib.h"
#include "raymath.h"

#include "game.hpp"
#include "pendulum.hpp"

#define SETTINGS_FILENAME "settings.txt"
#define MUSIC_FILENAME "music.mp3"

static FreeCamera2D camera;         // Main camera
static bool showInfo = true;        // Show usage information
static bool showPendulums = false;  // Show pendulum itself (not just trajectories)
static bool paused = true;          // Simulation paused
static double divergence;           // Divergence (average distance for samples)
static int resets = 0;              // Number of times it has been reset
static double initiatedReset = 0.0; // Initiated reset at time
static int settingsModTime;         // File modification time for settings file
static double toastMessageTimer = 0.0; // Timer for a toast message
static std::string toastMessage;    // Toast message shown at bottom right
static Music music;                 // Background music
static bool muted = false;          // Mute background music

// Initialize everything
static void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1000, 800, "Hypnotizing Double Pendulum");
    InitAudioDevice();
    SetTargetFPS(60);

    // Save default settings if file does not exist (creates new file)
    if (!FileExists(SETTINGS_FILENAME))
    {
        settings.SaveSettings(SETTINGS_FILENAME);
    }

    settings.LoadSettings(SETTINGS_FILENAME);
    settingsModTime = GetFileModTime(SETTINGS_FILENAME);
    music = LoadMusicStream(MUSIC_FILENAME);
    PlayMusicStream(music);

    InitializePendulums();
}

// Close everything
static void GameCleanup()
{
    UnloadMusicStream(music);
    CloseWindow();
}

// Update everything
static bool GameUpdate()
{
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressedRepeat(KEY_SPACE))
    {
        paused = !paused;
    }

    if (IsKeyPressed(KEY_F1) || IsKeyPressedRepeat(KEY_F1))
    {
        showInfo = !showInfo;
    }

    if (IsKeyPressed(KEY_F3) || IsKeyPressedRepeat(KEY_F3))
    {
        showPendulums = !showPendulums;
    }

    if (IsKeyPressed(KEY_M) || IsKeyPressedRepeat(KEY_M))
    {
        muted = !muted;
        SetMusicVolume(music, muted ? 0.0f : 1.0f);
    }

    if (IsKeyPressed(KEY_F11) || IsKeyPressedRepeat(KEY_F11))
    {
        ToggleBorderlessWindowed();
    }

    // Reload settings
    int newModTime = GetFileModTime(SETTINGS_FILENAME);
    if (settingsModTime != newModTime)
    {
        settingsModTime = newModTime;

        // Reset simulation if required
        if (settings.LoadSettings(SETTINGS_FILENAME))
        {
            resets = 0;
            InitializePendulums();
            toastMessageTimer = GetTime() + 5;
            toastMessage = "Reloaded file " SETTINGS_FILENAME " and reset simulation";
        }
        else
        {
            toastMessageTimer = GetTime() + 5;
            toastMessage = "Reloaded file " SETTINGS_FILENAME;
        }
    }

    // Open settings
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_O))
    {
        toastMessageTimer = GetTime() + 5;
        toastMessage = "Opened file " SETTINGS_FILENAME " in system text editor";
#ifdef _WIN32
        std::system("start " SETTINGS_FILENAME);
#elif __linux__
        std::system("xdg-open " SETTINGS_FILENAME);
#elif __APPLE__
        std::system("open " SETTINGS_FILENAME); // Untested
#else
        toastMessageTimer = GetTime() + 5;
        toastMessage = "Unsupported OS, please manually edit " SETTINGS_FILENAME " from current working directory";
#endif
    }

    // Save settings
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_S))
    {
        settings.SaveSettings(SETTINGS_FILENAME);
        toastMessageTimer = GetTime() + 5;
        toastMessage = "Saved file " SETTINGS_FILENAME " in current working directory";
    }

    // Reset after divergence
    divergence = GetDivergence();
    if (initiatedReset != 0.0)
    {
        if (GetTime() >= initiatedReset)
        {
            resets++;
            InitializePendulums(resets);
            initiatedReset = 0.0;
        }
    }

    // Reset calculation
    else
    {
        bool resetKey = IsKeyPressed(KEY_R) || IsKeyPressedRepeat(KEY_R);
        bool continueKey = IsKeyDown(KEY_C);
        bool diverged = divergence > settings.resetThreshold;
        if (!continueKey && (resetKey || diverged))
        {
            initiatedReset = GetTime() + settings.resetFadeTime;
        }
    }

    camera.Update();
    if (!paused)
    {
        UpdateMusicStream(music);
        UpdatePendulums();
    }

    return true;
}

// Draw everything
static void GameDraw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    camera.BeginMode2D();

    // Fade animation
    float alpha = initiatedReset == 0.0 ? 1.0f : (initiatedReset - GetTime()) / settings.resetFadeTime;
    DrawPendulumTrajectories(alpha, showPendulums);

    camera.EndMode2D();

    if (showInfo)
    {
        DrawText(
            "Press SPACE to resume/pause simulation\n"
            "Press F1 to toggle this info\n"
            "Press F3 to show pendulum itself\n"
            "Press F11 to toggle fullscreen\n"
            "\n",
            20, 20, 20, WHITE
        );
        DrawText(
            TextFormat(
                "\n\n\n"
                "\n"
                "FPS: %d\n"
                "Resets count: %d\n"
                "Divergence / Threshold to reset: %f / %f\n"
                "Press R to manually reset, or hold C to not auto reset\n"
                "\n"
                "Settings:\n"
                "  Gravity: %f\n"
                "  Fixed delta time: %f\n"
                "  Trajectory alpha power: %f\n"
                "  Pendulums joined = %zu\n"
                "  Joined pendulums count = %zu\n"
                "  Trajectory points = %zu\n"
                "  Pendulum length = %f\n"
                "  Pendulum mass = %f\n"
                "  Pendulum color saturation = %f\n"
                "  Pendulum color value = %f\n"
                "  Reset threshold = %f\n"
                "  Reset samples = %zu\n"
                "  Reset fade time = %f\n"
                "\n"
                "Edit configuration from " SETTINGS_FILENAME "\n"
                "Press CTRL+O to open settings file\n"
                "And CTRL+S to save current settings\n"
                "\n"
                "Music: " MUSIC_FILENAME "\n"
                "Replace the file and restart the app to change music\n"
                "Press M to toggle mute\n"
                "\n",
                GetFPS(),
                resets,
                divergence, settings.resetThreshold,

                settings.gravity,
                settings.fixedDeltaTime,
                settings.trajectoryAlphaPower,
                settings.pendulumsJoined,
                settings.joinedPendulumsCount,
                settings.trajectoryPoints,
                settings.pendulumLength,
                settings.pendulumMass,
                settings.pendulumColorSaturation,
                settings.pendulumColorValue,
                settings.resetThreshold,
                settings.resetSamples,
                settings.resetFadeTime
            ),
            20, 20, 20, GRAY
        );
    }

    if (toastMessageTimer >= GetTime())
    {
        int width = MeasureText(toastMessage.c_str(), 20);
        DrawText(toastMessage.c_str(), GetScreenWidth() - width - 20, GetScreenHeight() - 40, 20, WHITE);
    }

    EndDrawing();
}

// Do everything
int main()
{
    GameInit();

    while (!WindowShouldClose())
    {
        if (!GameUpdate())
            break;

        GameDraw();
    }
    GameCleanup();

    return 0;
}
