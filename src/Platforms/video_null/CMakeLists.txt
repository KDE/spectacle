add_library(spectacle_videoplatform_null MODULE)

target_sources(spectacle_videoplatform_null PRIVATE
    VideoPlatformNull.cpp
    VideoPlatformNull.h
)

target_link_libraries(spectacle_videoplatform_null PRIVATE
    SpectacleCore
)

install(TARGETS spectacle_videoplatform_null DESTINATION ${KDE_INSTALL_PLUGINDIR}/spectacle)
