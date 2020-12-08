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


#ifndef _xTreeView_h_
#define _xTreeView_h_ 1

#include <QTreeView>

class xTreeView: public QTreeView {
Q_OBJECT
public:
	xTreeView(QWidget * pParent = nullptr);
	~xTreeView();

    void    setPlaceholderText(const QString & text);
    void    setPlaceholderFont(const QFont & ft);
    void    setPlaceholderColor(const QColor & color);

    QString placeholderText() const;
    QColor  placeholderColor() const;
    QFont   placeholderFont() const;

protected:


    virtual void    paintEvent(QPaintEvent *event) override;

protected:

    QString     m_placeHolderText;
    QFont       m_placeHolderFont;
    QColor      m_placeHolderColor;
};

#endif