// Windows: talk to the mouse through the built-in HID API (hid.dll + SetupAPI).
#include "prismo.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// windows.h must come first; hidsdi.h pulls in hidusage.h + hidpi.h
#include <hidsdi.h>
#include <setupapi.h>

#include <algorithm>
#include <vector>

namespace prismo::platform {

namespace fs = std::filesystem;

struct Device {
    HANDLE h;
    USHORT feature_len;
};

Device* open_device() {
    GUID guid;
    HidD_GetHidGuid(&guid);
    HDEVINFO info = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) return nullptr;

    Device* found = nullptr;
    SP_DEVICE_INTERFACE_DATA ifd{sizeof ifd};
    for (DWORD i = 0; !found && SetupDiEnumDeviceInterfaces(info, nullptr, &guid, i, &ifd); ++i) {
        DWORD need = 0;
        SetupDiGetDeviceInterfaceDetailW(info, &ifd, nullptr, 0, &need, nullptr);
        std::vector<BYTE> buf(need);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        detail->cbSize = sizeof *detail;
        if (!SetupDiGetDeviceInterfaceDetailW(info, &ifd, detail, need, nullptr, nullptr)) continue;

        // Keyboard/mouse collections refuse read/write opens; the vendor one allows it.
        HANDLE h = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attr{sizeof attr};
        PHIDP_PREPARSED_DATA pp;
        if (HidD_GetAttributes(h, &attr) && attr.VendorID == kVendorId && attr.ProductID == kProductId &&
            HidD_GetPreparsedData(h, &pp)) {
            HIDP_CAPS caps;
            if (HidP_GetCaps(pp, &caps) == HIDP_STATUS_SUCCESS && caps.UsagePage == 0xFF01)
                found = new Device{h, caps.FeatureReportByteLength};
            HidD_FreePreparsedData(pp);
        }
        if (!found) CloseHandle(h);
    }
    SetupDiDestroyDeviceInfoList(info);
    return found;
}

bool send_feature(Device* d, const uint8_t* data, size_t len) {
    // Windows wants the buffer to be exactly the report's declared length.
    std::vector<uint8_t> buf(std::max<size_t>(d->feature_len, len), 0);
    std::copy(data, data + len, buf.begin());
    return HidD_SetFeature(d->h, buf.data(), ULONG(buf.size()));
}

void close_device(Device* d) {
    CloseHandle(d->h);
    delete d;
}

fs::path config_dir() {
    const wchar_t* appdata = _wgetenv(L"APPDATA");
    return fs::path(appdata ? appdata : L".") / L"prismo";
}

}  // namespace prismo::platform
