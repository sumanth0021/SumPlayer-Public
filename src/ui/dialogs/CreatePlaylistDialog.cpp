#include "ui/dialogs/CreatePlaylistDialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace SumPlayer
{
    CreatePlaylistDialog::CreatePlaylistDialog(const QString& titleText,
                                                const QString& prefillName,
                                                QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(titleText);
        setFixedSize(440, 180);
        setStyleSheet("background-color: #161616;");

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(25, 25, 25, 25);

        QLabel* title = new QLabel(titleText, this);
        title->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold;");

        m_nameEdit = new QLineEdit(prefillName, this);
        m_nameEdit->setPlaceholderText("Playlist name");
        m_nameEdit->setStyleSheet(
            "background-color: #1e1e1e; color: #ffffff;"
            "border: 1px solid #333333; border-radius: 4px;"
            "padding: 8px; font-size: 13px;"
        );

        QPushButton* cancelButton = new QPushButton("Cancel", this);
        QPushButton* createButton = new QPushButton("Create", this);
        createButton->setDefault(true);

        QString buttonStyle =
            "QPushButton { background-color: #2a2a2a; color: #ffffff;"
            "border-radius: 4px; padding: 8px 16px; }"
            "QPushButton:hover { background-color: #3a3a3a; }";
        cancelButton->setStyleSheet(buttonStyle);
        createButton->setStyleSheet(buttonStyle);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        buttonRow->addWidget(cancelButton);
        buttonRow->addWidget(createButton);

        layout->addWidget(title);
        layout->addSpacing(15);
        layout->addWidget(m_nameEdit);
        layout->addStretch();
        layout->addLayout(buttonRow);

        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(createButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    QString CreatePlaylistDialog::playlistName() const
    {
        return m_nameEdit->text().trimmed();
    }
}