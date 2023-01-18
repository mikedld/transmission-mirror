// This file Copyright © 2009-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <QItemDelegate>

#include <libtransmission/tr-macros.h>

class FileTreeDelegate : public QItemDelegate
{
    Q_OBJECT
    TR_DISABLE_COPY_MOVE(FileTreeDelegate)

public:
    explicit FileTreeDelegate(QObject* parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    // QAbstractItemDelegate
    QSize sizeHint(QStyleOptionViewItem const&, QModelIndex const&) const override;
    void paint(QPainter*, QStyleOptionViewItem const&, QModelIndex const&) const override;
};
