# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)
set(CMAKE_AUTORCC 1)

# target

if(XCB_FOUND)
    set(
        SPECTACLE_SRCS_PLATFORM_XCB
            Platforms/PlatformXcb.cpp
    )
endif()

set(
    SPECTACLE_SRCS_PLATFORM
        Platforms/PlatformLoader.cpp
        Platforms/Platform.cpp
        Platforms/PlatformNull.cpp
        Platforms/PlatformKWinWayland.cpp
        ${SPECTACLE_SRCS_PLATFORM_XCB}
)

set(
    SPECTACLE_SRCS_DEFAULT
        Main.cpp
        ExportManager.cpp
        SpectacleCore.cpp
        SpectacleDBusAdapter.cpp
        ShortcutActions.cpp
        ${SPECTACLE_SRCS_PLATFORM}
        Gui/KSMainWindow.cpp
        Gui/KSWidget.cpp
        Gui/KSImageWidget.cpp
        Gui/ExportMenu.cpp
        Gui/ProgressButton.cpp
        Gui/SmartSpinBox.cpp
        Gui/SettingsDialog/SaveOptionsPage.cpp
        Gui/SettingsDialog/SettingsDialog.cpp
        Gui/SettingsDialog/GeneralOptionsPage.cpp
        Gui/SettingsDialog/ShortcutsOptionsPage.cpp
        QuickEditor/QuickEditor.cpp
)

kconfig_add_kcfg_files(SPECTACLE_SRCS_DEFAULT Gui/SettingsDialog/settings.kcfgc)

ecm_qt_declare_logging_category(SPECTACLE_SRCS_DEFAULT HEADER spectacle_core_debug.h IDENTIFIER SPECTACLE_CORE_LOG CATEGORY_NAME org.kde.spectacle.core)
ecm_qt_declare_logging_category(SPECTACLE_SRCS_DEFAULT HEADER spectacle_gui_debug.h IDENTIFIER SPECTACLE_GUI_LOG CATEGORY_NAME org.kde.spectacle.gui)

if(KIPI_FOUND)
    set(
        SPECTACLE_SRCS_KIPI
            KipiInterface/KSGKipiInterface.cpp
            KipiInterface/KSGKipiInfoShared.cpp
            KipiInterface/KSGKipiImageCollectionShared.cpp
            KipiInterface/KSGKipiImageCollectionSelector.cpp
    )
endif()

set(
    SPECTACLE_SRCS_ALL
        ${SPECTACLE_SRCS_DEFAULT}
        ${SPECTACLE_SRCS_KIPI}
)

add_executable(
    spectacle
        ${SPECTACLE_SRCS_ALL}
)

ki18n_wrap_ui(spectacle Gui/SettingsDialog/GeneralOptions.ui Gui/SettingsDialog/SaveOptions.ui)

# link libraries

target_link_libraries(
    spectacle
        Qt5::DBus
        Qt5::PrintSupport
        KF5::CoreAddons
        KF5::DBusAddons
        KF5::WidgetsAddons
        KF5::Notifications
        KF5::ConfigCore
        KF5::I18n
        KF5::KIOWidgets
        KF5::WindowSystem
        KF5::NewStuff
        KF5::GlobalAccel
        KF5::XmlGui
        KF5::WaylandClient
)

if(XCB_FOUND)
    target_link_libraries(
        spectacle
            XCB::XFIXES
            XCB::IMAGE
            XCB::CURSOR
            XCB::UTIL
            Qt5::X11Extras
    )
endif()

if(KIPI_FOUND)
    target_link_libraries (
        spectacle
            KF5::Kipi
    )
endif()

if(PURPOSE_FOUND)
    target_link_libraries (
        spectacle
            KF5::PurposeWidgets
    )
endif()

if(KIMAGEANNOTATOR_FOUND)
    target_link_libraries(spectacle kImageAnnotator::kImageAnnotator)
endif()

install(TARGETS spectacle ${INSTALL_TARGETS_DEFAULT_ARGS})
