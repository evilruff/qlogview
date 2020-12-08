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
#include <QFont>

#include "xtableview.h"

xTableView::xTableView(QWidget * pParent):
    QTableView(pParent) {
    m_placeHolderColor = QColor(Qt::gray);
}

xTableView::~xTableView() {

};

void    xTableView::setPlaceholderText(const QString & text) {
    m_placeHolderText = text;
};

void    xTableView::setPlaceholderFont(const QFont & ft) {
    m_placeHolderFont = ft;
};

void    xTableView::setPlaceholderColor(const QColor & color) {
    m_placeHolderColor = color;
}

QString xTableView::placeholderText() const {
    return m_placeHolderText;
}

QColor  xTableView::placeholderColor() const {
    return m_placeHolderColor;
}

QFont   xTableView::placeholderFont() const {
    return m_placeHolderFont;
}

void    xTableView::paintEvent(QPaintEvent *event) {

    if (!m_placeHolderText.isEmpty() && (!model() || model()->rowCount() == 0)) {
        QPainter painter(viewport());
        painter.setPen(m_placeHolderColor);
        painter.setFont(m_placeHolderFont);
        painter.drawText(viewport()->rect(), Qt::AlignHCenter | Qt::AlignVCenter, m_placeHolderText );
        painter.end();
    } else {
        QTableView::paintEvent(event);
    }
}

