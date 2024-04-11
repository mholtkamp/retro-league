#include <stdint.h>

#undef min
#undef max

#include "Engine.h"
#include "World.h"
#include "Renderer.h"
#include "InputDevices.h"
#include "Log.h"
#include "AssetManager.h"

#include "Nodes/3D/TestSpinner.h"

#include "Nodes/Widgets/StatsOverlay.h"

#include "GameState.h"

#define EMBEDDED_ENABLED (PLATFORM_DOLPHIN || PLATFORM_3DS)

#if EMBEDDED_ENABLED
#include "../Generated/EmbeddedAssets.h"
#include "../Generated/EmbeddedScripts.h"
#endif

InitOptions OctPreInitialize()
{
    InitOptions initOptions;
    initOptions.mWidth = 1280;
    initOptions.mHeight = 720;
    initOptions.mProjectName = "Rocket";
    initOptions.mUseAssetRegistry = false;
    initOptions.mVersion = 1;

#if PLATFORM_DOLPHIN || PLATFORM_3DS
    initOptions.mEmbeddedAssetCount = gNumEmbeddedAssets;
    initOptions.mEmbeddedAssets = gEmbeddedAssets;
    initOptions.mEmbeddedScriptCount = gNumEmbeddedScripts;
    initOptions.mEmbeddedScripts = gEmbeddedScripts;
#endif

#if PLATFORM_WII
    initOptions.mWorkingDirectory = "sd://apps/RetroLeagueSD";
#endif

    return initOptions;
}

void OctPostInitialize()
{
#if 0

    GetWorld()->SpawnDefaultRoot();
    GetWorld()->SpawnDefaultCamera();
    GetWorld()->SpawnNode<TestSpinner>();

#else
    GetWorld(0)->SetAmbientLightColor({ 0.5f, 0.5f, 0.5f, 1.0f });
    GetWorld(0)->SetShadowColor({ 0.0f, 0.0f, 0.0f, 0.5f });

    GetGameState()->Initialize();
    
#if !EDITOR
    //GetGameState()->LoadPreferredMatchOptions();
    GetGameState()->LoadMainMenu();
#endif

#if PLATFORM_3DS
    // On old 3DS, our perf isn't good enough for 60 fps, so limit to 30 fps to avoid stutters.
    if (!GetEngineState()->mSystem.mNew3DS)
    {
        GFX_SetFrameRate(30.0f);
    }
#endif

    Renderer::Get()->SetGlobalUiScale(1.0f);
#endif
}

void OctPreUpdate()
{

}

void OctPostUpdate()
{
#if 1
    GetGameState()->Update(GetAppClock()->DeltaTime());

#if !EDITOR
    // Exit the game if home is pressed or the Smash Bros Melee reset combo is pressed.
    if (IsGamepadButtonDown(GAMEPAD_HOME, 0) ||
        (IsGamepadButtonDown(GAMEPAD_A, 0) &&
            IsGamepadButtonDown(GAMEPAD_L1, 0) &&
            IsGamepadButtonDown(GAMEPAD_R1, 0) &&
            IsGamepadButtonDown(GAMEPAD_START, 0)))
    {
        exit(0);
    }

    if ((IsGamepadButtonJustDown(GAMEPAD_START, 0) || IsKeyJustDown(KEY_ENTER)) &&
        !GetGameState()->mMainMenuOpen)
    {
        // Exit game, go back to menu
        GetGameState()->mTransitionToMainMenu = true;
    }


    if (IsGamepadButtonDown(GAMEPAD_Z, 0) ||
        IsGamepadButtonDown(GAMEPAD_SELECT, 0))
    {
        StatsOverlay* stats = Renderer::Get()->GetStatsWidget();
        if (IsGamepadButtonJustDown(GAMEPAD_DOWN, 0))
        {
            stats->SetDisplayMode(StatDisplayMode::None);
        }
        else if (IsGamepadButtonJustDown(GAMEPAD_LEFT, 0))
        {
            stats->SetDisplayMode(StatDisplayMode::Network);
        }
        else if (IsGamepadButtonJustDown(GAMEPAD_UP, 0))
        {
            stats->SetDisplayMode(StatDisplayMode::CpuStatText);
        }
        else if (IsGamepadButtonJustDown(GAMEPAD_RIGHT, 0))
        {
            stats->SetDisplayMode(StatDisplayMode::Memory);
        }
    }
#endif

#endif
}

void OctPreShutdown()
{
    GetGameState()->Shutdown();
}

void OctPostShutdown()
{
    
}
