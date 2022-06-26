# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

kconfig_add_kcfg_files(SPECTACLE_SRCS Gui/SettingsDialog/settings.kcfgc)

ecm_qt_declare_logging_category(SPECTACLE_SRCS HEADER spectacle_core_debug.h IDENTIFIER SPECTACLE_CORE_LOG CATEGORY_NAME org.kde.spectacle.core DESCRIPTION "spectacle (core)" EXPORT SPECTACLE)
ecm_qt_declare_logging_category(SPECTACLE_SRCS HEADER spectacle_gui_debug.h IDENTIFIER SPECTACLE_GUI_LOG CATEGORY_NAME org.kde.spectacle.gui DESCRIPTION "spectacle (gui)" EXPORT SPECTACLE)

add_executable(spectacle
    ${SPECTACLE_SRCS}
    Main.cpp
    ExportManager.cpp
    SpectacleCore.cpp
    SpectacleDBusAdapter.cpp
    ShortcutActions.cpp
    Gui/KSMainWindow.cpp
    Gui/KSWidget.cpp
    Gui/KSImageWidget.cpp
    Gui/ExportMenu.cpp
    Gui/ProgressButton.cpp
    Gui/CaptureAreaComboBox.cpp
    Gui/SmartSpinBox.cpp
    Gui/SettingsDialog/SaveOptionsPage.cpp
    Gui/SettingsDialog/SettingsDialog.cpp
    Gui/SettingsDialog/GeneralOptionsPage.cpp
    Gui/SettingsDialog/ShortcutsOptionsPage.cpp
    QuickEditor/QuickEditor.cpp
    Platforms/PlatformLoader.cpp
    Platforms/Platform.cpp
    Platforms/PlatformNull.cpp
    Platforms/PlatformKWinWayland.cpp
    Platforms/PlatformKWinWayland2.cpp

    ExportManager.h
    SpectacleCore.h
    SpectacleDBusAdapter.h
    ShortcutActions.h
    Gui/KSMainWindow.h
    Gui/KSWidget.h
    Gui/KSImageWidget.h
    Gui/ExportMenu.h
    Gui/ProgressButton.h
    Gui/SmartSpinBox.h
    Gui/SettingsDialog/SaveOptionsPage.h
    Gui/SettingsDialog/SettingsDialog.h
    Gui/SettingsDialog/GeneralOptionsPage.h
    Gui/SettingsDialog/ShortcutsOptionsPage.h
    QuickEditor/QuickEditor.h
    Platforms/PlatformLoader.h
    Platforms/Platform.h
    Platforms/PlatformNull.h
    Platforms/PlatformKWinWayland.h
    Platforms/PlatformKWinWayland2.h
    )

if(XCB_FOUND)
    target_sources(spectacle PRIVATE Platforms/PlatformXcb.cpp Platforms/PlatformXcb.h)

    target_link_libraries(spectacle
        XCB::XFIXES
        XCB::IMAGE
        XCB::CURSOR
        XCB::UTIL
    )
    if (QT_MAJOR_VERSION STREQUAL "5")
        target_link_libraries(spectacle Qt::X11Extras)
    else()
        target_link_libraries(spectacle Qt6::GuiPrivate) # Gui/private/qtx11extras_p.h
    endif()
endif()

if(PURPOSE_FOUND)
    target_link_libraries(spectacle KF5::PurposeWidgets)
endif()

if(KIMAGEANNOTATOR_FOUND)
    target_link_libraries(spectacle kImageAnnotator::kImageAnnotator)
endif()

ki18n_wrap_ui(spectacle Gui/SettingsDialog/GeneralOptions.ui Gui/SettingsDialog/SaveOptions.ui)

target_link_libraries(
    spectacle
        Qt::Concurrent
        Qt::DBus
        Qt::PrintSupport
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
        KF5::GuiAddons
)

install(TARGETS spectacle ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
