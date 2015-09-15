# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

set(
    KAPTURE_SRCS_DEFAULT
        Main.cpp
        KSCore.cpp
        ScreenClipper.cpp
        PlatformBackends/ImageGrabber.cpp
        PlatformBackends/DummyImageGrabber.cpp
        Gui/KSMainWindow.cpp
        Gui/KSWidget.cpp
        Gui/KSImageWidget.cpp
        Gui/KSSaveConfigDialog.cpp
        Gui/KSSendToMenu.cpp
)

if(XCB_FOUND)
    set(
        KAPTURE_SRCS_X11
            PlatformBackends/X11ImageGrabber.cpp
    )
endif()

if(KF5Kipi_FOUND)
    set(
        KAPTURE_SRCS_KIPI
            KipiInterface/KSGKipiInterface.cpp
            KipiInterface/KSGKipiInfoShared.cpp
            KipiInterface/KSGKipiImageCollectionShared.cpp
            KipiInterface/KSGKipiImageCollectionSelector.cpp
    )
endif()

set(
    KAPTURE_SRCS_ALL
        ${KAPTURE_SRCS_DEFAULT}
        ${KAPTURE_SRCS_KIPI}
        ${KAPTURE_SRCS_X11}
)

add_executable(
    kapture
        ${KAPTURE_SRCS_ALL}
)

# link libraries

target_link_libraries(
    kapture
        Qt5::DBus
        Qt5::PrintSupport
        KF5::CoreAddons
        KF5::Notifications
        KF5::ConfigGui
        KF5::I18n
        KF5::KIOWidgets
        KF5::WindowSystem
        KF5::XmlGui
        KF5::WidgetsAddons
)

if(XCB_FOUND)
    target_link_libraries(
        kapture
            XCB::XFIXES
            XCB::IMAGE
            XCB::CURSOR
            XCB::UTIL
            Qt5::X11Extras
            KF5::Screen
    )
endif()

if(KF5Kipi_FOUND)
    target_link_libraries (
        kapture
            KF5::Kipi
    )
endif()

install(TARGETS kapture ${INSTALL_TARGETS_DEFAULT_ARGS})
