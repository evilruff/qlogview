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


#ifndef _xTimestampPanel_h_
#define _xTimestampPanel_h_ 1

#include <QDialog>

class QPlainTextEdit;
class QLineEdit;
class QComboBox;
class QLabel;
class xRegexpSyntaxHighlighter;

class xTimeStampPanel: public QDialog {
	Q_OBJECT
public:

	xTimeStampPanel(QWidget * pParent);
	~xTimeStampPanel();

    void    setSampleText(const QString & text);
    void    setFormat(const QString & regexp);
    QString format() const;
    int     detectedSourceDataLength() const;
    bool    isValidFormat() const;

public slots:

    void     onTextChanged(const QString &text);
protected:
    int             m_formatLength = 0;
    QString         m_formatValue;
    QLineEdit    *  m_sampleText   = nullptr;
    QLineEdit    *  m_parsedTime   = nullptr;
    QComboBox    *  m_format       = nullptr;
};

#endif