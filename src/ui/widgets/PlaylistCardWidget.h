#pragma once
#include <QFrame>

class QLabel;
class QToolButton;

namespace SumPlayer
{
    class PlaylistCardWidget:public QFrame
    {
        Q_OBJECT

        public:
            public:
                    PlaylistCardWidget(const QString& name, int itemCount,
                                            const QString& dateCreated,
                                            const QString& thumbnailPath,
                                            QWidget* parent = nullptr);


        signals:
            void opened();
            void renameRequested();
            void deleteRequested();

        protected:
            void mousePressEvent(QMouseEvent* event) override;

        private:
            void showContextMenu();
            
    };
}