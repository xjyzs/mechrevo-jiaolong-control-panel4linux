#include "main.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include "sudoHelper.hpp"
#include "utils/gpuUtil.hpp"

using namespace std;

void checkOnlineStatus(slint::ComponentHandle<MainWindow> ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)]() {
        string result;
        root_system("echo '\\_SB.INOU.ECRR 0x0741' | tee /proc/acpi/call;cat /proc/acpi/call", &result);
        bool onlineStatus = !result.empty() && result.back() == '1';
        slint::invoke_from_event_loop([app_handle, onlineStatus]() {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_onlineStatus(onlineStatus);
            }
        });
    }).detach();
}

void checkGpu(slint::ComponentHandle<MainWindow> ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)] {
        bool result = isNvidiaConnected();
        slint::invoke_from_event_loop([app_handle, result] {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_nvidiaStatus(result);
            }
        });
    }).detach();
}

void checkGpuAfterReboot(slint::ComponentHandle<MainWindow> ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)] {
        bool result = getGpuAfterReboot();
        slint::invoke_from_event_loop([app_handle, result] {
            if (auto strong_app = app_handle.lock()) {
                (*strong_app)->set_nvidiaStatusAfterReboot(result);
            }
        });
    }).detach();
}

void checkPerfMode(slint::ComponentHandle<MainWindow> ui) {
    thread([app_handle = slint::ComponentWeakHandle(ui)]() {
        string result;
        root_system("echo '\\_SB.INOU.ECRR 0x0751' | tee /proc/acpi/call;cat /proc/acpi/call", &result);
        if (!result.empty()) {
            int perfMode=-1;
            if (result.ends_with("a0")) perfMode = 0;
            else if (result.ends_with("10")) perfMode = 2;
            else if (result.ends_with('0')) perfMode = 1;
            slint::invoke_from_event_loop([app_handle, perfMode]() {
                if (auto strong_app = app_handle.lock()) {
                    (*strong_app)->set_perfMode(perfMode);
                }
            });
        }
    }).detach();
}

int main(int argc, char *argv[]) {
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

    ui->on_onlineStatusToggled([ui](bool i) {
        if (i) root_system("echo '\\_SB.INOU.ECRW 0x0741 0x0' | sudo tee /proc/acpi/call");
        else root_system("echo '\\_SB.INOU.ECRW 0x0741 0x01' | sudo tee /proc/acpi/call");
        checkOnlineStatus(ui);
    });

    ui->on_gpuToggled([ui](bool i) {
        if (i)
            root_system( "python3 -c '"
        "import os, subprocess; "
        "p = \"/sys/firmware/efi/efivars/OemMagicVariable-9f33f85c-13ca-4fd1-9c4a-96217722c593\"; "
        "data = bytearray(open(p, \"rb\").read()); "
        "data[0x66] = 0x00; "
        "subprocess.run([\"chattr\", \"-i\", p]); "
        "fd = os.open(p, os.O_WRONLY); "
        "os.write(fd, data); "
        "os.close(fd)"
        "'");
        else
            root_system( "python3 -c '"
        "import os, subprocess; "
        "p = \"/sys/firmware/efi/efivars/OemMagicVariable-9f33f85c-13ca-4fd1-9c4a-96217722c593\"; "
        "data = bytearray(open(p, \"rb\").read()); "
        "data[0x66] = 0x01; "
        "subprocess.run([\"chattr\", \"-i\", p]); "
        "fd = os.open(p, os.O_WRONLY); "
        "os.write(fd, data); "
        "os.close(fd)"
        "'");
        checkGpuAfterReboot(ui);
    });

    ui->on_perfModeChange([ui](int i) {
        if (i==0) root_system("echo '\\_SB.INOU.ECRW 0x0751 0xa0' | tee /proc/acpi/call");
        else if (i==1) root_system("echo '\\_SB.INOU.ECRW 0x0751 0x0' | tee /proc/acpi/call");
        else if (i==2) root_system("echo '\\_SB.INOU.ECRW 0x0751 0x10' | tee /proc/acpi/call");
        checkPerfMode(ui);
    });

    ui->run();
    return 0;
}
