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
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTime>


#include "xtimestamppanel.h"

xTimeStampPanel::xTimeStampPanel(QWidget * pParent):
    QDialog(pParent) {
    QVBoxLayout * pLayout = new QVBoxLayout();

    QGridLayout * pGridLayout = new QGridLayout();

    m_sampleText = new QLineEdit();
    m_sampleText->setReadOnly(true);
    pGridLayout->addWidget(new QLabel(tr("Selected text:")), 0, 0);
    pGridLayout->addWidget(m_sampleText, 0, 1);

    m_parsedTime = new QLineEdit();
    m_parsedTime->setReadOnly(true);
    pGridLayout->addWidget(new QLabel(tr("Parsed timestamp:")), 1, 0);
    pGridLayout->addWidget(m_parsedTime, 1, 1);

    m_format = new QComboBox();
    m_format->setEditable(true);
    m_format->insertItem(0, "hh:mm:ss.zzz");
    m_format->insertItem(0, "hh:mm:ss.z");
    m_format->insertItem(0, "hh:mm:ss");
    m_format->insertItem(0, "hh:mm");
    
    connect(m_format, &QComboBox::currentTextChanged, this, &xTimeStampPanel::onTextChanged);
    connect(m_format, &QComboBox::editTextChanged, this, &xTimeStampPanel::onTextChanged);
  
    pGridLayout->addWidget(new QLabel(tr("Selected text:")), 2, 0);
    pGridLayout->addWidget(m_format, 2, 1);

    pLayout->addLayout(pGridLayout);
  
    QHBoxLayout * pRowLayout = new QHBoxLayout();
    pRowLayout->addStretch();
    
    QPushButton * pBtn = new QPushButton(tr("Create"));
    connect(pBtn, &QPushButton::clicked, [this]() {
        accept();
    });
    pRowLayout->addWidget(pBtn);
    pBtn = new QPushButton(tr("Cancel"));
    connect(pBtn, &QPushButton::clicked, [this]() {
        reject();
    });
    pRowLayout->addWidget(pBtn);

    pLayout->addLayout(pRowLayout);

    setLayout(pLayout);

}

xTimeStampPanel::~xTimeStampPanel() {

}

void    xTimeStampPanel::setSampleText(const QString & text) {
    m_sampleText->setText(text);
    onTextChanged(m_format->currentText());
}

void    xTimeStampPanel::onTextChanged(const QString &text) {
    
    m_formatLength = 0;
    QString currentText;
    
    for (int nLen = text.length(); nLen <= m_sampleText->text().length(); nLen++) {
        currentText = m_sampleText->text().left(nLen);

        QTime t = QTime::fromString(currentText, text);
        if (t.isValid()) {
            m_formatLength = nLen;
        }
    }

    if (m_formatLength == 0) {
        m_parsedTime->setText(tr("Wrong format"));
        m_formatValue = QString();
        return;
    }

    m_formatValue = text;
    currentText = m_sampleText->text().left(m_formatLength);
    QString parsed = QTime::fromString(currentText, text).toString("hh:mm:ss.zzz");
    m_parsedTime->setText(parsed);

}

int     xTimeStampPanel::detectedSourceDataLength() const {
    return m_formatLength;
}

bool    xTimeStampPanel::isValidFormat() const {
    return (detectedSourceDataLength() > 0);
}

QString xTimeStampPanel::format() const {
    return m_formatValue;
}
