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


#ifndef _xSearchWidget_h_
#define _xSearchWidget_h_ 1

#include <QWidget>

#include "xdocument.h"

class xPlainTextViewer;
class QCheckBox;
class QComboBox;

class xSearchWidget: public QWidget {
	Q_OBJECT
public:
	xSearchWidget(QWidget * pParent);
	~xSearchWidget();

    void                setSearchText(const QString & text);
    searchRequestItem   currentSearchItem(bool * bOk) const;

signals:

    void       searchChanged(const searchRequestItem & item);
    void       findAll(const searchRequestItem & item);
    void       addFilter(const searchRequestItem & item);

public slots:

    void     onTextChanged(const QString &text);

protected:

    QComboBox        *  m_searchLine;

    QCheckBox        *  m_wholeWords;
    QCheckBox        *  m_matchCase;
    QCheckBox        *  m_regularExpression;

};

#endif