#pragma once
#include <QDialog>

class QLineEdit;

namespace SumPlayer
{
    class CreatePlaylistDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit CreatePlaylistDialog(const QString& titleText = "New Playlist",
                                       const QString& prefillName = "",
                                       QWidget* parent = nullptr);
        QString playlistName() const;

    private:
        QLineEdit* m_nameEdit;
    };
}