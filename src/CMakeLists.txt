# common - configure file and version definitions

set(KSNAPVERSION ${KSG_VERSION})
#configure_file(config-ksnapshot.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-ksnapshot.h)

#if(XCB_XCB_FOUND AND X11_XCB_FOUND)
#    set(
#        KSnapshot_SRCS_XCB
#            platforms/xcb/pixmaphelper.cpp
#    )
#endif(XCB_XCB_FOUND AND X11_XCB_FOUND)

# target

set(
    KSG_SRCS_DEFAULT
        Main.cpp
        ImageGrabber.cpp
        KScreenGenie.cpp
        KScreenGenieGUI.cpp
        KSGImageProvider.cpp
        SendToActionsPopulator.cpp
)
qt5_add_resources (KSG_SRCS_RESOURCES QmlResources/QmlResources.qrc)

if(X11_FOUND)
    set(
        KSG_SRCS_X11
            X11ImageGrabber.cpp
    )
endif()

#if(KF5Kipi_FOUND)
#    set(
#        KSG_SRCS_KIPI
#            KIPI.cpp
#    )
#endif()

set(
    KSG_SRCS_ALL
        ${KSG_SRCS_DEFAULT}
        ${KSG_SRCS_X11}
        ${KSG_SRCS_RESOURCES}
)

add_executable(
    kscreengenie
        ${KSG_SRCS_ALL}
)

# link libraries

target_link_libraries(
    kscreengenie
        Qt5::Quick
        Qt5::QuickWidgets
        Qt5::DBus
        KF5::CoreAddons
        KF5::Declarative
        KF5::ConfigGui
        KF5::I18n
        KF5::KIOWidgets
        KF5::WindowSystem
        KF5::XmlGui
        ${X11_LIBRARIES}
        ${X11_Xfixes_LIB}
)

if(X11_FOUND)
    target_link_libraries(
        kscreengenie
            Qt5::X11Extras
            KF5::Screen
    )
endif()

#if (X11_XCB_FOUND)
#    target_link_libraries(
#        ksnapshot
#            ${X11_XCB_LIBRARIES}
#    )
#endif ()

#if (XCB_XCB_FOUND)
#    target_link_libraries(
#        ksnapshot
#            ${XCB_XCB_LIBRARY}
#    )
#endif ()

#if (XCB_SHAPE_FOUND)
#    target_link_libraries(
#        ksnapshot
#            ${XCB_SHAPE_LIBRARY}
#    )
#endif ()

#if (XCB_XFIXES_FOUND)
#    target_link_libraries(
#        ksnapshot
#            ${XCB_XFIXES_LIBRARY}
#    )
#endif ()

#if (KF5Kipi_FOUND)
#    target_link_libraries(
#        ksnapshot
#            KF5::Kipi
#    )
#endif ()

install(TARGETS kscreengenie ${INSTALL_TARGETS_DEFAULT_ARGS})
#install(FILES org.kde.ksnapshot.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
