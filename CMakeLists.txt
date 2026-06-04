cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
# Managed by release script
set(PROJECT_VERSION "6.7.80")
set(SPECTACLE_VERSION ${PROJECT_VERSION})

# Spectacle project
project(Spectacle VERSION ${SPECTACLE_VERSION})

# minimum requirements
set(PROJECT_DEP_VERSION "6.6.90")
set(QT_MIN_VERSION "6.10.0")
set(KF6_MIN_VERSION "6.26.0")

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

option(WITH_X11 "Enable X11 support" ON)

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
find_package(Qt6GuiPrivate ${QT_MIN_VERSION} REQUIRED NO_MODULE)

qt_policy(SET QTP0001 NEW)

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
if (NOT TARGET KF6::PrisonScanner ) 
    message(SEND_ERROR "Prison built without required PrisonScanner component")
endif()

find_package(Wayland REQUIRED COMPONENTS Client)
find_package(PlasmaWaylandProtocols REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(KPipeWire)
find_package(OpenCV 4.7 REQUIRED core imgproc)
find_package(PkgConfig REQUIRED)

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

pkg_check_modules(TESSERACT REQUIRED IMPORTED_TARGET tesseract)

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

if(WITH_X11)
    find_package(XCB REQUIRED COMPONENTS XFIXES IMAGE UTIL CURSOR RANDR)
endif()

# setup handling of deprecated Qt & KF API

ecm_set_disabled_deprecation_versions(QT 6.11.0  KF 6.26.0)

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
