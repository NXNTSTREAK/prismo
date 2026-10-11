#include "prismo.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

namespace prismo {

namespace {

constexpr uint8_t kModeCodes[ModeCount] = {0x02, 0x01, 0x00, 0x03};  // static, breathe, rainbow, off

std::filesystem::path config_file() { return platform::config_dir() / "config.ini"; }

}  // namespace

bool apply(const Settings& s, std::string& err) {
    platform::Device* dev = platform::open_device();
    if (!dev) {
        err = "Mouse not found. Is it plugged in?";
        return false;
    }
    bool ok = true;
    auto send = [&](std::initializer_list<uint8_t> payload) {
        if (!ok) return;
        uint8_t buf[8] = {0x07};  // report ID 7, then 7 payload bytes
        std::copy(payload.begin(), payload.end(), buf + 1);
        ok = platform::send_feature(dev, buf, sizeof buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    };
    uint8_t speed = uint8_t(std::clamp(s.speed, 1, 10));
    uint8_t brightness = uint8_t(std::clamp(s.brightness, 10, 100));

    // Sequence replayed from a USB capture of the vendor software.
    send({0xa0});                                  // init
    for (uint8_t i = 1; i <= 5; ++i) send({0x01, i, i});
    send({0x09, 0x06, 0xf0});
    send({0x01, 0x07, 0x07});
    send({0x08, 0x01, 0x04});
    send({0x0b, 0x00, 0xff, 0x00, 0x00});         // color slots: index, R, G, B
    send({0x0b, 0x01, s.r, s.g, s.b});             // slot 1 is the one static/breathe use
    send({0x0b, 0x02, 0x00, 0x00, 0xff});
    send({0x0b, 0x03, 0xff, 0x00, 0xff});
    send({0x0a, kModeCodes[s.mode], 0x01, 0x01, 0x01, speed, brightness});

    platform::close_device(dev);
    if (!ok) err = "Could not send to the mouse.";
    return ok;
}

Settings load_settings() {
    Settings s;
    std::ifstream in(config_file());
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq), val = line.substr(eq + 1);
        if (key == "color") {
            unsigned r, g, b;
            if (std::sscanf(val.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
                s.r = uint8_t(r), s.g = uint8_t(g), s.b = uint8_t(b);
        } else if (key == "mode") {
            for (int m = 0; m < ModeCount; ++m)
                if (val == kModeNames[m]) s.mode = m;
        } else if (key == "brightness") {
            s.brightness = std::clamp(std::atoi(val.c_str()), 10, 100);
        } else if (key == "speed") {
            s.speed = std::clamp(std::atoi(val.c_str()), 1, 10);
        }
    }
    return s;
}

bool save_settings(const Settings& s) {
    std::error_code ec;
    std::filesystem::create_directories(platform::config_dir(), ec);
    std::ofstream out(config_file());
    char color[8];
    std::snprintf(color, sizeof color, "#%02x%02x%02x", s.r, s.g, s.b);
    out << "color=" << color << "\nmode=" << kModeNames[s.mode]
        << "\nbrightness=" << s.brightness << "\nspeed=" << s.speed << "\n";
    return bool(out);
}

}  // namespace prismo
