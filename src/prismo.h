// Prismo: RGB lighting control for N5312A-based mice (tested: Ant Esports KM540, USB 4e53:5406).
#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace prismo {

constexpr uint16_t kVendorId = 0x4E53;
constexpr uint16_t kProductId = 0x5406;

enum Mode { Static, Breathe, Rainbow, Off, ModeCount };
inline const char* const kModeNames[ModeCount] = {"Static", "Breathe", "Rainbow", "Off"};

struct Settings {
    uint8_t r = 255, g = 0, b = 0;
    int mode = Static;
    int brightness = 90;  // 10-100
    int speed = 5;        // 1-10
};

// Sends the settings to the mouse. On failure returns false and fills err.
bool apply(const Settings& s, std::string& err);

Settings load_settings();
bool save_settings(const Settings& s);

// ---- implemented per OS (device_linux.cpp / device_windows.cpp) ----
namespace platform {

struct Device;
// Opens the mouse's vendor HID collection (usage page 0xFF01); nullptr if not plugged in.
Device* open_device();
bool send_feature(Device* d, const uint8_t* data, size_t len);
void close_device(Device* d);

std::filesystem::path config_dir();

}  // namespace platform
}  // namespace prismo
