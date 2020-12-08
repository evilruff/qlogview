/**
 *  Copyright 2020 by Yuri Alexandrov <evilruff@gmail.com>
 *
 * This file is part of some open source application.
 *
 * Some open source application is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QLogView.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */


#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>

#include "xscrollbar.h"
#include "xplaintextviewer.h"
#include "xdocument.h"

xScrollBar::xScrollBar(xPlainTextViewer * pViewer, QWidget *parent)
    : QScrollBar(parent) {
    m_pViewer = pViewer;
}

void    xScrollBar::setMarksModel(QAbstractItemModel * pModel, int nColumn, int nRole) {
    if (m_marksModel) {
        disconnect(this, nullptr, m_marksModel, nullptr);
    }

    m_marksModel  = pModel;
    m_marksColumn = nColumn;
    m_marksRole   = nRole;

    if (m_marksModel) {
        connect(m_marksModel, &QAbstractItemModel::rowsInserted, [this]() {
            update();
        });
        connect(m_marksModel, &QAbstractItemModel::rowsRemoved, [this]() {
            update();
        });
        connect(m_marksModel, &QAbstractItemModel::dataChanged, [this]() {
            update();
        });
        connect(m_marksModel, &QAbstractItemModel::layoutChanged, [this]() {
            update();
        });
    }
}

QAbstractItemModel  * xScrollBar::marksModel() const {
    return m_marksModel;
}

int     xScrollBar::marksColumn() const {
    return m_marksColumn;
}

bool xScrollBar::isClipped() const
{
    return m_isClipped;
}

void xScrollBar::enableClipping(bool clip)
{
    m_isClipped = clip;
}

void xScrollBar::paintEvent(QPaintEvent *event)
{
    QScrollBar::paintEvent(event);

    if (!m_marksModel)
        return;

    QStyleOptionSlider styleOption;
    initStyleOption(&styleOption);

    QRect addPage = style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
        QStyle::SC_ScrollBarAddPage, this);
    QRect subPage = style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
        QStyle::SC_ScrollBarSubPage, this);
    QRect slider = style()->subControlRect(QStyle::CC_ScrollBar, &styleOption,
        QStyle::SC_ScrollBarSlider, this);

    QPainter p(this);
    if (isClipped()) {
        p.setClipRegion(QRegion(addPage) + QRegion(subPage));
    }

    qreal sf;

    if (orientation() == Qt::Horizontal)
    {
        p.translate(qMin(subPage.left(), addPage.left()), 0.0);

        sf = (addPage.width() + subPage.width() + slider.width())
            / ((qreal)maximum() - minimum());
    } else {
        p.translate(0.0, qMin(subPage.top(), addPage.top()));

        sf = (addPage.height() + subPage.height() + slider.height())
            / ((qreal)maximum() - minimum());
    }

    if (m_pViewer->document()) {
        int nRows = m_marksModel->rowCount();
        for (int i = 0; i < nRows; i++) {
            int         nMarkPosition = m_marksModel->data(m_marksModel->index(i, m_marksColumn), Qt::DisplayRole).toInt();
            nMarkPosition = m_pViewer->document()->logicalToSourceLineNumber(nMarkPosition);
            if (nMarkPosition == -1)
                continue;

            QColor      markColor = m_marksModel->data(m_marksModel->index(i, m_marksColumn), m_marksRole).value<QColor>();

            p.setPen(markColor);

            if (orientation() == Qt::Horizontal)
            {
                p.drawLine(QPoint(nMarkPosition * sf, 2),
                    QPoint(nMarkPosition * sf, slider.height() - 3));
            }
            else if (orientation() == Qt::Vertical)
            {
                p.drawLine(QPoint(2, nMarkPosition * sf),
                    QPoint(slider.width() - 3, nMarkPosition * sf));
            }
        }
    }

}

