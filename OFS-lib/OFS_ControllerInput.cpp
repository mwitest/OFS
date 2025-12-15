#include "OFS_ControllerInput.h"
#include "OFS_Util.h"
#include "OFS_Profiling.h"
#include "OFS_EventSystem.h"

#include "SDL_gamecontroller.h"
#include "SDL_joystick.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <string>

// Variables globales
std::array<int64_t, SDL_CONTROLLER_BUTTON_MAX> ButtonsHeldDown = { -1 };
std::unordered_map<std::string, int> g_joystickConfig;

// Función para obtener path del ejecutable - PONLA PRIMERO
std::string GetExePath() {
    char* path = SDL_GetBasePath();
    if (path) {
        std::string result(path);
        SDL_free(path);
        return result;
    }
    return "./";
}

namespace {
    void DebugLog(const std::string& message) {
        static std::string exePath = GetExePath();
        static std::string logPath = exePath + "controller_final.txt";
        static std::ofstream debugFile(logPath, std::ios::app);
        
        if (debugFile.is_open()) {
            debugFile << message << std::endl;
            debugFile.flush();
        }
    }

    std::unordered_map<std::string, int> LoadJoystickConfig() {
        std::unordered_map<std::string, int> config;
        
        // Valores por defecto
        config["A"] = 0;
        config["B"] = 1;
        config["X"] = 2;
        config["Y"] = 3;
        config["LB"] = 6;
        config["RB"] = 4;
        config["BACK"] = 8;
        config["START"] = 9;
        config["L3"] = 10;
        config["R3"] = 11;
        config["DPAD_UP"] = 10;
        config["DPAD_DOWN"] = 11;
        config["DPAD_LEFT"] = 12; 
        config["DPAD_RIGHT"] = 13;
        config["LT"] = 7;    // L2 como botón
        config["RT"] = 5;    // R2 como botón
        
        // Cargar desde archivo EN CARPETA DEL EXE
        std::string exePath = GetExePath();
        std::string configPath = exePath + "joystick_config.txt";
        std::ifstream file(configPath);
        
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                
                size_t equals = line.find('=');
                if (equals != std::string::npos) {
                    std::string key = line.substr(0, equals);
                    std::string value = line.substr(equals + 1);
                    
                    try {
                        int num = std::stoi(value);
                        config[key] = num;
                    } catch (...) {
                        DebugLog("Error en línea: " + line);
                    }
                }
            }
            file.close();
        } else {
            DebugLog("No se encontro " + configPath + " - usando valores por defecto");
        }
        
        return config;
    }

    constexpr int32_t RepeatDelayMs = 300;
    constexpr int16_t AXIS_THRESHOLD = 16000;

    int MapDirectInputButtonToXInput(int button) {
        //comentado DebugLog("MapDirectInputButtonToXInput: boton fisico " + std::to_string(button));
        
        for (const auto& [name, physicalButton] : g_joystickConfig) {
            if (button == physicalButton) {
                //comentado DebugLog("                            -> Es " + name);
                
                if (name == "A") return SDL_CONTROLLER_BUTTON_A;
                if (name == "B") return SDL_CONTROLLER_BUTTON_B;
                if (name == "X") return SDL_CONTROLLER_BUTTON_X;
                if (name == "Y") return SDL_CONTROLLER_BUTTON_Y;
                if (name == "LB") return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
                if (name == "RB") return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
                if (name == "BACK") return SDL_CONTROLLER_BUTTON_BACK;
                if (name == "START") return SDL_CONTROLLER_BUTTON_START;
                if (name == "L3") return SDL_CONTROLLER_BUTTON_LEFTSTICK;
                if (name == "R3") return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
                if (name == "DPAD_UP") return SDL_CONTROLLER_BUTTON_DPAD_UP;
                if (name == "DPAD_DOWN") return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
                if (name == "DPAD_LEFT") return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
                if (name == "DPAD_RIGHT") return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
                //if (name == "LT") return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;  // o el que quieras
                //if (name == "RT") return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; // o el que quieras
            }
        }
        
        DebugLog("  -> NO MAPEADO");
        return -1;
    }

void GenerateControllerButtonEvent(SDL_JoystickID instance_id, int button, bool pressed) {
    SDL_Event event;
    event.type = pressed ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP;
    event.cbutton.which = instance_id;
    event.cbutton.button = static_cast<Uint8>(button);
    event.cbutton.state = pressed ? SDL_PRESSED : SDL_RELEASED;

    // LOG MEJORADO
/*comentado     DebugLog("Evento fake: Boton " + std::to_string(button) + 
             (pressed ? " PRESIONADO" : " LIBERADO") + 
             " (instancia " + std::to_string(instance_id) + ")");
  comentado*/
    if (SDL_PushEvent(&event) < 0) {
        DebugLog("ERROR SDL: " + std::string(SDL_GetError()));
    }
}

void GenerateControllerAxisEvent(SDL_JoystickID instance_id, int axis, int16_t value) {
    SDL_Event event;
    event.type = SDL_CONTROLLERAXISMOTION;
    event.caxis.which = instance_id;
    event.caxis.axis = static_cast<Uint8>(axis);
    event.caxis.value = value;
    
    if (SDL_PushEvent(&event) < 0) {
        DebugLog("ERROR SDL eje: " + std::string(SDL_GetError()));
    }
}

void AddFakeMapping(SDL_Joystick* joystick) {
    if (!joystick) return;
    
    SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
    char guidStr[33] = {0};
    SDL_JoystickGetGUIDString(guid, guidStr, sizeof(guidStr));

    int vendor = SDL_JoystickGetVendor(joystick);
    int product = SDL_JoystickGetProduct(joystick);

    // Crear mapping usando la CONFIGURACIÓN
    std::string mapping = std::string(guidStr) + ",Fake XInput Mapping," +
                          std::to_string(vendor) + ":" + std::to_string(product) +
                          ",platform:Windows,";
    
    // Agregar cada botón desde la configuración
    mapping += "a:b" + std::to_string(g_joystickConfig["A"]) + ",";
    mapping += "b:b" + std::to_string(g_joystickConfig["B"]) + ",";
    mapping += "x:b" + std::to_string(g_joystickConfig["X"]) + ",";
    mapping += "y:b" + std::to_string(g_joystickConfig["Y"]) + ",";
    mapping += "back:b" + std::to_string(g_joystickConfig["BACK"]) + ",";
    mapping += "start:b" + std::to_string(g_joystickConfig["START"]) + ",";
    mapping += "leftshoulder:b" + std::to_string(g_joystickConfig["LB"]) + ",";
    mapping += "rightshoulder:b" + std::to_string(g_joystickConfig["RB"]) + ",";
    mapping += "leftstick:b" + std::to_string(g_joystickConfig["L3"]) + ",";
    mapping += "rightstick:b" + std::to_string(g_joystickConfig["R3"]) + ",";
    mapping += "dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,";
    mapping += "leftx:a0,lefty:a1,rightx:a3,righty:a2,";  
    mapping += "lefttrigger:a4,righttrigger:a5,";
    
    int res = SDL_GameControllerAddMapping(mapping.c_str());
    //comentado DebugLogDebugLog(std::string("Añadiendo mapping SDL: ") + (res >= 0 ? "OK" : "ERROR"));
}
}

std::array<ControllerInput, 4> ControllerInput::Controllers;
int32_t ControllerInput::activeControllers = 0;

void ControllerInput::OpenController(int device) noexcept {
    gamepad = SDL_GameControllerOpen(device);
    if (gamepad) {
        SDL_Joystick* j = SDL_GameControllerGetJoystick(gamepad);
        instance_id = SDL_JoystickInstanceID(j);
        isConnected = true;
        isVirtualGamepad = false;

        //comentado DebugLog("GameController real conectado: " + 
        //comentario         std::string(SDL_GameControllerName(gamepad)) +
        //comentario         " instancia=" + std::to_string(instance_id));

        if (SDL_JoystickIsHaptic(j)) {
            haptic = SDL_HapticOpenFromJoystick(j);
            if (haptic && SDL_HapticRumbleSupported(haptic)) {
                if (SDL_HapticRumbleInit(haptic) != 0) {
                    SDL_HapticClose(haptic);
                    haptic = nullptr;
                }
            } else {
                if (haptic) SDL_HapticClose(haptic);
                haptic = nullptr;
            }
        }
    }
}

void ControllerInput::OpenJoystickAsGameController(int device_index) noexcept {
    //Comentado DebugLog("Abriendo joystick DirectInput index=" + std::to_string(device_index));

    joystick = SDL_JoystickOpen(device_index);
    if (!joystick) {
        //comentado DebugLog("ERROR: No se pudo abrir joystick físico");
        return;
    }

    instance_id = SDL_JoystickInstanceID(joystick);
    physicalDeviceIndex = device_index;
    isConnected = true;
    isVirtualGamepad = true;

    AddFakeMapping(joystick); // <-- ESTO DEBE ESTAR ACTIVO

    const char* name = SDL_JoystickName(joystick);

    //comentado DebugLog("Joystick DirectInput conectado:");
    //comentado DebugLog("  Nombre: " + std::string(name ? name : "Desconocido"));
    //comentado DebugLog("  Instancia=" + std::to_string(instance_id));
    //comentado DebugLog("  Botones=" + std::to_string(SDL_JoystickNumButtons(joystick)));
    //comentado DebugLog("  Ejes=" + std::to_string(SDL_JoystickNumAxes(joystick)));

    // Fake controller added event - ESTO TAMBIÉN
    SDL_Event ev;
    ev.type = SDL_CONTROLLERDEVICEADDED;
    ev.cdevice.which = device_index;  
    SDL_PushEvent(&ev);

    //comentado DebugLog("Generado evento CONTROLLERDEVICEADDED (fake XInput)");
}

void ControllerInput::CloseController() noexcept {
    if (isConnected) {
        //comentado DebugLog("Desconectando controlador instancia=" + std::to_string(instance_id));
        isConnected = false;
        if (haptic) { SDL_HapticClose(haptic); haptic = nullptr; }
        if (gamepad) { SDL_GameControllerClose(gamepad); gamepad = nullptr; }
        if (joystick) { SDL_JoystickClose(joystick); joystick = nullptr; }
    }
}

int ControllerInput::GetControllerIndex(SDL_JoystickID instance) noexcept {
    for (int i = 0; i < Controllers.size(); i++) {
        if (Controllers[i].isConnected && Controllers[i].instance_id == instance)
            return i;
    }
    return -1;
}

// Eventos GameController
void ControllerInput::ControllerButtonDown(const OFS_SDL_Event* ev) const noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& cbutton = ev->sdl.cbutton;
    
    // TABLA de nombres XInput
    const char* xinputNames[] = {
        "A", "B", "X", "Y", 
        "BACK", "GUIDE", "START",
        "LEFTSTICK", "RIGHTSTICK", 
        "LB", "RB",
        "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT"
    };
    
    const char* buttonName = "UNKNOWN";
    if (cbutton.button >= 0 && cbutton.button < 15) {
        buttonName = xinputNames[cbutton.button];
    }
    
    // Detectar si es fake o real
    bool isFake = false;
    for (int i = 0; i < Controllers.size(); i++) {
        if (Controllers[i].isConnected && 
            Controllers[i].instance_id == cbutton.which &&
            Controllers[i].isVirtualGamepad) {
            isFake = true;
            break;
        }
    }
    
    if (isFake) {
        //comentado DebugLog("FAKE XInput recibido: " + std::string(buttonName) + 
        //comentado         " (ID " + std::to_string(cbutton.button) + ")");
    } else {
        //comentado DebugLog("XInput REAL recibido: " + std::string(buttonName) + 
        //comentado         " (ID " + std::to_string(cbutton.button) + ")");
    }
    
    if (cbutton.button >= 0 && cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
        ButtonsHeldDown[cbutton.button] = (int64_t)SDL_GetTicks() + RepeatDelayMs;
}

void ControllerInput::ControllerButtonUp(const OFS_SDL_Event* ev) const noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& cbutton = ev->sdl.cbutton;
    
    // LOG MEJORADO
/*comentado    DebugLog("XInput REAL: Boton " + std::to_string(cbutton.button) + 
             " LIBERADO (instancia " + std::to_string(cbutton.which) + ")");
  comentado*/    
    if (cbutton.button >= 0 && cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
        ButtonsHeldDown[cbutton.button] = -1;
}

// Eventos Joystick
void ControllerInput::JoystickButtonDown(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& jbutton = ev->sdl.jbutton;
    
     //comentado DebugLog("JOYSTICK FISICO: Boton " + std::to_string(jbutton.button) + 
     //comentado         " PRESIONADO (instancia " + std::to_string(jbutton.which) + ")");

    for (int i = 0; i < Controllers.size(); i++) {
        auto& controller = Controllers[i];
        if (controller.isConnected && controller.isVirtualGamepad &&
            controller.instance_id == jbutton.which) {
            
            // PRIMERO verificar si es LT o RT
            if (jbutton.button == g_joystickConfig["LT"]) {
                GenerateControllerAxisEvent(jbutton.which, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 32767);
                return;
            }
            else if (jbutton.button == g_joystickConfig["RT"]) {
                GenerateControllerAxisEvent(jbutton.which, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);
                return;
            }
            
            // Si no es LT/RT, mapear normalmente
            int xinputButton = MapDirectInputButtonToXInput(jbutton.button);
            if (xinputButton >= 0) {
                GenerateControllerButtonEvent(jbutton.which, xinputButton, true);
            }
            return;
        }
    }
}


void ControllerInput::JoystickButtonUp(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& jbutton = ev->sdl.jbutton;
    
    for (int i = 0; i < Controllers.size(); i++) {
        auto& controller = Controllers[i];
        if (controller.isConnected && controller.isVirtualGamepad &&
            controller.instance_id == jbutton.which) {
            
            // PRIMERO verificar si es LT o RT (EJES, no botones)
            if (jbutton.button == g_joystickConfig["LT"]) {
                GenerateControllerAxisEvent(jbutton.which, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0); //  VALOR 0
                return;
            }
            else if (jbutton.button == g_joystickConfig["RT"]) {
                GenerateControllerAxisEvent(jbutton.which, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0); //  VALOR 0
                return;
            }
            
            // Si no es LT/RT, mapear normalmente como botón
            int xinputButton = MapDirectInputButtonToXInput(jbutton.button);
            if (xinputButton >= 0) {
                GenerateControllerButtonEvent(jbutton.which, xinputButton, false);
            }
            return;
        }
    }
}

void ControllerInput::JoystickAxisMotion(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& jaxis = ev->sdl.jaxis;
    
    
    
    for (int i = 0; i < Controllers.size(); i++) {
        auto& controller = Controllers[i];
        if (controller.isConnected && controller.isVirtualGamepad &&
            controller.instance_id == jaxis.which) {
            if (jaxis.axis == 0) {
                GenerateControllerButtonEvent(jaxis.which, SDL_CONTROLLER_BUTTON_DPAD_LEFT, jaxis.value < -AXIS_THRESHOLD);
                GenerateControllerButtonEvent(jaxis.which, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, jaxis.value > AXIS_THRESHOLD);
            } else if (jaxis.axis == 1) {
                GenerateControllerButtonEvent(jaxis.which, SDL_CONTROLLER_BUTTON_DPAD_UP, jaxis.value < -AXIS_THRESHOLD);
                GenerateControllerButtonEvent(jaxis.which, SDL_CONTROLLER_BUTTON_DPAD_DOWN, jaxis.value > AXIS_THRESHOLD);
            }
            return;
        }
    }
}

void ControllerInput::JoystickHatMotion(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& jhat = ev->sdl.jhat;
    for (int i = 0; i < Controllers.size(); i++) {
        auto& controller = Controllers[i];
        if (controller.isConnected && controller.isVirtualGamepad &&
            controller.instance_id == jhat.which) {
            GenerateControllerButtonEvent(jhat.which, SDL_CONTROLLER_BUTTON_DPAD_UP, jhat.value & SDL_HAT_UP);
            GenerateControllerButtonEvent(jhat.which, SDL_CONTROLLER_BUTTON_DPAD_DOWN, jhat.value & SDL_HAT_DOWN);
            GenerateControllerButtonEvent(jhat.which, SDL_CONTROLLER_BUTTON_DPAD_LEFT, jhat.value & SDL_HAT_LEFT);
            GenerateControllerButtonEvent(jhat.which, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, jhat.value & SDL_HAT_RIGHT);
            return;
        }
    }
}

void ControllerInput::ControllerDeviceAdded(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& cdevice = ev->sdl.cdevice;
    for (int i = 0; i < Controllers.size(); i++) {
        if (Controllers[i].isConnected && Controllers[i].isVirtualGamepad &&
            Controllers[i].physicalDeviceIndex == cdevice.which) {
            //comentado DebugLog("  Evento CONTROLLERDEVICEADDED ignorado (joystick virtual).");
            return;
        }
    }
    //comentado DebugLog("ControllerDeviceAdded: device=" + std::to_string(cdevice.which));
    for (int i = 0; i < Controllers.size(); i++) {
        if (!Controllers[i].isConnected) {
            Controllers[i].OpenController(cdevice.which);
            if (Controllers[i].isConnected) {
                activeControllers++;
                //comentado DebugLog("  Controlador asignado al slot " + std::to_string(i) + 
                //comentado DebugLog         ", total activos: " + std::to_string(activeControllers));
            }
            break;
        }
    }
}

void ControllerInput::JoystickDeviceAdded(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    auto& jdevice = ev->sdl.jdevice;
    int device_index = jdevice.which;
    
    // 1. VERIFICAR SI ES XINPUT (control Xbox real)
    const char* name = SDL_JoystickNameForIndex(device_index);
    bool isXInput = false;
    
    if (name) {
        std::string nameStr(name);
        // Busca indicadores de que es XInput
        if (nameStr.find("Xbox") != std::string::npos ||
            nameStr.find("XInput") != std::string::npos ||
            nameStr.find("X360") != std::string::npos ||
            nameStr.find("X-Box") != std::string::npos) {
            isXInput = true;
        }
    }
    
    // 2. SI ES XINPUT, NO HACER NADA (dejar que SDL lo maneje)
    if (isXInput) {
        //comentado DebugLog("  Ignorando joystick (es XInput nativo): " + std::string(name ? name : "Unknown"));
        return;
    }
    
    // 3. SOLO PARA DIRECTINPUT, convertir a fake controller
    //comentado DebugLog("  Detectado joystick DirectInput: " + std::string(name ? name : "Unknown"));
    
    for (int i = 0; i < Controllers.size(); i++) {
        if (!Controllers[i].isConnected) {
            Controllers[i].OpenJoystickAsGameController(device_index);
            if (Controllers[i].isConnected) {
                activeControllers++;
                //comentado DebugLog("  DirectInput convertido a fake XInput en slot " + std::to_string(i));
            }
            return;
        }
    }
}

void ControllerInput::ControllerDeviceRemoved(const OFS_SDL_Event* ev) noexcept {
    OFS_PROFILE(__FUNCTION__);
    int cIndex = GetControllerIndex(ev->sdl.cdevice.which);
    if (cIndex < 0) {
        //comentado DebugLog("ControllerDeviceRemoved: instancia no encontrada");
        return;
    }
    auto& jc = Controllers[cIndex];
    //comentado DebugLog("Desconectando controlador del slot " + std::to_string(cIndex));
    jc.CloseController();
    activeControllers--;
    //comentado DebugLog("Total activos: " + std::to_string(activeControllers));
}

void ControllerInput::Init() noexcept {
    //comentado DebugLog("=== INICIALIZANDO ControllerInput ===");
    
    // CARGAR CONFIGURACIÓN DE BOTONES
    g_joystickConfig = LoadJoystickConfig();
    //comentado DebugLog("Configuración de joystick cargada");
    
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_GameControllerEventState(SDL_ENABLE);

    // GameController
    EV::Queue().appendListener(SDL_CONTROLLERDEVICEADDED,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::ControllerDeviceAdded)));
    EV::Queue().appendListener(SDL_CONTROLLERDEVICEREMOVED,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::ControllerDeviceRemoved)));
    EV::Queue().appendListener(SDL_CONTROLLERBUTTONDOWN,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::ControllerButtonDown)));
    EV::Queue().appendListener(SDL_CONTROLLERBUTTONUP,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::ControllerButtonUp)));

    // Joystick
    EV::Queue().appendListener(SDL_JOYDEVICEADDED,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::JoystickDeviceAdded)));
    EV::Queue().appendListener(SDL_JOYBUTTONDOWN,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::JoystickButtonDown)));
    EV::Queue().appendListener(SDL_JOYBUTTONUP,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::JoystickButtonUp)));
    EV::Queue().appendListener(SDL_JOYAXISMOTION,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::JoystickAxisMotion)));
    EV::Queue().appendListener(SDL_JOYHATMOTION,
        OFS_SDL_Event::HandleEvent(EVENT_SYSTEM_BIND(this, &ControllerInput::JoystickHatMotion)));

    //comentado DebugLog("=== INICIALIZACION COMPLETADA ===");
}

void ControllerInput::Update() noexcept {}
void ControllerInput::UpdateControllers() noexcept {}

void ControllerInput::Shutdown() noexcept {
    //comentado DebugLog("=== APAGANDO ControllerInput ===");
    for (auto& controller : Controllers) {
        controller.CloseController();
    }
    activeControllers = 0;
}

