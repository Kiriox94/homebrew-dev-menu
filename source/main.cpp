#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <format>
#include <vector>
#include <deque>
#include <filesystem>

#include <switch.h>
#include "./netload.hpp"

using namespace std;
namespace fs = std::filesystem;

#define MAX_LOGS 12

struct HomebrewInfo
{
    string name;
    string author;
    string version;
    string path;

    bool isEmpty()
    {
        return path.empty() || name.empty();
    }
};

Thread thread;
size_t selectedHomebrew = 0;
vector<HomebrewInfo> homebrews;
deque<string> logs = {};   

void addLog(string log) {
    if (logs.size() > MAX_LOGS) {
        logs.pop_front(); // Delete the oldest log
    }
    logs.push_back(format("- {}", log));
}

HomebrewInfo getNroInfo(string filePath)
{
    FILE *file = fopen(filePath.c_str(), "r");

    NroHeader header;
    NroAssetHeader assetHeader;
    NacpStruct nacp;

    fseek(file, sizeof(NroStart), SEEK_SET);
    if (fread(&header, sizeof(NroHeader), 1, file) != 1)
    {
        fclose(file);
        return {};
    }

    fseek(file, header.size, SEEK_SET);
    if (fread(&assetHeader, sizeof(NroAssetHeader), 1, file) != 1)
    {
        fclose(file);
        return {};
    }

    fseek(file, header.size + assetHeader.nacp.offset, SEEK_SET);
    if (fread(&nacp, sizeof(NacpStruct), 1, file) != 1)
    {
        fclose(file);
        return {};
    }

    fclose(file);

    return {
        nacp.lang[0].name,
        nacp.lang[0].author,
        nacp.display_version,
        filePath
    };
}

void updateHomebrewsList()
{
    addLog("Updating homebrew list...");
    homebrews.clear();
    for (const auto &e : fs::directory_iterator("/switch/.dev"))
    {
        printf("Found file: %s\n", e.path().string().c_str());
        if (e.is_regular_file())
        {
            auto info = getNroInfo(e.path().string());
            if (!info.isEmpty())
            {
                homebrews.push_back(info);
            }
        }
    }
}

bool launchHomebrew(string path) {
    addLog(format("Launching {}...", path));
    if (R_SUCCEEDED(envSetNextLoad(path.c_str(), path.c_str())))
    {
        return true;
    }
    else
    {
        addLog(format("Failed to launch {}.", path));
    }
    return false;
}

void print_progress_bar(float progress) {
    int bar_width = 17;
    int pos = bar_width * progress;

    std::cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos)
            std::cout << "#";
        else
            std::cout << "-";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
    std::cout << std::endl;
}

bool cp(const std::string& filein, const std::string& fileout) {
    FILE *exein, *exeout;
    exein = fopen(filein.c_str(), "rb");
    if (exein == NULL) {
        /* handle error */
        perror("file open for reading");
        return false;
    }
    exeout = fopen(fileout.c_str(), "wb");
    if (exeout == NULL) {
        /* handle error */
        perror("file open for writing");
        return false;
    }
    size_t n, m;
    unsigned char buff[131072];
    do {
        n = fread(buff, 1, sizeof buff, exein);
        if (n) m = fwrite(buff, 1, n, exeout);
        else   m = 0;
    }
    while ((n > 0) && (n == m));
    if (m) {
        perror("copy");
        return false;
    }
    if (fclose(exeout)) {
        perror("close output file");
        return false;
    }
    if (fclose(exein)) {
        perror("close input file");
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);
    socketInitializeDefault();
    nifmInitialize(NifmServiceType_User);

    threadCreate(&thread, netloader::task, nullptr, nullptr, 0x1000, 0x2C, -2);
    threadStart(&thread);

    HomebrewInfo thisMetadata = getNroInfo(argv[0]);
    string instructions = format("======= DevMenu v{} =======\nPress B to exit.\nPress - to launch homebrew menu\nPress + to refresh homebrew list.\n", thisMetadata.version);

    if (!fs::exists("/switch/.dev")) fs::create_directories("/switch/.dev");
    updateHomebrewsList();
    if(homebrews.size() > 0) instructions += "Press A to launch homebrew.\nPress Y to delete homebrew.\nPress X to add homebrew to Homebrew Menu.\n";

    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_B) {
            break;
        } else if (kDown & HidNpadButton_Minus && launchHomebrew("/hbmenu.nro")) {
            break;
        } else if (kDown & HidNpadButton_AnyUp) {
            if(selectedHomebrew > 0) {
                selectedHomebrew--;
            }else {
                selectedHomebrew = homebrews.size() - 1;
            }
        } else if (kDown & HidNpadButton_AnyDown) {
            if(selectedHomebrew < homebrews.size() - 1) {
                selectedHomebrew++;
            }else {
                selectedHomebrew = 0;
            }
        } else if (kDown & HidNpadButton_Plus) {
            updateHomebrewsList();
        } else if (kDown & HidNpadButton_A && !homebrews.empty()) {
            auto homebrew = homebrews[selectedHomebrew];
            if (!homebrew.isEmpty()) {
                if (launchHomebrew(homebrew.path)) break;
            }
        } else if (kDown & HidNpadButton_X && !homebrews.empty()) {
            auto homebrew = homebrews[selectedHomebrew];
            if (!homebrew.isEmpty()) {
                std::string filename = homebrew.path.substr(homebrew.path.find_last_of("/\\") + 1);
                std::string dest = "/switch/" + filename;
                if (cp(homebrew.path, dest)) {
                    addLog(format("{} was successfuly added to Homebrew Menu", filename));
                }else {
                    addLog(format("Error while copying {} to {}", homebrew.path, dest));
                }
            }
        } else if (kDown & HidNpadButton_Y && !homebrews.empty()) {
            auto homebrew = homebrews[selectedHomebrew];
            if (!homebrew.isEmpty())
            {
                addLog(format("Deleting {}...", homebrew.path));
                if (fs::remove(homebrew.path))
                {
                    addLog("Successfully deleted.");
                    if (selectedHomebrew > 0) selectedHomebrew -= 1;
                    updateHomebrewsList();
                }else
                {
                    addLog("Failed to delete.");
                }
            }
        }

        netloader::State state = {};
        netloader::getState(&state);

        consoleClear();
        cout << instructions << endl;

        u32 ip;
        nifmGetCurrentIpAddress(&ip);

        if (!state.errormsg.empty())
            addLog(state.errormsg);
        else if (ip == 0)
        {
            printf("No internet connection!");
        }
        else if (!state.sock_connected) {
            cout << format("Waiting for nxlink to connect...\nIP: {}.{}.{}.{}, Port: {}", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF, NXLINK_SERVER_PORT) << endl;
        }
        else {
            cout << "Transferring...\n" << endl;
            cout << format("{}/{} KiB written ", state.filetotal / 1024, state.filelen / 1024) << endl;
            print_progress_bar(static_cast<float>(state.filetotal) / state.filelen);
        }

        size_t i = 0;
        if(homebrews.size() > 0) cout << "\nPrevious homebrews:\n" << endl;
        for (HomebrewInfo homebrew : homebrews)
        {
            string display = format("{}{} (v{}) by {}", selectedHomebrew == i ? "=> ":"   ", homebrew.name, homebrew.version, homebrew.author);
            cout << display << endl;
            i++;
        }

        cout << "\n==== Logs: ====\n" << endl;
        for (const auto& log : logs) {
            cout << log << endl;
        }

        if (state.launch_app && !state.activated)
        {
            if (R_SUCCEEDED(netloader::setNext()))
            {
                break;
            }
            else
            {
                addLog("Failed to launch sended app.");
            }
        }

        consoleUpdate(NULL);
    }

    netloader::signalExit();

    threadWaitForExit(&thread);
    threadClose(&thread);

    nifmExit();
    socketExit();
    consoleExit(NULL);
    return 0;
}
