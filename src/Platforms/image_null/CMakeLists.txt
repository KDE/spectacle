add_library(spectacle_imageplatform_null MODULE)

target_sources(spectacle_imageplatform_null PRIVATE
    ImagePlatformNull.cpp
    ImagePlatformNull.h
)

target_link_libraries(spectacle_imageplatform_null PRIVATE
    SpectacleCore
)

install(TARGETS spectacle_imageplatform_null DESTINATION ${KDE_INSTALL_PLUGINDIR}/spectacle)
