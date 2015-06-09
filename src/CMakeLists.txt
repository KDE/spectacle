# common - configure file and version definitions

configure_file(Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h)

# target

set(
    KSG_SRCS_DEFAULT
        Main.cpp
        ImageGrabber.cpp
        KScreenGenie.cpp
        ScreenClipper.cpp
        Gui/KSMainWindow.cpp
        Gui/KSWidget.cpp
        Gui/KSImageWidget.cpp
        Gui/KSSendToMenu.cpp
)

if(XCB_FOUND)
    set(
        KSG_SRCS_X11
            X11ImageGrabber.cpp
    )
endif()

if(KF5Kipi_FOUND)
    set(
        KSG_SRCS_KIPI
            KipiInterface/KSGKipiInterface.cpp
            KipiInterface/KSGKipiInfoShared.cpp
            KipiInterface/KSGKipiImageCollectionShared.cpp
            KipiInterface/KSGKipiImageCollectionSelector.cpp
    )
endif()

set(
    KSG_SRCS_ALL
        ${KSG_SRCS_DEFAULT}
        ${KSG_SRCS_KIPI}
        ${KSG_SRCS_X11}
)

add_executable(
    kscreengenie
        ${KSG_SRCS_ALL}
)

# link libraries

target_link_libraries(
    kscreengenie
        Qt5::Quick
        Qt5::DBus
        Qt5::PrintSupport
        KF5::CoreAddons
        KF5::Declarative
        KF5::ConfigGui
        KF5::I18n
        KF5::KIOWidgets
        KF5::WindowSystem
        KF5::XmlGui
        KF5::WidgetsAddons
)

if(XCB_FOUND)
    target_link_libraries(
        kscreengenie
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
        kscreengenie
            KF5::Kipi
    )
endif()

install(TARGETS kscreengenie ${INSTALL_TARGETS_DEFAULT_ARGS})
