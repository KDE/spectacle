add_library(spectacle_imageplatform_kwin MODULE)

ecm_qt_declare_logging_category(spectacle_imageplatform_kwin
    HEADER imageplatform_kwin_debug.h
    IDENTIFIER SPECTACLE_IMAGEPLATFORM_KWIN
    CATEGORY_NAME spectacle
    DESCRIPTION "spectacle (image platform KWin)"
    EXPORT SPECTACLE
)

target_sources(spectacle_imageplatform_kwin PRIVATE
    ImagePlatformKWin.cpp
    ImagePlatformKWin.h
)

target_link_libraries(spectacle_imageplatform_kwin PRIVATE
    SpectacleCore
    Qt6::Concurrent
)

install(TARGETS spectacle_imageplatform_kwin DESTINATION ${KDE_INSTALL_PLUGINDIR}/spectacle)
