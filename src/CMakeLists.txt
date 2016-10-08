# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)
set(CMAKE_AUTORCC 1)

# target

set(
    SPECTACLE_SRCS_DEFAULT
        Main.cpp
        ExportManager.cpp
        SpectacleCore.cpp
        SpectacleConfig.cpp
        SpectacleDBusAdapter.cpp
        PlatformBackends/ImageGrabber.cpp
        PlatformBackends/DummyImageGrabber.cpp
        Gui/KSMainWindow.cpp
        Gui/KSWidget.cpp
        Gui/KSImageWidget.cpp
        Gui/ExportMenu.cpp
        Gui/SmartSpinBox.cpp
        Gui/SettingsDialog/SettingsDialog.cpp
        Gui/SettingsDialog/SettingsPage.cpp
        Gui/SettingsDialog/SaveOptionsPage.cpp
        Gui/SettingsDialog/GeneralOptionsPage.cpp
        QuickEditor/QuickEditor.cpp
        QuickEditor/QmlResources.qrc
)

if(XCB_FOUND)
    set(
        SPECTACLE_SRCS_X11
            PlatformBackends/X11ImageGrabber.cpp
    )
endif()

if(KF5Kipi_FOUND)
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
        ${SPECTACLE_SRCS_X11}
)

add_executable(
    spectacle
        ${SPECTACLE_SRCS_ALL}
)

# link libraries

target_link_libraries(
    spectacle
        Qt5::DBus
        Qt5::PrintSupport
        Qt5::Quick
        KF5::CoreAddons
        KF5::DBusAddons
        KF5::WidgetsAddons
        KF5::Notifications
        KF5::ConfigCore
        KF5::I18n
        KF5::KIOWidgets
        KF5::WindowSystem
        KF5::XmlGui
        KF5::Declarative
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

if(KF5Kipi_FOUND)
    target_link_libraries (
        spectacle
            KF5::Kipi
    )
endif()

if(KDEExperimentalPurpose_FOUND)
    target_link_libraries (
        spectacle
            KDEExperimental::PurposeWidgets
    )
endif()

install(TARGETS spectacle ${INSTALL_TARGETS_DEFAULT_ARGS})
