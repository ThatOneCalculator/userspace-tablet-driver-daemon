/*
userspace_tablet_driver_daemon
Copyright (C) 2021 - Aren Villanueva <https://github.com/kurikaesu/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include "deco_03.h"

deco_03::deco_03() {
    productIds.push_back(0x0096);

    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_6; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string deco_03::getProductName(int productId) {
    if (productId == 0x0096) {
        return "XP-Pen Deco 03";
    }

    return "Unknown XP-Pen Device";
}

void deco_03::setConfig(nlohmann::json config) {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});

        auto addToButtonMap = [&config](int key, int eventType, std::vector<int> codes) {
            std::string evstring = std::to_string(eventType);
            config["mapping"]["buttons"][std::to_string(key)][evstring] = codes;
        };

        auto addToDialMap = [&config](int dial, int value, int eventType, std::vector<int> codes) {
            std::string strvalue = std::to_string(value);
            std::string evstring = std::to_string(eventType);
            config["mapping"]["dials"][std::to_string(dial)][strvalue][evstring] = codes;
        };

        addToButtonMap(BTN_0, EV_KEY, {KEY_B});
        addToButtonMap(BTN_1, EV_KEY, {KEY_E});
        addToButtonMap(BTN_2, EV_KEY, {KEY_SPACE});
        addToButtonMap(BTN_3, EV_KEY, {KEY_LEFTALT});
        addToButtonMap(BTN_4, EV_KEY, {KEY_V});
        addToButtonMap(BTN_5, EV_KEY, {KEY_LEFTCTRL, KEY_S});

        addToDialMap(REL_WHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
        addToDialMap(REL_WHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

bool deco_03::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleFrameEvent(handle, data, dataLen);
            break;

        case 0x03:
            handleNonUnifiedDialEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void deco_03::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        long button = data[2];
        // Only 8 buttons on this device
        long position = ffsl(data[2]);

        if (button != 0) {
            handlePadButtonPressed(handle, position);
        } else {
            handlePadButtonUnpressed(handle);
        }

        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}

void deco_03::handleNonUnifiedDialEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] == 0x01) {
        std::bitset<sizeof(data)> dialBits(data[2]);

        // Take the dial
        short dialValue = 0;
        if (dialBits.test(0)) {
            dialValue = 1;
        } else if (dialBits.test(1)) {
            dialValue = -1;
        }

        if (dialValue != 0) {
            handleDialEvent(handle, REL_WHEEL, dialValue);
        }
    }
}
