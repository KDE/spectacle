# Spectacle project

project(Spectacle)

# KDE Application Version, managed by release script
set(KDE_APPLICATIONS_VERSION_MAJOR "15")
set(KDE_APPLICATIONS_VERSION_MINOR "12")
set(KDE_APPLICATIONS_VERSION_MICRO "0")
set(KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")
set(SPECTACLE_VERSION ${KDE_APPLICATIONS_VERSION})

# minimum requirements

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
if (POLICY CMP0063)
   cmake_policy(SET CMP0063 NEW)
endif()

set(QT_MIN_VERSION     "5.4.0")
set(KF5_MIN_VERSION    "5.6.0")
set(PLASMA_MIN_VERSION "5.2.0")

find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(
    CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
        ${CMAKE_MODULE_PATH}
        ${ECM_MODULE_PATH}
        ${ECM_KDE_MODULE_DIR}
)

# set up kf5

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(FeatureSummary)

find_package(
    Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
        Core
        Widgets
        DBus
        PrintSupport
)

find_package(
    KF5 ${KF5_MIN_VERSION} REQUIRED
        CoreAddons
        WidgetsAddons
        DBusAddons
        Notifications
        Config
        I18n
        KIO
        XmlGui
        WindowSystem
        DocTools
)

# optional components

find_package(KF5Kipi)
if (KF5Kipi_FOUND)
    set(KIPI_FOUND 1)
endif ()

find_package(XCB COMPONENTS XFIXES IMAGE UTIL CURSOR)
if (XCB_FOUND)
    find_package(Qt5X11Extras ${QT_MIN_VERSION} REQUIRED)
    find_package(KF5Screen ${PLASMA_MIN_VERSION} REQUIRED)
endif()

# fail build if none of the platform backends can be found

if (!XCB_FOUND)
    message(FATAL_ERROR "No suitable backend platform was found. Currenty supported platforms are: XCB")
endif()

# hand off to subdirectories

add_subdirectory(src)
add_subdirectory(dbus)
add_subdirectory(desktop)
add_subdirectory(icons)
add_subdirectory(doc)

# summaries

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
