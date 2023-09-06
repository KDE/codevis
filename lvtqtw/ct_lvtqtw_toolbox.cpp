// ct_lvtqtw_toolbox.cpp                                            -*-C++-*-

/*
    SPDX-FileCopyrightText: 2007 Vladimir Kuznetsov <ks.vladimir@gmail.com>
// Copyright 2023 Codethink Ltd <codethink@codethink.co.uk>
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <ct_lvtqtw_toolbox.h>

#include <ct_lvtqtc_itool.h>

#include <preferences.h>

#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QDebug>
#include <QEvent>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleOption>
#include <QStylePainter>
#include <QToolButton>
#include <QVBoxLayout>

class QPaintEvent;

using namespace Codethink::lvtqtw;

namespace {

void applyStyleSheetHack(QToolButton *btn)
{
#if defined __APPLE__
    static const QString buttonCss = R"(
        QToolButton {
            border: none;
        }

        QToolButton:pressed {
            border: none;
            background-color: gray;
        }

        QToolButton:checked {
            border: none;
            background-color: gray;
        }

        QToolButton:hover {
            border: none;
            background-color: lightgray;
        }
    )";
    btn->setStyleSheet(buttonCss);
#else
    Q_UNUSED(btn);
#endif
}

class Separator : public QWidget {
  public:
    explicit Separator(QString text, QWidget *parent): QWidget(parent), d_text(std::move(text))
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        setProperty("isSeparator", true);
    }

    [[nodiscard]] QSize sizeHint() const override
    {
        QStyleOption opt;
        opt.initFrom(this);
        const int extent = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, &opt, parentWidget());

        QFontMetrics fm(font());
        const int textHeight = fm.height();

        return {extent, textHeight};
    }

    void paintEvent(QPaintEvent *ev) override
    {
        Q_UNUSED(ev);
        QPainter p(this);
        QStyleOption opt;
        opt.initFrom(this);

        QFontMetrics fm(font());
        const int textWidth = fm.horizontalAdvance(d_text);
        constexpr int SPACER = 5;

        const int line1X = rect().x();
        const int lineWidth = (rect().width() / 2) - (textWidth / 2) - SPACER;
        const int line2X = rect().x() + lineWidth + textWidth + (SPACER * 2);

        QRect origRect = opt.rect;
        // Too small to write the name of the separator, just draw the line.
        if (d_text.isEmpty()) {
            // noop. don't draw anything.
        } else if (origRect.width() < textWidth + (SPACER * 2)) {
            style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, parentWidget());
        }
        // Draw two smaller lines on the side, and the name of the separator in
        // the middle.
        else {
            const QBrush textColor = qApp->palette().text();
            opt.rect = QRect(line1X, rect().y(), lineWidth, rect().height());
            style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, parentWidget());

            p.setPen(QPen(textColor, 1));
            p.setBrush(textColor);
            p.drawText(rect(), Qt::AlignCenter, d_text);

            opt.rect = QRect(line2X, rect().y(), lineWidth, rect().height());
            style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, parentWidget());
        }
    }

  private:
    QString d_text;
};

class ToolBoxLayout : public QLayout {
  public:
    explicit ToolBoxLayout(QWidget *parent, int margin = 0, int spacing = -1): QLayout(parent)
    {
        setContentsMargins(margin, margin, margin, margin);
        setSpacing(spacing);
        resetCache();
    }

    ~ToolBoxLayout() override
    {
        qDeleteAll(d_itemList);
    }

    void addItem(QLayoutItem *item) override
    {
        d_itemList.append(item);
        resetCache();
    }

    int count() const override
    {
        return d_itemList.size();
    }

    QLayoutItem *itemAt(int index) const override
    {
        return d_itemList.value(index);
    }

    QLayoutItem *takeAt(int index) override
    {
        resetCache();
        if (index >= 0 && index < d_itemList.size()) {
            return d_itemList.takeAt(index);
        }
        return nullptr;
    }

    Qt::Orientations expandingDirections() const override
    {
        return Qt::Vertical | Qt::Horizontal;
    }

    bool hasHeightForWidth() const override
    {
        return true;
    }

    int heightForWidth(int width) const override
    {
        if (d_isCachedHeightForWidth && d_cachedHeightForWidth.width() == width) {
            return d_cachedHeightForWidth.height();
        }
        d_cachedHeightForWidth.setWidth(width);
        d_cachedHeightForWidth.setHeight(doLayout(QRect(0, 0, width, 0), true));
        d_isCachedHeightForWidth = true;
        return d_cachedHeightForWidth.height();
    }

    void setGeometry(const QRect& rect) override
    {
        resetCache();
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

    QSize sizeHint() const override
    {
        return minimumSize();
    }

    QSize minimumSize() const override
    {
        if (d_isCachedMinimumSize) {
            return d_cachedMinimumSize;
        }

        d_cachedMinimumSize = QSize();
        for (QLayoutItem *item : d_itemList) {
            d_cachedMinimumSize = d_cachedMinimumSize.expandedTo(item->minimumSize());
        }
        d_isCachedMinimumSize = true;
        return d_cachedMinimumSize;
    }

    void setOneLine(bool b)
    {
        d_oneLine = b;
        invalidate();
    }

    bool isOneLine() const
    {
        return d_oneLine;
    }

    void invalidate() override
    {
        resetCache();
        QLayout::invalidate();
    }

  protected:
    void resetCache()
    {
        d_isCachedMinimumSize = false;
        d_isCachedHeightForWidth = false;
    }

    int doLayout(const QRect& rect, bool testOnly) const
    {
        int x = rect.x();
        int y = rect.y();
        int lineHeight = 0;

        if (d_oneLine) {
            for (QLayoutItem *item : d_itemList) {
                y = y + lineHeight + spacing();
                lineHeight = item->sizeHint().height();
                if (!testOnly) {
                    item->setGeometry(QRect(rect.x(), y, rect.width(), lineHeight));
                }
            }
        } else {
            for (QLayoutItem *item : d_itemList) {
                int w = item->sizeHint().width();
                int h = item->sizeHint().height();
                int nextX = x + item->sizeHint().width() + spacing();
                if (item->widget() && item->widget()->property("isSeparator").toBool()) {
                    x = rect.x();
                    y = y + lineHeight + spacing();
                    nextX = x + rect.width();
                    w = rect.width();
                    lineHeight = 0;
                } else if (nextX - spacing() > rect.right() && lineHeight > 0) {
                    x = rect.x();
                    y = y + lineHeight + spacing();
                    nextX = x + w + spacing();
                    lineHeight = 0;
                }

                if (!testOnly) {
                    item->setGeometry(QRect(x, y, w, h));
                }

                x = nextX;
                lineHeight = qMax(lineHeight, h);
            }
        }
        return y + lineHeight - rect.y();
    }

  private:
    QList<QLayoutItem *> d_itemList;
    bool d_oneLine = false;

    mutable bool d_isCachedMinimumSize = false;
    mutable bool d_isCachedHeightForWidth = true;
    mutable QSize d_cachedMinimumSize;
    mutable QSize d_cachedHeightForWidth;
};

class LayoutToolButton : public QToolButton {
  public:
    LayoutToolButton(QWidget *parent = nullptr): QToolButton(parent)
    {
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        if (toolButtonStyle() == Qt::ToolButtonIconOnly) {
            QToolButton::paintEvent(event);
            return;
        }

        QStylePainter sp(this);
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        const QString strText = opt.text;
        const QIcon icn = opt.icon;

        // draw background
        opt.text.clear();
        opt.icon = QIcon();
        sp.drawComplexControl(QStyle::CC_ToolButton, opt);

        // draw content
        const int nSizeHintWidth = minimumSizeHint().width();

        opt.text = strText;
        opt.icon = icn;
        opt.rect.setWidth(nSizeHintWidth);

        // Those flags are drawn on the previous call to
        // drawn the background. Here we need to clear them
        // if we don't, we have artefacts.
        opt.state.setFlag(QStyle::State_MouseOver, false);
        opt.state.setFlag(QStyle::State_Selected, false);
        opt.state.setFlag(QStyle::State_Off, false);
        opt.state.setFlag(QStyle::State_On, false);
        opt.state.setFlag(QStyle::State_Sunken, false);
        sp.drawComplexControl(QStyle::CC_ToolButton, opt);
    }
};

class ToolBoxScrollArea : public QScrollArea {
  public:
    explicit ToolBoxScrollArea(QWidget *parent): QScrollArea(parent)
    {
    }

    void recalculateWidgetSize()
    {
        if (widget() && widget()->layout()) {
            QSize size(maximumViewportSize().width(),
                       widget()->layout()->heightForWidth(maximumViewportSize().width()));

            if (size.height() > maximumViewportSize().height()) {
                const int ext = style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
                size.setWidth(maximumViewportSize().width() - verticalScrollBar()->sizeHint().width() - ext);

                size.setHeight(widget()->layout()->heightForWidth(size.width()));
            }

            widget()->resize(size);
        }
    }

  protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QScrollArea::resizeEvent(event);
        recalculateWidgetSize();
    }
};

} // namespace

struct ToolBox::Private {
    ToolBoxScrollArea *scrollArea = nullptr;
    QWidget *widget = nullptr;
    ToolBoxLayout *layout = nullptr;
    QAction *toggleInformativeViewAction = nullptr;
    QActionGroup *actionGroup = nullptr;
    QMap<QString, QList<QWidget *>> buttonsInCategory;
    QToolButton *invisibleButton = nullptr;
    QButtonGroup toolsGroup;
    QList<QToolButton *> toolButtons;
    QList<QString> categories;
};

ToolBox::ToolBox(QWidget *parent): QWidget(parent), d(std::make_unique<ToolBox::Private>())
{
    // "invisibleButton" is necessary to allow the button group to not have a default selected item.
    d->invisibleButton = new QToolButton();
    d->invisibleButton->hide();
    d->invisibleButton->setCheckable(true);
    d->invisibleButton->setChecked(true);
    d->toolsGroup.setExclusive(true);
    d->toolsGroup.addButton(d->invisibleButton);

    d->scrollArea = new ToolBoxScrollArea(this);
    d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->scrollArea->setFrameShape(QFrame::NoFrame);

    d->widget = new QWidget(d->scrollArea);
    d->layout = new ToolBoxLayout(d->widget);

    d->layout->setSpacing(0);
    d->layout->setOneLine(Preferences::self()->showText());

    d->actionGroup = new QActionGroup(d->widget);
    d->actionGroup->setExclusive(true);

    d->scrollArea->setWidget(d->widget);
    d->scrollArea->setMinimumWidth(d->widget->minimumSizeHint().width());

    auto *topLayout = new QVBoxLayout();
    topLayout->addWidget(d->scrollArea);
    setLayout(topLayout);

    d->toggleInformativeViewAction = new QAction(tr("Informative View"), this);
    d->toggleInformativeViewAction->setCheckable(true);
    d->toggleInformativeViewAction->setChecked(Preferences::self()->showText());
    QObject::connect(d->toggleInformativeViewAction, &QAction::toggled, this, &ToolBox::setInformativeView);
    addAction(d->toggleInformativeViewAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

ToolBox::~ToolBox() = default;

QToolButton *ToolBox::createToolButton(const QString& category, QAction *action)
{
    auto *button = new LayoutToolButton(this);
    if (Preferences::self()->showText()) {
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    } else {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    button->setAutoRaise(true);
    button->setIconSize(QSize(22, 22));
    button->setDefaultAction(action);
    button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    d->toolButtons.append(button);
    d->layout->addWidget(button);
    d->buttonsInCategory[category].append(button);
    applyStyleSheetHack(button);
    return button;
}

QToolButton *ToolBox::createToolButton(const QString& category, lvtqtc::ITool *tool)
{
    auto *btn = createToolButton(category, tool->action());
    connect(tool, &lvtqtc::ITool::deactivated, this, [category, this] {
        d->invisibleButton->setChecked(true);
    });
    d->toolsGroup.addButton(btn);
    return btn;
}

void ToolBox::createGroup(const QString& category)
{
    auto *action = new QAction(this);
    action->setSeparator(true);
    action->setObjectName(category);

    QWidget *sep = new Separator(category, this);
    d->actionGroup->addAction(action);
    d->layout->addWidget(sep);
    d->buttonsInCategory[category].append(sep);
}

void ToolBox::setInformativeView(bool isActive)
{
    Preferences::self()->setShowText(isActive);
    for (QToolButton *button : qAsConst(d->toolButtons)) {
        button->setToolButtonStyle(isActive ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    }
    d->layout->setOneLine(isActive);
    d->scrollArea->recalculateWidgetSize();
}

void ToolBox::hideElements(const QString& category)
{
    for (auto *btn : qAsConst(d->buttonsInCategory[category])) {
        btn->setVisible(false);
    }
    d->scrollArea->recalculateWidgetSize();
}

void ToolBox::showElements(const QString& category)
{
    for (auto *btn : qAsConst(d->buttonsInCategory[category])) {
        btn->setVisible(true);
    }
    d->scrollArea->recalculateWidgetSize();
}

QToolButton *ToolBox::getButtonNamed(const std::string& title) const
{
    auto it = std::find_if(d->toolButtons.begin(), d->toolButtons.end(), [&title](QToolButton *btn) {
        auto txt = btn->text().toStdString();
        return txt == title;
    });
    if (it == d->toolButtons.end()) {
        return nullptr;
    }
    return *it;
}
