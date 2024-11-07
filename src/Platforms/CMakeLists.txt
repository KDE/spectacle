target_sources(SpectacleCore PRIVATE
    PlatformLoader.cpp
    ImagePlatform.cpp
    VideoPlatform.cpp
)

target_sources(SpectacleCore PUBLIC
    PlatformLoader.h
    ImagePlatform.h
    VideoPlatform.h
)

target_include_directories(SpectacleCore PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

add_subdirectory(image_null)
add_subdirectory(image_kwin)

if (XCB_FOUND)
    add_subdirectory(image_xcb)
endif()

add_subdirectory(video_null)
add_subdirectory(video_wayland)
