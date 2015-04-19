# KScreenGenie project

project(KScreenGenie)
set(KSG_VERSION_MAJOR 0)
set(KSG_VERSION_MINOR 0)
set(KSG_VERSION_PATCH 95)
set(KSG_VERSION "${KSG_VERSION_MAJOR}.${KSG_VERSION_MINOR}.${KSG_VERSION_PATCH}")

# minimum requirements

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
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
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
include(GenerateExportHeader)
include(FeatureSummary)

find_package(
    Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
        Core
        Quick
        Widgets
        QuickWidgets
        DBus
)
include_directories(${Qt5QuickWidgets_PRIVATE_INCLUDE_DIRS})

find_package(
    KF5 ${KF5_MIN_VERSION} REQUIRED
        CoreAddons
        Declarative
        Config
        I18n
        KIO
        XmlGui
        WidgetsAddons
        WindowSystem
)

# optional components

find_package(KF5Kipi)
if (KF5Kipi_FOUND)
    set(KIPI_FOUND 1)
endif ()

find_package(XCB COMPONENTS XFIXES IMAGE)
if (XCB_FOUND)
    find_package(Qt5X11Extras ${QT_MIN_VERSION} REQUIRED)
    find_package(KF5Screen ${PLASMA_MIN_VERSION} REQUIRED)
endif()

# hand off to subdirectories

add_subdirectory(src)
add_subdirectory(desktop)

# summaries

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
