add_library(spectacle_videoplatform_wayland MODULE)

target_sources(spectacle_videoplatform_wayland PRIVATE
    VideoPlatformWayland.h
    VideoPlatformWayland.cpp
    screencasting.h
    screencasting.cpp
)

if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
    set(private_code_option  "PRIVATE_CODE")
endif()
qt6_generate_wayland_protocol_client_sources(spectacle_videoplatform_wayland FILES
    ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml
    ${private_code_option}
)

target_link_libraries(spectacle_videoplatform_wayland PRIVATE
    SpectacleCore
    Qt6::GuiPrivate
    Qt6::WaylandClient
    Qt6::Concurrent
    K::KPipeWireRecord
)

install(TARGETS spectacle_videoplatform_wayland DESTINATION ${KDE_INSTALL_PLUGINDIR}/spectacle)
