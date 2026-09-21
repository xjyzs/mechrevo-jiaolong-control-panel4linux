#include "main.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <filesystem>
#include "utils/sudoHelper.hpp"
#include "utils/ecUtil.hpp"
#include "utils/gpuUtil.hpp"

using namespace std;

static void checkOnlineStatus(const slint::ComponentHandle<MainWindow>& ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)]() {
        const int result=readEc(0x0741);
        bool onlineStatus = result==0x81||result==0x1;
        slint::invoke_from_event_loop([app_handle, onlineStatus]() {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_onlineStatus(onlineStatus);
            }
        });
    }).detach();
}

static void checkGpu(const slint::ComponentHandle<MainWindow>& ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)] {
        bool result = isNvidiaConnected();
        slint::invoke_from_event_loop([app_handle, result] {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_nvidiaStatus(result);
            }
        });
    }).detach();
}

static void checkGpuAfterReboot(const slint::ComponentHandle<MainWindow>& ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)] {
        bool result = getGpuAfterReboot();
        slint::invoke_from_event_loop([app_handle, result] {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_nvidiaStatusAfterReboot(result);
            }
        });
    }).detach();
}

static void checkPerfMode(const slint::ComponentHandle<MainWindow>& ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)]() {
        int result = readEc(0x0751);
        int resultWild = readEc(0x0728);
        int resultCustom = readEc(0x0727);
        int perfMode = -1;
        if (result == 0xa0) perfMode = 0;
        else if (result == 0x0) perfMode = 1;
        else if (result == 0x10) {
            perfMode = 2;
            if (resultWild == 0x1) perfMode = 3;
            else if (resultWild == 0x0) perfMode = 4;
        }
        if (resultCustom == 0x40) perfMode = 5;
        slint::invoke_from_event_loop([app_handle, perfMode]() {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_perfMode(perfMode);
                (*strong_app)->set_modeRadioGroupEnabled(true);
                (*strong_app)->set_initialized(true);
            }
        });
    }).detach();
}

static void checkFan(const slint::ComponentHandle<MainWindow>& ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)] {
        while (true) {
            string result;
            int cpuFanRatio = readEc(0x075B);
            int gpuFanRatio = readEc(0x075C);
            int cpuFan = readEc(0x0464, 0x0465);
            int gpuFan = readEc(0x046C, 0x046B);

            slint::invoke_from_event_loop([app_handle, cpuFanRatio,gpuFanRatio,cpuFan,gpuFan]() {
                if (auto strong_app = app_handle.lock()) {
                    (*strong_app)->set_cpuFanRatio(cpuFanRatio / 2.0);
                    (*strong_app)->set_gpuFanRatio(gpuFanRatio / 2.0);
                    (*strong_app)->set_cpuFan(cpuFan);
                    (*strong_app)->set_gpuFan(gpuFan);
                }
            });
            sleep(1);
        }
    }).detach();
}


int main(const int argc, char *argv[]) {
    // 如果包含 --root-daemon 参数, 则启动特权命令执行器, 不启动 UI
    if (argc > 1 && strcmp(argv[1], "--root-daemon") == 0) {
        return run_as_root_daemon();
    }
    auto ui = MainWindow::create();

    // 启动时执行
    checkOnlineStatus(ui);
    checkGpu(ui);
    checkGpuAfterReboot(ui);
    checkPerfMode(ui);
    checkFan(ui);

    const fs::path acpiCallFile = "/proc/acpi/call";
    if (!fs::exists(acpiCallFile)) ui->set_show_acpi_call_alert(true);

    ui->on_onlineStatusToggled([ui](bool i) {
        if (i) writeEc(0x0741,0x1);
        else writeEc(0x0741,0x0);
        checkOnlineStatus(ui);
    });

    ui->on_gpuToggled([ui](const bool i) {
        if (i)
            root_system("python3 -c '"
                "import os, subprocess; "
                "p = \"/sys/firmware/efi/efivars/OemMagicVariable-9f33f85c-13ca-4fd1-9c4a-96217722c593\"; "
                "data = bytearray(open(p, \"rb\").read()); "
                "data[0x66] = 0x01; "
                "subprocess.run([\"chattr\", \"-i\", p]); "
                "fd = os.open(p, os.O_WRONLY); "
                "os.write(fd, data); "
                "os.close(fd)"
                "'");
        else
            root_system("python3 -c '"
                "import os, subprocess; "
                "p = \"/sys/firmware/efi/efivars/OemMagicVariable-9f33f85c-13ca-4fd1-9c4a-96217722c593\"; "
                "data = bytearray(open(p, \"rb\").read()); "
                "data[0x66] = 0x00; "
                "subprocess.run([\"chattr\", \"-i\", p]); "
                "fd = os.open(p, os.O_WRONLY); "
                "os.write(fd, data); "
                "os.close(fd)"
                "'");
        checkGpuAfterReboot(ui);
        ui->set_show_reboot_alert(true);
    });

    ui->on_perfModeChange([ui](const int i) {
        cout << i << endl;
        if (i == 5) writeEc(0x0727,0x40);
        else {
            writeEc(0x0727,0x0);
            if (i == 0) writeEc(0x0751,0xa0);
            else if (i == 1) writeEc(0x0751,0x0);
            else if (i >= 2 && i < 5) {
                writeEc(0x0751,0x10);
                if (i == 3) writeEc(0x0728,0x1);
                else if (i == 4) writeEc(0x0728,0x0);
            }
        }
        checkPerfMode(ui);
    });

    ui->on_reboot([ui] {
    root_system("reboot");
});

    ui->run();
    return 0;
}
