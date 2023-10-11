# add the spectacle.upd file
install(FILES spectacle.upd DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})

# add the scripts
add_executable(spectacle-24.02.0-video_format "spectacle-24.02.0-video_format.cpp")
target_link_libraries(spectacle-24.02.0-video_format
    KF6::ConfigCore
    KF6::XmlGui
)

add_executable(spectacle-24.02.0-keep_old_save_location "spectacle-24.02.0-keep_old_save_location.cpp")
target_link_libraries(spectacle-24.02.0-keep_old_save_location
    KF6::ConfigCore
    KF6::XmlGui
)

# install C++ scripts to kconf_update_bin
install(
    TARGETS
        spectacle-24.02.0-video_format
        spectacle-24.02.0-keep_old_save_location
    DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin
)
