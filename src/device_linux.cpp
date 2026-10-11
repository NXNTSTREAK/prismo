// Linux: talk to the mouse through hidraw.
#include "prismo.h"

#include <fcntl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>

namespace prismo::platform {

namespace fs = std::filesystem;

struct Device {
    int fd;
};

namespace {

std::string read_file(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

}  // namespace

Device* open_device() {
    char id[32];
    std::snprintf(id, sizeof id, "%08X:%08X", kVendorId, kProductId);
    std::error_code ec;
    for (const auto& node : fs::directory_iterator("/sys/class/hidraw", ec)) {
        if (read_file(node.path() / "device/uevent").find(id) == std::string::npos) continue;
        // The vendor collection's report descriptor starts a usage page 0xFF01 (06 01 FF).
        if (read_file(node.path() / "device/report_descriptor").find("\x06\x01\xff") == std::string::npos)
            continue;
        int fd = open(("/dev/" + node.path().filename().string()).c_str(), O_RDWR | O_CLOEXEC);
        if (fd >= 0) return new Device{fd};
    }
    return nullptr;
}

bool send_feature(Device* d, const uint8_t* data, size_t len) {
    uint8_t buf[64];
    std::copy(data, data + len, buf);
    return ioctl(d->fd, HIDIOCSFEATURE(len), buf) == int(len);
}

void close_device(Device* d) {
    close(d->fd);
    delete d;
}

fs::path config_dir() {
    if (const char* x = std::getenv("XDG_CONFIG_HOME"); x && *x) return fs::path(x) / "prismo";
    const char* home = std::getenv("HOME");
    return fs::path(home ? home : ".") / ".config" / "prismo";
}

}  // namespace prismo::platform
