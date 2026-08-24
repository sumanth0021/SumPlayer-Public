#pragma once
#include <QFrame>
#include <QPixmap>

namespace SumPlayer{
    class PlaylistItemWidget: public QFrame
    {
        Q_OBJECT

        public:
        
        explicit PlaylistItemWidget(const QString& displayName,
                            const QString& thumbnailPath,
                            QWidget* parent = nullptr);

        signals:
            void playRequested();
            void renameRequested();
            void deleteRequested();

        protected:
            void mouseDoubleClickEvent(QMouseEvent* event) override;

        private:
            void showContextMenu();
    };
}