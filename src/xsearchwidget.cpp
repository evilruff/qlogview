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
#include <QLabel>
#include <QPalette>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTextCodec>

#include "xsearchwidget.h"
#include "xplaintextviewer.h"

xSearchWidget::xSearchWidget(QWidget * pParent) :
    QWidget(pParent) {

    setAutoFillBackground(true);
    
    QPalette p = palette();
    p.setColor(backgroundRole(), Qt::gray);
    setPalette(p);
    
    QVBoxLayout * pLayout = new QVBoxLayout();

    QHBoxLayout * pSearchLayout = new QHBoxLayout();
    pSearchLayout->addWidget(new QLabel(tr("Search:")));
    m_searchLine = new QComboBox();
    m_searchLine->setEditable(true);
    m_searchLine->setMinimumWidth(200);
    pSearchLayout->addWidget(m_searchLine);
    
    m_wholeWords = new QCheckBox(tr("Whole words only"));
    connect(m_wholeWords, &QCheckBox::stateChanged, [this]() {
        onTextChanged(m_searchLine->currentText());
    });

    m_matchCase = new QCheckBox(tr("Match case"));
    connect(m_matchCase, &QCheckBox::stateChanged, [this]() {
        onTextChanged(m_searchLine->currentText());
    });

    m_regularExpression = new QCheckBox(tr("Regular expression"));
    connect(m_regularExpression, &QCheckBox::stateChanged, [this]() {
        onTextChanged(m_searchLine->currentText());
    });


    pLayout->addLayout(pSearchLayout);
    pLayout->addWidget(m_wholeWords);
    pLayout->addWidget(m_matchCase);
    pLayout->addWidget(m_regularExpression);

    connect(m_searchLine, &QComboBox::currentTextChanged, this, &xSearchWidget::onTextChanged);
    connect(m_searchLine, &QComboBox::editTextChanged, this, &xSearchWidget::onTextChanged);

    QHBoxLayout * pBtnLayout = new QHBoxLayout();

    pBtnLayout->addSpacing(100);

    QPushButton * pBtn = new QPushButton(tr("Find All"));    
    connect(pBtn, &QPushButton::clicked, [this]() {
        bool bOk = false;
        searchRequestItem   item = currentSearchItem(&bOk);
        if (bOk) {
            emit findAll(item);
        }
    });
    pBtnLayout->addWidget(pBtn);

    pBtn = new QPushButton(tr("Add Filter"));
    connect(pBtn, &QPushButton::clicked, [this]() {
        bool bOk = false;
        searchRequestItem   item = currentSearchItem(&bOk);
        if (bOk) {
            emit addFilter(item);
        }
    });
    pBtnLayout->addWidget(pBtn);
    

    pLayout->addLayout(pBtnLayout);

    setLayout(pLayout);

    setFocusProxy(m_searchLine);
}

void      xSearchWidget::setSearchText(const QString & text) {
    m_searchLine->setCurrentText(text);
}

xSearchWidget::~xSearchWidget() {

}

searchRequestItem   xSearchWidget::currentSearchItem(bool * bOk) const {
    searchRequestItem   item;

    if (m_regularExpression->isChecked()) {
        item = searchRequestItem::regexpSearch(m_searchLine->currentText());
        if (!item.regexp.isValid() || item.regexp.pattern().isEmpty()) {
            *bOk = false;
            return item;
        }
    }
    else {
        if (m_wholeWords->isChecked()) {
            item = searchRequestItem::regexpSearch(QString("\\b%1\\b").arg(m_searchLine->currentText()));
            if (!item.regexp.isValid() || item.regexp.pattern().isEmpty()) {
                *bOk = false;
                return item;
            }
        }
        else {
            if (m_searchLine->currentText().isEmpty()) {
                *bOk = false;
                return item;
            }
            item = searchRequestItem::patternSearch(m_searchLine->currentText());
            if (m_matchCase->isChecked()) {
                item.matcher.setCaseSensitivity(Qt::CaseSensitive);
            }
            else {
                item.matcher.setCaseSensitivity(Qt::CaseInsensitive);
            }
        }
    }

    *bOk = true;
    return item;
}

void xSearchWidget::onTextChanged(const QString & text) {
    if (!isVisible() || (text.length() == 0))
        return;

    bool bOk = false;
    searchRequestItem   item = currentSearchItem(&bOk);

    if (bOk) {
        emit searchChanged(item);
    }
}
