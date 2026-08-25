cmake_minimum_required(VERSION 3.20)
project(SumPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_PREFIX_PATH "loaction of /Qt/6.11.1/msvc2022_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
set(CMAKE_AUTOMOC ON)


set(FFMPEG_DIR " loaction of  /ffmpeg-master-latest-win64-gpl-shared")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_DIR}/include")
set(FFMPEG_LIB_DIR "${FFMPEG_DIR}/lib")

set(VCPKG_DIR "D:/DOIN/Dev/vcpkg/installed/x64-windows")
set(LIBASS_INCLUDE_DIR "${VCPKG_DIR}/include")
set(LIBASS_LIB_DIR "${VCPKG_DIR}/lib")
set(LIBASS_BIN_DIR "${VCPKG_DIR}/bin")

add_executable(SumPlayer WIN32
    src/main.cpp
    src/core/app/AppSettings.cpp
    src/core/audio/AudioDecoder.cpp
    src/core/audio/AudioOutput.cpp
    src/core/media/MediaFile.cpp
    src/core/media/MediaProbe.cpp
    src/core/media/ThumbnailGenerator.cpp
    src/core/playback/PlaybackEngine.cpp
    src/core/playback/PlayerState.cpp
    src/core/playlist/Playlist.cpp
    src/core/playlist/PlaylistManager.cpp
    src/core/playlist/PlaylistStore.cpp
    src/ui/dialogs/CreatePlaylistDialog.cpp
    src/ui/screens/HomeScreen.cpp
    src/ui/screens/PlaylistDetailScreen.cpp
    src/ui/screens/SettingsScreen.cpp
    src/ui/widgets/ActionCard.cpp
    src/ui/widgets/ClickableSlider.cpp
    src/ui/widgets/PlaylistCardWidget.cpp
    src/ui/widgets/PlaylistItemWidget.cpp
    src/ui/widgets/VideoWidget.cpp
    src/ui/windows/MainWindow.cpp
    assets/app.rc
    )

add_custom_command(TARGET SumPlayer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${LIBASS_BIN_DIR}
        $<TARGET_FILE_DIR:SumPlayer>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:SumPlayer>/assets
)

target_include_directories(SumPlayer PRIVATE
    src
    ${FFMPEG_INCLUDE_DIR}
    ${LIBASS_INCLUDE_DIR}
    )

target_link_libraries(SumPlayer PRIVATE 
    Qt6::Widgets
    Qt6::Multimedia
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avformat.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
    ${FFMPEG_LIB_DIR}/swscale.lib
    ${FFMPEG_LIB_DIR}/swresample.lib
    ${LIBASS_LIB_DIR}/ass.lib
)

qt_import_plugins(SumPlayer)

