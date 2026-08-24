#pragma once

#include <QMainWindow>
#include <QRect>
#include <QString>

#include <vector>

class QLabel;
class QKeyEvent;
class QMouseEvent;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTimer;
class QWidget;

namespace SumPlayer
{

class AppSettings;
class AudioOutput;
class ClickableSlider;
class HomeScreen;
class PlaybackEngine;
class PlaylistDetailScreen;
class PlaylistManager;
class SettingsScreen;
class VideoWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void onOpenFileClicked();
    void onRenderFrame();
    void onPlayPauseClicked();
    void onVolumeChanged(int value);
    void onProgressUpdateTick();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;



private:
    void updateMetadataDisplay(const QString& filepath);
    void stopPlayback();
    void onHomeOpenFileRequested();
    void onPlaylistOpenRequested(int playlistIndex);
    void onPlaylistItemPlayRequested(int itemIndex, const QString& filepath);
    void navigateTo(QWidget* Screen);
    void navigateBack();
    void onBackClicked();
    void onAudioTrackSelected(int streamIndex);
    void onSubtitleTrackSelected(int streamIndex);
    void onLoadSubtitleFileClicked();
    void playFromPlaylist(int playlistIndex, int itemIndex);
    void onNextClicked();
    void onPreviousClicked();
    
    void showControlsOverlay();
    void hideControlsOverlay();
    void toggleFullscreen();

    void onVolumeIconClicked();
    QPushButton* m_volumeIconButton;
    int m_lastNonZeroVolume = 200;
    QPushButton* m_fullscreenButton;


    int m_currentPlaylistIndex = -1;
    int m_currentItemIndex = -1;

    

    QTimer* m_hideControlsTimer;
    QPushButton* m_nextButton;
    QPushButton* m_previousButton;
    QPushButton* m_aspectRatioButton;
    QPushButton* m_loopButton;
    QPushButton* m_audioTrackButton;
    QPushButton* m_subtitleTrackButton;
    QPushButton*  m_playPauseButton;
    QStackedWidget* m_stack;
    HomeScreen*   m_homeScreen;
    QWidget*   m_playerScreen;
    QLabel*         m_metadataLabel;
    QSlider*      m_volumeSlider;
    ClickableSlider* m_progressSlider;
    QTimer*  m_progressTimer;
    VideoWidget*    m_videoWidget;
    PlaybackEngine* m_engine;
    PlaylistDetailScreen* m_playlistDetailScreen;
    std::vector<QWidget*> m_navigationHistory;
    QPushButton* m_backButton;
    PlaylistManager* m_playlistManager;
    AudioOutput*    m_audioOutput;
    QTimer*         m_renderTimer;
    AppSettings* m_appSettings;
    SettingsScreen* m_settingsScreen;
    int             m_frameCount;
    bool m_userIsSeeking;

    bool m_isFullscreen = false;

    QRect m_normalGeometry;
    

    QLabel* m_titleLabel;
    QLabel* m_currentTimeLabel;
    QLabel* m_totalTimeLabel;
    QWidget* m_topBar;
    QWidget* m_bottomBar;
    QWidget* m_controlsOverlay;
    QString m_lastUpdatedFilename;


};

}
