#include "ui/windows/MainWindow.h"

#include "core/app/AppSettings.h"
#include "core/app/IconPath.h"
#include "core/audio/AudioOutput.h"
#include "core/media/MediaProbe.h"
#include "core/playback/PlaybackEngine.h"
#include "core/playlist/PlaylistManager.h"
#include "core/playlist/PlaylistStore.h"
#include "ui/screens/HomeScreen.h"
#include "ui/screens/PlaylistDetailScreen.h"
#include "ui/screens/SettingsScreen.h"
#include "ui/widgets/ClickableSlider.h"
#include "ui/widgets/VideoWidget.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRect>
#include <QScreen>
#include <QSlider>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <chrono>
#include <iostream>

namespace SumPlayer
{

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_metadataLabel(nullptr)
    , m_videoWidget(nullptr)
    , m_engine(nullptr)
    , m_audioOutput(nullptr)
    , m_renderTimer(nullptr)
    , m_frameCount(0)
{
    setWindowTitle("SUM PLAYER");
    resize(1280, 720);
    setMinimumSize(640, 360);
    setStyleSheet("QMainWindow { background-color: #111111; }");
    setWindowIcon(QIcon(QDir(QCoreApplication::applicationDirPath()).filePath("assets/icon.ico")));

    static const QString kMenuStyle =
    "QMenu {"
    "  background-color: rgba(30, 30, 30, 245);"
    "  border: 1px solid rgba(255, 255, 255, 25);"
    "  border-radius: 12px;"
    "  padding: 6px 4px;"
    "}"
    "QMenu::item {"
    "  padding: 8px 16px 8px 10px;"
    "  border-radius: 6px;"
    "  font-size: 13px;"
    "  color: rgba(255, 255, 255, 230);"
    "  min-width: 160px;"
    "  icon-size: 17px;"
    "}"
    "QMenu::item:selected {"
    "  background-color: rgba(255, 255, 255, 30);"
    "  color: #ffffff;"
    "}"
    "QMenu::item:disabled {"
    "  color: rgba(255, 255, 255, 70);"
    "}"
    "QMenu::separator {"
    "  height: 1px;"
    "  background: rgba(255, 255, 255, 22);"
    "  margin: 6px 10px;"
    "}"
    "QMenu::icon {"
    "  padding-left: 6px;"
    "  padding-right: 4px;"
    "}"
    "QMenu::indicator {"
    "  width: 16px;"
    "  height: 16px;"
    "  margin-left: 6px;"
    "}"
    "QMenu::right-arrow {"
    "  width: 9px;"
    "  height: 9px;"
    "}";

    m_playerScreen = new QWidget(this);
    m_playerScreen->setStyleSheet("background-color: #000000;");

    QStackedLayout* stackedLayout = new QStackedLayout(m_playerScreen);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);
    stackedLayout->setContentsMargins(0, 0, 0, 0);

    m_playlistManager = new PlaylistManager();
    PlaylistStore::load(*m_playlistManager);

    m_homeScreen = new HomeScreen(m_playlistManager, this);
    connect(m_homeScreen, &HomeScreen::openFileRequested,
            this,       &MainWindow::onHomeOpenFileRequested);

    connect(m_homeScreen, &HomeScreen::playlistOpenRequested,
            this,    &MainWindow::onPlaylistOpenRequested);
    connect(m_homeScreen, &HomeScreen::playlistsChanged, this, [this]()
    {
        PlaylistStore::save(*m_playlistManager);
    });

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_homeScreen);
    m_stack->addWidget(m_playerScreen);
    setCentralWidget(m_stack);

    m_appSettings = new AppSettings();

    m_settingsScreen = new SettingsScreen(m_appSettings, this);
    m_stack->addWidget(m_settingsScreen);

    connect(m_settingsScreen, &SettingsScreen::backRequested, this, &MainWindow::navigateBack);

    connect(m_settingsScreen, &SettingsScreen::subtitleFontScaleChanged, this, [this](double scale)
    {
        m_engine->setSubtitleFontScale(scale);
    });

    m_playlistDetailScreen = new PlaylistDetailScreen(m_playlistManager, this);
    m_stack->addWidget(m_playlistDetailScreen);

    connect(m_playlistDetailScreen, &PlaylistDetailScreen::playlistsChanged, this, [this]()
    {
        PlaylistStore::save(*m_playlistManager);
    });
    connect(m_playlistDetailScreen, &PlaylistDetailScreen::backRequested,
            this, &MainWindow::navigateBack);

    connect(m_playlistDetailScreen, &PlaylistDetailScreen::playRequested,
            this, &MainWindow::onPlaylistItemPlayRequested);

    setCentralWidget(m_stack);

    m_stack->setCurrentWidget(m_homeScreen);

    m_videoWidget = new VideoWidget(this);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedLayout->addWidget(m_videoWidget);

    m_controlsOverlay = new QWidget(m_playerScreen);
    m_controlsOverlay->setAttribute(Qt::WA_NoSystemBackground);
    m_controlsOverlay->setStyleSheet("background: transparent;");

    QVBoxLayout* overlayLayout = new QVBoxLayout(m_controlsOverlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);

    m_topBar = new QWidget(m_controlsOverlay);
    m_topBar->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        " stop:0 rgba(0,0,0,180), stop:1 rgba(0,0,0,0));"
    );

    QHBoxLayout* topBarLayout = new QHBoxLayout(m_topBar);
    topBarLayout->setContentsMargins(25, 20, 25, 40);

    m_backButton = new QPushButton(m_topBar);
    m_backButton->setIcon(QIcon(iconPath("back.svg")));
    m_backButton->setIconSize(QSize(22, 22));
    m_backButton->setFixedSize(36, 36);
    m_backButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );

    m_titleLabel = new QLabel("File Title", m_topBar);
    m_titleLabel->setStyleSheet("color:rgba(255, 255, 255, 0.51); font-size: 36px; font-weight: bold; background: transparent;");

    QVBoxLayout* topTextColumn = new QVBoxLayout();
    topTextColumn->addWidget(m_backButton, 0, Qt::AlignLeft);
    topTextColumn->addWidget(m_titleLabel, 0, Qt::AlignLeft);

    topBarLayout->addLayout(topTextColumn);
    topBarLayout->addStretch();

    m_bottomBar = new QWidget(m_controlsOverlay);
    m_bottomBar->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        " stop:0 rgba(0,0,0,0), stop:1 rgba(0,0,0,190));"
    );

    QVBoxLayout* bottomBarLayout = new QVBoxLayout(m_bottomBar);
    bottomBarLayout->setContentsMargins(25, 40, 25, 15);
    bottomBarLayout->setSpacing(8);

    m_progressSlider = new ClickableSlider(Qt::Horizontal, m_bottomBar);
    m_progressSlider->setStyle(QStyleFactory::create("Fusion"));
    m_progressSlider->setRange(0, 1000);
    m_progressSlider->setValue(0);
    m_progressSlider->setStyleSheet(
        "QSlider { background: transparent; }"
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: rgba(255, 255, 255, 0.51);"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 10px;"
        "  height: 10px;"
        "  margin: -3px 0;"
        "  background: #e6e6e6;"
        "  border: none;"
        "  border-radius: 5px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: rgba(255, 255, 255, 0.51);"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
        "QSlider::add-page:horizontal {"
        "  background: rgba(255, 255, 255, 0.51);"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
    );

    m_currentTimeLabel = new QLabel("0:00:00", m_bottomBar);
    m_currentTimeLabel->setStyleSheet("color: #cccccc; font-size: 12px; background: transparent;");

    m_totalTimeLabel = new QLabel("0:00:00", m_bottomBar);
    m_totalTimeLabel->setStyleSheet("color: #cccccc; font-size: 12px; background: transparent;");

    QHBoxLayout* progressRow = new QHBoxLayout();
    progressRow->addWidget(m_currentTimeLabel);
    progressRow->addWidget(m_progressSlider);
    progressRow->addWidget(m_totalTimeLabel);

    m_playPauseButton = new QPushButton(m_bottomBar);
    m_playPauseButton->setIcon(QIcon(iconPath("pause.svg")));
    m_playPauseButton->setIconSize(QSize(18, 18));
    m_playPauseButton->setFixedSize(36, 36);
    m_playPauseButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );
    m_playPauseButton->setEnabled(false);

    m_previousButton = new QPushButton(m_bottomBar);
    m_previousButton->setIcon(QIcon(iconPath("skip-back.svg")));
    m_previousButton->setIconSize(QSize(18, 18));
    m_previousButton->setFixedSize(36, 36);
    m_previousButton->setEnabled(false);
    m_previousButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
        "QPushButton:disabled { opacity: 0.4; }"
    );

    m_nextButton = new QPushButton(m_bottomBar);
    m_nextButton->setIcon(QIcon(iconPath("skip-forward.svg")));
    m_nextButton->setIconSize(QSize(18, 18));
    m_nextButton->setFixedSize(36, 36);
    m_nextButton->setEnabled(false);
    m_nextButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
        "QPushButton:disabled { opacity: 0.4; }"
    );

    m_volumeIconButton = new QPushButton(m_bottomBar);
    m_volumeIconButton->setIcon(QIcon(iconPath(m_appSettings->getDefaultVolume() > 0 ? "volume.svg" : "volume-mute.svg")));
    m_volumeIconButton->setIconSize(QSize(18, 18));
    m_volumeIconButton->setFixedSize(36, 36);
    m_volumeIconButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );
    connect(m_volumeIconButton, &QPushButton::clicked, this, &MainWindow::onVolumeIconClicked);

    m_volumeSlider = new QSlider(Qt::Horizontal, m_bottomBar);
    m_volumeSlider->setFixedWidth(100);
    m_volumeSlider->setRange(0, 400);
    m_volumeSlider->setValue(m_appSettings->getDefaultVolume());
    m_volumeSlider->setStyleSheet(
        "QSlider {"
        "  background: transparent;"
        "}"
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: #444444;"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 12px;"
        "  height: 12px;"
        "  margin: -4px 0;"
        "  background: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: #6a6a6a;"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
        "QSlider::add-page:horizontal {"
        "  background: #444444;"
        "  border: none;"
        "  border-radius: 2px;"
        "}"
    );

    m_subtitleTrackButton = new QPushButton(m_bottomBar);
    m_subtitleTrackButton->setIcon(QIcon(iconPath("subtitles.svg")));
    m_subtitleTrackButton->setIconSize(QSize(18, 18));
    m_subtitleTrackButton->setFixedSize(36, 36);
    m_subtitleTrackButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );

    m_audioTrackButton = new QPushButton(m_bottomBar);
    m_audioTrackButton->setIcon(QIcon(iconPath("audio.svg")));
    m_audioTrackButton->setIconSize(QSize(18, 18));
    m_audioTrackButton->setFixedSize(36, 36);
    m_audioTrackButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );

    m_loopButton = new QPushButton(m_bottomBar);
    m_loopButton->setIcon(QIcon(iconPath("loop.svg")));
    m_loopButton->setIconSize(QSize(18, 18));
    m_loopButton->setFixedSize(36, 36);
    m_loopButton->setCheckable(true);
    m_loopButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
        "QPushButton:checked { background: rgba(255,255,255,40); border-radius: 18px; }"
    );

    m_aspectRatioButton = new QPushButton(m_bottomBar);
    m_aspectRatioButton->setIcon(QIcon(iconPath("aspect-ratio.svg")));
    m_aspectRatioButton->setIconSize(QSize(18, 18));
    m_aspectRatioButton->setFixedSize(36, 36);
    m_aspectRatioButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );

    m_fullscreenButton = new QPushButton(m_bottomBar);
    m_fullscreenButton->setIcon(QIcon(iconPath("fullscreen.svg")));
    m_fullscreenButton->setIconSize(QSize(18, 18));
    m_fullscreenButton->setFixedSize(36, 36);
    m_fullscreenButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255,255,255,20); border-radius: 18px; }"
    );
    connect(m_fullscreenButton, &QPushButton::clicked, this, &MainWindow::toggleFullscreen);

    QHBoxLayout* controlsRow = new QHBoxLayout();
    controlsRow->addWidget(m_previousButton);
    controlsRow->addWidget(m_playPauseButton);
    controlsRow->addWidget(m_nextButton);
    controlsRow->addSpacing(15);
    controlsRow->addWidget(m_volumeIconButton);
    controlsRow->addWidget(m_volumeSlider);
    controlsRow->addStretch();
    controlsRow->addWidget(m_subtitleTrackButton);
    controlsRow->addWidget(m_audioTrackButton);
    controlsRow->addWidget(m_loopButton);
    controlsRow->addWidget(m_aspectRatioButton);
    controlsRow->addWidget(m_fullscreenButton);

    bottomBarLayout->addLayout(progressRow);
    bottomBarLayout->addLayout(controlsRow);

    QWidget* middleSpacer = new QWidget(m_controlsOverlay);
    middleSpacer->setAttribute(Qt::WA_TransparentForMouseEvents);
    middleSpacer->setStyleSheet("background: transparent;");

    overlayLayout->addWidget(m_topBar);
    overlayLayout->addWidget(middleSpacer, 1);
    overlayLayout->addWidget(m_bottomBar);

    stackedLayout->addWidget(m_controlsOverlay);
    m_controlsOverlay->raise();

    m_playerScreen->setMouseTracking(true);
    m_videoWidget->setMouseTracking(true);
    m_controlsOverlay->setMouseTracking(true);
    m_topBar->setMouseTracking(true);
    m_bottomBar->setMouseTracking(true);

    m_playerScreen->installEventFilter(this);
    m_videoWidget->installEventFilter(this);
    m_controlsOverlay->installEventFilter(this);
    m_topBar->installEventFilter(this);
    m_bottomBar->installEventFilter(this);

    m_metadataLabel = new QLabel("No file open.", this);
    m_metadataLabel->setStyleSheet(
        "QLabel { color: #888888; font-size: 12px; }"
    );
    m_metadataLabel->hide();

    m_hideControlsTimer = new QTimer(this);
    m_hideControlsTimer->setInterval(2000);
    m_hideControlsTimer->setSingleShot(true);
    connect(m_hideControlsTimer, &QTimer::timeout, this, &MainWindow::hideControlsOverlay);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(1);
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(250);

    connect(m_homeScreen, &HomeScreen::settingsRequested, this, [this]()
    {
        navigateTo(m_settingsScreen);
    });

    connect(m_renderTimer, &QTimer::timeout,
            this,          &MainWindow::onRenderFrame);
    
    connect(m_progressTimer, &QTimer::timeout,
            this,             &MainWindow::onProgressUpdateTick);

    connect(m_playPauseButton, &QPushButton::clicked,
            this,       &MainWindow::onPlayPauseClicked);

    connect(m_volumeSlider, &QSlider::valueChanged,
            this,      &MainWindow::onVolumeChanged);

    connect(m_progressSlider, &QSlider::sliderPressed,
            this,       &MainWindow::onSeekSliderPressed);

    connect(m_progressSlider, &QSlider::sliderReleased,
            this,     &MainWindow::onSeekSliderReleased);

    connect(m_backButton, &QPushButton::clicked,
        this,          &MainWindow::onBackClicked);

    connect(m_audioTrackButton, &QPushButton::clicked, this, [this]()
    {
        auto tracks = m_engine->getAudioTracks();
        if (tracks.empty()) return;

        QMenu menu(this);
        menu.setStyleSheet(kMenuStyle);

        int current = m_engine->getCurrentAudioStreamIndex();

        for (const auto& t : tracks)
        {
            QString label = QString("Track %1 (%2, %3)")
                .arg(t.streamIndex)
                .arg(QString::fromStdString(t.language))
                .arg(QString::fromStdString(t.codecName));

            QAction* action = menu.addAction(label);
            action->setCheckable(true);
            action->setChecked(t.streamIndex == current);

            connect(action, &QAction::triggered, this, [this, t]()
            {
                onAudioTrackSelected(t.streamIndex);
            });
        }
        menu.exec(m_audioTrackButton->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
    });

    connect(m_subtitleTrackButton, &QPushButton::clicked, this, [this]()
    {
        auto tracks = m_engine->getSubtitleTracks();

        QMenu menu(this);
        menu.setStyleSheet(kMenuStyle);

        int current = m_engine->getCurrentSubtitleStreamIndex();

        QAction* offAction = menu.addAction("Off");
        offAction->setCheckable(true);
        offAction->setChecked(current < 0);
        connect(offAction, &QAction::triggered, this, [this]()
        {
            onSubtitleTrackSelected(-1);
        });

        menu.addSeparator();

        for (const auto& t : tracks)
        {
            QString label = QString("Track %1 (%2, %3)")
                .arg(t.streamIndex)
                .arg(QString::fromStdString(t.language))
                .arg(QString::fromStdString(t.codecName));

            QAction* action = menu.addAction(label);
            action->setCheckable(true);
            action->setChecked(t.streamIndex == current);

            connect(action, &QAction::triggered, this, [this, t]()
            {
                onSubtitleTrackSelected(t.streamIndex);
            });
        }

        menu.addSeparator();
        QAction* loadFileAction = menu.addAction("Load Subtitle File...");
        connect(loadFileAction, &QAction::triggered, this, &MainWindow::onLoadSubtitleFileClicked);

        menu.exec(m_subtitleTrackButton->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
    });

    connect(m_aspectRatioButton, &QPushButton::clicked, this, [this]()
    {
        QMenu menu(this);
        menu.setStyleSheet(kMenuStyle);

        struct Option { QString label; AspectRatioMode mode; };
        Option options[] = {
            { "Fit (Default)", AspectRatioMode::Fit },
            { "Fill", AspectRatioMode::Fill },
            { "16:9", AspectRatioMode::Ratio_16_9 },
            { "4:3", AspectRatioMode::Ratio_4_3 },
            { "1:1", AspectRatioMode::Ratio_1_1 },
            { "2.35:1 (Cinemascope)", AspectRatioMode::Ratio_2_35_1 }
        };

        AspectRatioMode current = m_videoWidget->getAspectRatioMode();

        for (const auto& opt : options)
        {
            QAction* action = menu.addAction(opt.label);
            action->setCheckable(true);
            action->setChecked(opt.mode == current);

            AspectRatioMode mode = opt.mode;
            connect(action, &QAction::triggered, this, [this, mode]()
            {
                m_videoWidget->setAspectRatioMode(mode);
            });
        }

        menu.exec(m_aspectRatioButton->mapToGlobal(QPoint(0, -menu.sizeHint().height())));
    });

    connect(m_loopButton, &QPushButton::toggled, this, [this](bool checked)
    {
        m_engine->setLoopEnabled(checked);
    });

    connect(m_previousButton, &QPushButton::clicked, this, &MainWindow::onPreviousClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &MainWindow::onNextClicked);

    m_engine      = new PlaybackEngine();
    m_engine->setErrorCallback([this](PlaybackEngine::ErrorSeverity severity, const std::string& message)
    {
        QString text = QString::fromStdString(message);

        QMetaObject::invokeMethod(this, [this, severity, text]()
        {
            if (severity == PlaybackEngine::ErrorSeverity::Error)
            {
                QMessageBox::critical(this, "Playback Error", text);
            }
            else if (severity == PlaybackEngine::ErrorSeverity::Warning)
            {
                QMessageBox::warning(this, "Warning", text);
            }
            else
            {
                m_metadataLabel->setText(text);
            }
        }, Qt::QueuedConnection);
    });

    m_audioOutput = new AudioOutput(this);

}

MainWindow::~MainWindow()
{
    stopPlayback();
    PlaylistStore::save(*m_playlistManager);
    delete m_engine;
    delete m_playlistManager;
    delete m_appSettings;
}

void MainWindow::navigateTo(QWidget* screen)
{
    QWidget* current = m_stack->currentWidget();
    if(current && current != screen){
        m_navigationHistory.push_back(current);
    }
    m_stack->setCurrentWidget(screen);
}

void MainWindow::navigateBack(){
    if(m_navigationHistory.empty())
    {
        m_stack->setCurrentWidget(m_homeScreen);
        return;
    }
    QWidget* previous = m_navigationHistory.back();
    m_navigationHistory.pop_back();
    m_stack->setCurrentWidget(previous);
}

void MainWindow::onBackClicked(){
    stopPlayback();
    navigateBack();
    
}

void MainWindow::stopPlayback()
{
    m_renderTimer->stop();
    m_progressTimer->stop();
    m_audioOutput->stop();
    m_engine->stop();
    m_frameCount = 0;
}

void MainWindow::onVolumeChanged(int value)
{
    const float breakpoint = 0.8f;
    const float maxVolume  = 4.0f;

    float p = value / 400.0f;
    float volume;

    if (p <= breakpoint)
    {
        volume = p / breakpoint;
    }
    else
    {
        volume = 1.0f + (p - breakpoint) / (1.0f - breakpoint)
                       * (maxVolume - 1.0f);
    }

    m_audioOutput->setVolume(volume);
    m_volumeSlider->setTickPosition(QSlider::TicksBelow);
    m_volumeSlider->setTickInterval(400);

    m_volumeIconButton->setIcon(QIcon(iconPath(m_appSettings->getDefaultVolume() > 0 ? "volume.svg" : "volume-mute.svg")));

    if (value > 0)
    {
        m_lastNonZeroVolume = value;
        m_volumeIconButton->setIcon(QIcon(iconPath("volume.svg")));
    }
    else
    {
        m_volumeIconButton->setIcon(QIcon(iconPath("volume-mute.svg")));
    }
}

void MainWindow::onVolumeIconClicked()
{
    if (m_volumeSlider->value() > 0)
    {
        m_lastNonZeroVolume = m_volumeSlider->value();
        m_volumeSlider->setValue(0);
    }
    else
    {
        m_volumeSlider->setValue(m_lastNonZeroVolume);
    }
}

void MainWindow::onOpenFileClicked()
{
    stopPlayback(); 

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Media File",
        QString(),
        "Media Files (*.mp4 *.mkv *.avi *.mov *.mp3 *.flac *.wav)"
        ";;All Files (*.*)"
    );

    if (filePath.isEmpty())
        return;

    std::string path = filePath.toStdString();

    if (!m_engine->open(path))
    {
        m_metadataLabel->setText("Failed to open file.");
        return;
    }

    m_engine->setSubtitleFontScale(m_appSettings->getSubtitleFontScale());

    m_currentPlaylistIndex = -1;
    m_currentItemIndex = -1;
    m_nextButton->setEnabled(false);
    m_previousButton->setEnabled(false);

    navigateTo(m_playerScreen);

    m_engine->setVideoFrameCallback(
        [this](const uint8_t* data, int w, int h, int ls)
        {
            m_videoWidget->displayFrame(data, w, h, ls);
        }
    );

    updateMetadataDisplay(filePath);
    m_titleLabel->setText(QFileInfo(filePath).fileName());

    m_audioOutput->start(
        44100, 2,
        m_engine->getAudioQueue(),
        m_engine->getAVSync()
    );

    m_engine->play();
    m_frameCount = 0;
    m_renderTimer->start();
    m_progressTimer->start();
    m_playPauseButton->setEnabled(true);
    m_playPauseButton->setIcon(QIcon(iconPath("pause.svg")));
    std::cout << "[MainWindow] Playback started." << std::endl;
}

void MainWindow::onPlaylistOpenRequested(int playlistIndex)
{
    m_playlistDetailScreen->showPlaylist(playlistIndex);
    navigateTo(m_playlistDetailScreen);
}

void MainWindow::onPlaylistItemPlayRequested(int itemIndex, const QString& filepath)
{
    int playlistIndex = m_playlistDetailScreen->getCurrentPlaylistIndex();
    playFromPlaylist(playlistIndex, itemIndex);
}

void MainWindow::playFromPlaylist(int playlistIndex, int itemIndex)
{
    Playlist* pl = m_playlistManager->getPlaylist(playlistIndex);
    if (!pl) return;

    const PlaylistItem* item = pl->getItem(itemIndex);
    if (!item) return;

    m_currentPlaylistIndex = playlistIndex;
    m_currentItemIndex = itemIndex;

    QString filepath = QString::fromStdString(item->filepath);

    stopPlayback();
    std::string path = filepath.toStdString();

    if (!m_engine->open(path))
    {
        m_metadataLabel->setText("Failed to open the file.");
        return;
    }

    m_engine->setSubtitleFontScale(m_appSettings->getSubtitleFontScale());

    navigateTo(m_playerScreen);

    m_engine->setVideoFrameCallback(
        [this](const uint8_t* data, int w, int h, int ls)
        {
            m_videoWidget->displayFrame(data, w, h, ls);
        }
    );

    updateMetadataDisplay(filepath);
    m_titleLabel->setText(QFileInfo(filepath).fileName());

    m_audioOutput->start(44100, 2, m_engine->getAudioQueue(), m_engine->getAVSync());
    m_engine->play();
    m_frameCount = 0;
    m_renderTimer->start();
    m_progressTimer->start();
    m_playPauseButton->setEnabled(true);
    m_playPauseButton->setIcon(QIcon(iconPath("pause.svg")));

    m_nextButton->setEnabled(pl->getItem(itemIndex + 1) != nullptr);
    m_previousButton->setEnabled(itemIndex > 0);
}

void MainWindow::onNextClicked()
{
    if (m_currentPlaylistIndex < 0) return;
    playFromPlaylist(m_currentPlaylistIndex, m_currentItemIndex + 1);
}

void MainWindow::onPreviousClicked()
{
    if (m_currentPlaylistIndex < 0) return;
    if (m_currentItemIndex <= 0) return;
    playFromPlaylist(m_currentPlaylistIndex, m_currentItemIndex - 1);
}

void MainWindow::showControlsOverlay()
{
    m_controlsOverlay->show();
    m_hideControlsTimer->start();
}

void MainWindow::hideControlsOverlay()
{
    if (m_stack->currentWidget() == m_playerScreen && m_engine->isPlaying() && !m_engine->isPaused())
    {
        m_controlsOverlay->hide();
    }
}

void MainWindow::onLoadSubtitleFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Subtitle File",
        QString(),
        "Subtitle Files (*.srt *.ass *.ssa *.vtt);;All Files (*.*)"
    );

    if (filePath.isEmpty())
        return;

    m_engine->requestLoadSubtitleFile(filePath.toStdString());
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        if (m_stack->currentWidget() == m_playerScreen)
        {
            showControlsOverlay();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onAudioTrackSelected(int streamIndex)
{
    std::cout << "[Timing] Audio switch requested at "
               << std::chrono::steady_clock::now().time_since_epoch().count() << std::endl;
    m_engine->requestAudioTrackChange(streamIndex);
}

void MainWindow::onSubtitleTrackSelected(int streamIndex)
{
    m_engine->requestSubtitleTrackChange(streamIndex);
}

void MainWindow::onHomeOpenFileRequested()
{
    onOpenFileClicked();  
}

void MainWindow::onPlayPauseClicked()
{
    if (!m_engine->isPlaying() && !m_engine->isPaused())
    {
        return;
    }

    if(m_engine->isPaused())
    {
        m_engine->resume();
        m_audioOutput->resume();
        m_playPauseButton->setIcon(QIcon(iconPath("pause.svg")));
        std::cout<<"[MainWindow] resumed."<<std::endl;
    }
    else
    {
        m_engine->pause();
        m_audioOutput->pause();
        m_playPauseButton->setIcon(QIcon(iconPath("play.svg")));
        std::cout<<"[MainWindow] paused. " <<std::endl;
    }
}

void MainWindow::onSeekSliderPressed()
{
    m_userIsSeeking = true;
}

void MainWindow::onSeekSliderReleased()
{
    if(!m_engine->isOpen()) { m_userIsSeeking = false; return; }
    
    int sliderValue = m_progressSlider->value();
    double fraction = sliderValue / 1000.0;
    double duration = m_engine->getDuration();
    double target = fraction * duration;

    m_engine->requestSeek(target);

    m_userIsSeeking = false;

    std::cout << "[MainWindow] Seek slider released. Target sec "<< target << "s" <<std::endl;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_S && m_engine->isOpen())
    {
        double mid = m_engine->getDuration() / 2.0;
        m_engine->requestSeek(mid);
        std::cout<<"[MainWindow] seek to middle: " <<mid<<std::endl;
    }

    if(event->key() == Qt::Key_Space && m_engine->isOpen())
    {
        onPlayPauseClicked();

    }
    if(event->key() == Qt::Key_P && m_engine->isOpen())
    {
        onPlayPauseClicked();

    }
    if(event->key() == Qt::Key_Right && m_engine->isOpen())
    {
        double forward = m_engine->getPosition() + 10.0;
        if(forward > m_engine->getDuration()) forward = m_engine->getDuration();
        m_engine->requestSeek(forward);
        std::cout<<"[MainWindow] seek forward 10s to: " <<forward<<std::endl;
    }

    if(event->key() == Qt::Key_Left && m_engine->isOpen())
    {
        double backward = m_engine->getPosition() - 10.0;
        if(backward < 0.0) backward = 0.0;
        m_engine->requestSeek(backward);
        std::cout<<"[MainWindow] seek backward 10s to: " <<backward<<std::endl;
    }

    if(event->key() == Qt::Key_L && m_engine->isOpen())
    {
        double forward = m_engine->getPosition() + 10.0;
        if(forward > m_engine->getDuration()) forward = m_engine->getDuration();
        m_engine->requestSeek(forward);
        std::cout<<"[MainWindow] seek forward 10s to: " <<forward<<std::endl;
    }

    if(event->key() == Qt::Key_K && m_engine->isOpen())
    {
        double backward = m_engine->getPosition() - 10.0;
        if(backward < 0.0) backward = 0.0;
        m_engine->requestSeek(backward);
        std::cout<<"[MainWindow] seek backward 10s to: " <<backward<<std::endl;
    }


    if(m_engine->isOpen() && (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_5))
        {
            auto tracks = m_engine->getAudioTracks();
            int requestedPosition = event->key() - Qt::Key_0;

            if (requestedPosition >= (int)tracks.size())
            {
                std::cout << "[MainWindow] No audio track at position "
                        << requestedPosition << " (only " << tracks.size()
                        << " available)." << std::endl;
            }
            else
            {
                int streamIndex = tracks[requestedPosition].streamIndex;
                std::cout << "[MainWindow] Switching to audio track " << requestedPosition
                        << " (stream index " << streamIndex << ", "
                        << tracks[requestedPosition].language << ", "
                        << tracks[requestedPosition].codecName << ")" << std::endl;

                m_engine->requestAudioTrackChange(streamIndex);
            }
        }
    
    if(event->key() == Qt::Key_T && m_engine->isOpen())
    {
        auto subs = m_engine->getSubtitleTracks();
        std::cout << "[MainWindow] Subtitle tracks found: " << subs.size() << std::endl;
        for (const auto& s : subs)
        {
            std::cout << "  - stream " << s.streamIndex << ", lang: " << s.language
                    << ", codec: " << s.codecName << std::endl;
        }
    }

    QMainWindow::keyPressEvent(event);

}

void MainWindow::onRenderFrame()
{
    m_frameCount++;

    if (m_frameCount > 120 && !m_engine->isPlaying())
    {
        stopPlayback();
        m_metadataLabel->setText("Playback complete.");
        return;
    }

    m_engine->renderNextFrame();
}

void MainWindow::onProgressUpdateTick()
{
    if(!m_engine->isOpen()) return;
    if(m_userIsSeeking) return;

    double position = m_engine->getPosition();
    double duration = m_engine->getDuration();

    if(duration <= 0.0) return;

    auto formatTime = [](double seconds) -> QString
    {
        int total = (int)seconds;
        int h = total / 3600;
        int m = (total % 3600) / 60;
        int s = total % 60;
        return QString("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    };

    m_currentTimeLabel->setText(formatTime(position));
    m_totalTimeLabel->setText(formatTime(duration));

    double fraction = position / duration;
    if(fraction < 0.0) fraction = 0.0;
    if(fraction > 1.0) fraction = 1.0;

    m_progressSlider->blockSignals(true);
    m_progressSlider->setValue(static_cast<int>(fraction * 1000));
    m_progressSlider->blockSignals(false);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    toggleFullscreen();
    event->accept();
}

void MainWindow::toggleFullscreen()
{
    if (m_isFullscreen)
    {
        setWindowFlag(Qt::FramelessWindowHint, false);
        show();
        showNormal();
        setGeometry(m_normalGeometry);
        m_isFullscreen = false;
    }
    else
    {
        m_normalGeometry = geometry();
        setWindowFlag(Qt::FramelessWindowHint, true);
        show();
        if (QScreen* scr = screen())
        {
            const int overscan = 2;
            setGeometry(scr->geometry().adjusted(-overscan, -overscan, overscan, overscan));
        }
        m_isFullscreen = true;
    }
}

void MainWindow::updateMetadataDisplay(const QString& filepath)
{
    SumPlayer::MediaProbe probe;
    probe.probe(filepath.toStdString());

    QString text = QString(
        "%1  |  %2x%3 @ %4fps  |  %5  |  %6ch @%7Hz  |  %8")
        .arg(QFileInfo(filepath).fileName())
        .arg(probe.getVideoWidth())
        .arg(probe.getVideoHeight())
        .arg(probe.getFrameRate(), 0, 'f', 3)
        .arg(QString::fromStdString(probe.getVideoCodec()))
        .arg(probe.getAudioChannels())
        .arg(probe.getAudioSampleRate())
        .arg(QString::fromStdString(probe.getAudioCodec()));

    m_metadataLabel->setText(text);
}

}
