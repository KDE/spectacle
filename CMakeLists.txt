cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
# Managed by release script
set(PROJECT_VERSION "6.5.80")
set(SPECTACLE_VERSION ${PROJECT_VERSION})

# Spectacle project
project(Spectacle VERSION ${SPECTACLE_VERSION})

# minimum requirements
set(PROJECT_DEP_VERSION "6.4.90")
set(QT_MIN_VERSION "6.9.0")
set(KF6_MIN_VERSION "6.18.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# set up standard kde build settings

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

# Set C++ standard separately from KDECompilerSettings since that is set to an older C++ standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# used cmake macros

include(ECMInstallIcons)
include(ECMSetupVersion)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)
include(ECMFindQmlModule)

# find dependencies

find_package(
    Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED
        Core
        Concurrent
        Qml
        Quick
        QuickControls2
        QuickTemplates2
        Widgets
        DBus
        PrintSupport
        Test
        WaylandClient
        Multimedia
)
qt_policy(SET QTP0001 NEW)

if(Qt6_VERSION VERSION_EQUAL 6.8.0)
    # https://bugreports.qt.io/browse/QTBUG-129797
    message(NOTICE "Qt6_VERSION: ${Qt6_VERSION} detected. Enabling CAST_FROM_ASCII")
    remove_definitions(-DQT_NO_CAST_FROM_ASCII)
endif()

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
endif()

find_package(
    KF6 ${KF6_MIN_VERSION} REQUIRED
        CoreAddons
        WidgetsAddons
        DBusAddons
        Notifications
        Config
        I18n
        KIO
        WindowSystem
        GlobalAccel
        XmlGui
        GuiAddons
        KirigamiPlatform
        StatusNotifierItem
        Prison
        Crash
)

find_package(Wayland REQUIRED COMPONENTS Client)
find_package(PlasmaWaylandProtocols REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(KPipeWire)
find_package(OpenCV 4.7 REQUIRED core imgproc)

# Find Tesseract for OCR functionality
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(TESSERACT tesseract)

    if(TESSERACT_FOUND)
        # Test if Tesseract has usable language packs
        try_run(
            TESSERACT_TEST_RUN_RESULT
            TESSERACT_TEST_COMPILE_RESULT
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/tesseract_test.cpp
            LINK_LIBRARIES ${TESSERACT_LIBRARIES}
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${TESSERACT_INCLUDE_DIRS}"
            COMPILE_OUTPUT_VARIABLE TESSERACT_COMPILE_OUTPUT
            RUN_OUTPUT_VARIABLE TESSERACT_RUN_OUTPUT
        )

        if(TESSERACT_TEST_COMPILE_RESULT AND TESSERACT_TEST_RUN_RESULT EQUAL 0)
            message(STATUS "Tesseract OCR support enabled")
            message(STATUS "${TESSERACT_RUN_OUTPUT}")
            set(HAVE_TESSERACT_OCR TRUE)
        else()
            message(WARNING "Tesseract library found but no usable language packs detected")
            message(WARNING "${TESSERACT_RUN_OUTPUT}")
            message(WARNING "OCR functionality will be disabled. Install language data packages (e.g., tesseract-ocr-eng)")
            set(HAVE_TESSERACT_OCR FALSE)
        endif()
    else()
        message(STATUS "Tesseract not found - OCR functionality disabled")
        set(HAVE_TESSERACT_OCR FALSE)
    endif()

    set_package_properties(TESSERACT PROPERTIES
        DESCRIPTION "OCR engine for text recognition in screenshots"
        TYPE OPTIONAL
        PURPOSE "Enables optical character recognition functionality"
    )
else()
    message(STATUS "PkgConfig not found - Tesseract detection disabled")
    set(HAVE_TESSERACT_OCR FALSE)
endif()

set_package_properties(KPipeWire PROPERTIES DESCRIPTION
    "Used to record pipewire streams into a file"
    TYPE REQUIRED
)

ecm_find_qmlmodule(org.kde.kquickimageeditor)
find_package(KQuickImageEditor COMPONENTS)
set_package_properties(KQuickImageEditor PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Simple image editor for QtQuick applications"
    URL "https://invent.kde.org/libraries/kquickimageeditor/"
    PURPOSE "Add image editing capability to the screenshots"
)

# optional components
find_package(KF6DocTools ${KF6_MIN_VERSION})
set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
find_package(KF6Purpose)
if (KF6Purpose_FOUND)
    set(PURPOSE_FOUND 1)
endif()

find_package(XCB COMPONENTS XFIXES IMAGE UTIL CURSOR RANDR)
set(XCB_COMPONENTS_ERRORS FALSE)
set(XCB_COMPONENTS_FOUND TRUE)
if(NOT XCB_XFIXES_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-XFIXES ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()
if(NOT XCB_IMAGE_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-IMAGE ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()
if(NOT XCB_UTIL_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-UTIL ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()
if(NOT XCB_CURSOR_FOUND)
	set(XCB_COMPONENTS_ERRORS "${XCB_COMPONENTS_ERRORS} XCB-CURSOR ")
	set(XCB_COMPONENTS_FOUND FALSE)
endif()

# fail build if none of the platform backends can be found
if (NOT XCB_FOUND OR NOT XCB_COMPONENTS_FOUND)
    message(FATAL_ERROR "No suitable backend platform was found. Currently supported platforms are: XCB Components Required: ${XCB_COMPONENTS_ERRORS}")
endif()

# setup handling of deprecated Qt & KF API

ecm_set_disabled_deprecation_versions(QT 6.7  KF 6.5.0)

# hand off to subdirectories

add_subdirectory(src)
add_subdirectory(dbus)
add_subdirectory(desktop)
add_subdirectory(icons)
add_subdirectory(kconf_update)
add_subdirectory(tests)

if (KF6DocTools_FOUND)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()
ki18n_install(po)

ecm_qt_install_logging_categories(EXPORT SPECTACLE FILE spectacle.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})

# summaries

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
