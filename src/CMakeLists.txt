# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

kconfig_add_kcfg_files(SPECTACLE_SRCS GENERATE_MOC Gui/SettingsDialog/settings.kcfgc)

ecm_qt_declare_logging_category(SPECTACLE_SRCS HEADER spectacle_core_debug.h IDENTIFIER SPECTACLE_CORE_LOG CATEGORY_NAME org.kde.spectacle.core DESCRIPTION "spectacle (core)" EXPORT SPECTACLE)
ecm_qt_declare_logging_category(SPECTACLE_SRCS HEADER spectacle_gui_debug.h IDENTIFIER SPECTACLE_GUI_LOG CATEGORY_NAME org.kde.spectacle.gui DESCRIPTION "spectacle (gui)" EXPORT SPECTACLE)

add_executable(spectacle
    ${SPECTACLE_SRCS}
    Main.cpp
    CaptureModeModel.cpp
    ExportManager.cpp
    SpectacleCore.cpp
    SpectacleDBusAdapter.cpp
    ShortcutActions.cpp
    Gui/CaptureWindow.cpp
    Gui/ExportMenu.cpp
    Gui/HelpMenu.cpp
    Gui/OptionsMenu.cpp
    Gui/SmartSpinBox.cpp
    Gui/Selection.cpp
    Gui/SelectionEditor.cpp
    Gui/SpectacleImageProvider.cpp
    Gui/SpectacleWindow.cpp
    Gui/SpectacleMenu.cpp
    Gui/ViewerWindow.cpp
    Gui/Annotations/AnnotationDocument.cpp
    Gui/Annotations/AnnotationViewport.cpp
    Gui/Annotations/EditAction.cpp
    Gui/Annotations/Utils.cpp
    Gui/SettingsDialog/SaveOptionsPage.cpp
    Gui/SettingsDialog/SettingsDialog.cpp
    Gui/SettingsDialog/GeneralOptionsPage.cpp
    Gui/SettingsDialog/ShortcutsOptionsPage.cpp
    Platforms/PlatformLoader.cpp
    Platforms/Platform.cpp
    Platforms/PlatformNull.cpp
    Platforms/PlatformKWinWayland.cpp
    Platforms/PlatformKWinWayland2.cpp

    ../resources.qrc
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

ki18n_wrap_ui(spectacle Gui/SettingsDialog/GeneralOptions.ui Gui/SettingsDialog/SaveOptions.ui)

target_link_libraries(
    spectacle
        Qt::Concurrent
        Qt::DBus
        Qt::PrintSupport
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickTemplates2
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
