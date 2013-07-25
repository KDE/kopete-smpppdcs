project(kopete-smpppdcs)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
find_package(Kopete REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${KDE4_INCLUDES} ${KOPETE_INCLUDE_DIR} ${KOPETE_INCLUDE_DIR}/kopete)

add_subdirectory(plugins/smpppdcs)
