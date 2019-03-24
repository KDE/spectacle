include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ecm_add_test(FilenameTest.cpp
        ../src/ExportManager.cpp ../src/SpectacleConfig.cpp ../src/PlatformBackends/ImageGrabber.cpp
    TEST_NAME "filename_test"
    LINK_LIBRARIES  Qt5::Test 
        Qt5::PrintSupport KF5::I18n KF5::ConfigCore KF5::KIOCore KF5::WindowSystem
)
