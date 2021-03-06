cmake_minimum_required(VERSION 3.1)

project(akonadi_decsync_resource)

set(KF5_MIN_VERSION "5.38.0")

set(ECM_MIN_VERSION ${KF5_MIN_VERSION})
find_package(ECM ${ECM_MIN_VERSION} CONFIG REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

set(QT_MIN_VERSION "5.11.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED Core DBus Gui Network)

find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)

# If we need a library, add its corresponding /usr/lib/cmake subdirectory here,
# then add it to the target_link_libraries call in src/CMakeLists.txt!
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)

set(AKONADI_MIN_VERSION "5.2")
find_package(KF5Akonadi ${AKONADI_MIN_VERSION} CONFIG REQUIRED)

find_program(XSLTPROC_EXECUTABLE xsltproc DOC "Path to the xsltproc executable")
if (NOT XSLTPROC_EXECUTABLE)
    message(FATAL_ERROR "\nThe command line XSLT processor program 'xsltproc'  could not be found.\nPlease install xsltproc.\n")
endif()

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
