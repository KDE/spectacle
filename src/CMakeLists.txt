# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

set(
    SPECTACLE_SRCS_DEFAULT
        Main.cpp
        ExportManager.cpp
        SpectacleCore.cpp
        SpectacleDBusAdapter.cpp
        PlatformBackends/ImageGrabber.cpp
        PlatformBackends/DummyImageGrabber.cpp
        Gui/KSMainWindow.cpp
        Gui/KSWidget.cpp
        Gui/KSImageWidget.cpp
        Gui/KSSaveConfigDialog.cpp
        Gui/KSSendToMenu.cpp
        Gui/ScreenClipper.cpp
        Gui/SmartSpinBox.cpp
)

if(XCB_FOUND)
    set(
        SPECTACLE_SRCS_X11
            PlatformBackends/X11ImageGrabber.cpp
    )
endif()

set(
    SPECTACLE_SRCS_ALL
        ${SPECTACLE_SRCS_DEFAULT}
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
        KF5::CoreAddons
        KF5::DBusAddons
        KF5::WidgetsAddons
        KF5::Notifications
        KF5::ConfigCore
        KF5::I18n
        KF5::KIOWidgets
        KF5::WindowSystem
        KF5::XmlGui
        KDEExperimental::PurposeWidgets
)

if(XCB_FOUND)
    target_link_libraries(
        spectacle
            XCB::XFIXES
            XCB::IMAGE
            XCB::CURSOR
            XCB::UTIL
            Qt5::X11Extras
            KF5::Screen
    )
endif()

install(TARGETS spectacle ${INSTALL_TARGETS_DEFAULT_ARGS})
