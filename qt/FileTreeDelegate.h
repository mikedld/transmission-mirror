/*
 * This file Copyright (C) 2009-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#pragma once

#include <QItemDelegate>

#include "Macros.h"

class FileTreeDelegate : public QItemDelegate
{
    Q_OBJECT
    TR_DISABLE_COPY_MOVE(FileTreeDelegate)

public:
    FileTreeDelegate(QObject* parent = nullptr) :
        QItemDelegate(parent)
    {
    }

public:
    // QAbstractItemDelegate
    QSize sizeHint(QStyleOptionViewItem const&, QModelIndex const&) const override;
    void paint(QPainter*, QStyleOptionViewItem const&, QModelIndex const&) const override;
};
