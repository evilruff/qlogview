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


#ifndef _xScrollBar_h_
#define _xScrollBar_h_  1

#include <QScrollBar>
#include <QPaintEvent>

class QAbstractItemModel;
class xPlainTextViewer;
class xScrollBar : public QScrollBar
{
    Q_OBJECT

public:

    xScrollBar(xPlainTextViewer * pViewer, QWidget *parent = nullptr);
    ~xScrollBar() {};

    void                    setMarksModel(QAbstractItemModel * pModel, int nColumn = 0, int nRole = Qt::UserRole + 1);
    QAbstractItemModel  *   marksModel() const;
    int                     marksColumn() const;
    int                     marksRole() const;

    bool isClipped() const;
    void enableClipping(bool clip);

protected:

    virtual void paintEvent(QPaintEvent *event);

protected:

    QAbstractItemModel * m_marksModel   = nullptr;
    int                  m_marksColumn  = -1;
    int                  m_marksRole    = -1;
    bool m_isClipped                    = true;
    xPlainTextViewer   * m_pViewer      = nullptr;
};

#endif