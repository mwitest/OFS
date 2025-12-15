#pragma once
#include "OFS_Event.h"
#include "SDL_gamecontroller.h"
#include "SDL_haptic.h"
#include <array>

class ControllerInput {
private:
    SDL_GameController* gamepad = nullptr;
    SDL_Joystick* joystick = nullptr;
    SDL_Haptic* haptic = nullptr;
    SDL_JoystickID instance_id = -1;
    bool isConnected = false;
    bool isVirtualGamepad = false;
    int physicalDeviceIndex = -1;

    void OpenController(int device) noexcept;
    void OpenJoystickAsGameController(int device) noexcept;
    void CloseController() noexcept;

    static int32_t activeControllers;
    static int GetControllerIndex(SDL_JoystickID instance) noexcept;

    void ControllerButtonDown(const OFS_SDL_Event* ev) const noexcept;
    void ControllerButtonUp(const OFS_SDL_Event* ev) const noexcept;
    void ControllerDeviceAdded(const OFS_SDL_Event* ev) noexcept;
    void ControllerDeviceRemoved(const OFS_SDL_Event* ev) noexcept;
    
    // Funciones para eventos joystick - NO CONST
    void JoystickButtonDown(const OFS_SDL_Event* ev) noexcept;
    void JoystickButtonUp(const OFS_SDL_Event* ev) noexcept;
    void JoystickAxisMotion(const OFS_SDL_Event* ev) noexcept;
    void JoystickHatMotion(const OFS_SDL_Event* ev) noexcept;
    void JoystickDeviceAdded(const OFS_SDL_Event* ev) noexcept;

public:
    static std::array<ControllerInput, 4> Controllers;

    void Init() noexcept;
    void Update() noexcept;
    void Shutdown() noexcept;

    static void UpdateControllers() noexcept;

    inline const char* GetName() const noexcept { 
        return "Xbox 360 Controller";
    }
    
    inline bool Connected() const noexcept { return isConnected; }
    static inline bool AnythingConnected() noexcept { return activeControllers > 0; }
};

extern std::array<int64_t, SDL_CONTROLLER_BUTTON_MAX> ButtonsHeldDown;