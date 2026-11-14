#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
# SPDX-License-Identifier: MIT

import subprocess
import unittest
from typing import Final

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy

APP_COMMAND: Final = "spectacle --testmode"
CONFIG_FILE: Final = "~/.qttest/config/spectaclerc"


def read_config(group: str, key: str) -> subprocess.CompletedProcess:
    return subprocess.run(["kreadconfig6", "--file", CONFIG_FILE, "--group", group, "--key", key], check=True, capture_output=True)


def write_config(group: str, key: str, value: str) -> subprocess.CompletedProcess:
    return subprocess.run(["kwriteconfig6", "--file", CONFIG_FILE, "--group", group, "--key", key, value], check=True, capture_output=True)


def get_driver(app_command: str = APP_COMMAND) -> webdriver.Remote:
    options = AppiumOptions()
    options.set_capability("app", app_command)
    driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
    driver.implicitly_wait = 10
    return driver


class LastCaptureModeTest(unittest.TestCase):
    # def setUpClass(self) -> None:
    # def setUp(self) -> None:
    # def tearDown(self) -> None:
    # def tearDownClass(self) -> None:


    def test_initial_config(self) -> None:
        # Enable "Use last-used capture mode" setting.
        write_config("General", "launchAction", "UseLastUsedCapturemode")
        # Set captureMode value to something other than the mode we will use.
        write_config("GuiConfig", "captureMode", "4") # ActiveWindow
        # Verify that the expected initial values have been set.
        self.assertEqual(read_config("General", "launchAction").stdout.rstrip(), b"UseLastUsedCapturemode")
        self.assertEqual(read_config("GuiConfig", "captureMode").stdout.rstrip(), b"4") # ActiveWindow


    def test_set_capture_mode(self) -> None:
        self.driver = get_driver(app_command=f"{APP_COMMAND} --launchonly")
        self.driver.find_element(by=AppiumBy.NAME, value="Rectangular Region").click()
        self.driver.find_element(by=AppiumBy.NAME, value="Accept").click()
        self.driver.quit()
        self.assertEqual(read_config("GuiConfig", "captureMode").stdout.rstrip(), b"0") # RectangularRegion


    def test_use_capture_mode(self) -> None:
        self.driver = get_driver()
        # Find an element only visible in receptacle capture windows to verify
        # that the correct capture mode was selected.
        elements = self.driver.find_elements(by=AppiumBy.NAME, value="Accept")
        self.assertGreater(len(elements), 0)
        self.driver.quit()


    def test_next_capture_mode(self) -> None:
        self.driver = get_driver()
        self.driver.find_element(by=AppiumBy.NAME, value="Accept").click()
        self.driver.find_element(by=AppiumBy.NAME, value="New Screenshot").click()
        self.driver.find_element(by=AppiumBy.NAME, value="Active Window").click()
        self.driver.quit()
        self.assertEqual(read_config("GuiConfig", "captureMode").stdout.rstrip(), b"4") # ActiveWindow


if __name__ == '__main__':
    unittest.main()
