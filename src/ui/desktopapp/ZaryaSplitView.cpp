#include "ui/desktopapp/ZaryaSplitView.h"

#include "ui/theme/ThemeManager.h"
#include "ui/theme/ThemeTokens.h"

#include "ui/rp_widget.h"

#include <QDataStream>
#include <QEnterEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace zarya {
namespace {

constexpr auto kHandleHeight = 9;
constexpr auto kKeyboardStep = 16;
constexpr auto kLargeKeyboardStep = 64;
constexpr quint32 kStateMagic = 0x5A535056; // ZSPV
constexpr quint32 kStateVersion = 1;
constexpr qint32 kQSplitterStateMagic = 0xff;

class SplitHandle final : public Ui::RpWidget {
public:
    SplitHandle(
        QWidget* parent,
        std::function<int()> currentExtent,
        std::function<int()> maximumExtent,
        std::function<void(int)> setExtent)
        : Ui::RpWidget(parent)
        , m_currentExtent(std::move(currentExtent))
        , m_maximumExtent(std::move(maximumExtent))
        , m_setExtent(std::move(setExtent))
    {
        setCursor(Qt::SizeVerCursor);
        setFocusPolicy(Qt::StrongFocus);
        setAccessibleName(ZaryaSplitView::tr("Resize profiles and logs"));
        setAccessibleDescription(
            ZaryaSplitView::tr("Use Up and Down arrow keys to resize"));
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        const ThemeTokens tokens = ThemeManager::instance().tokens();
        QPainter painter(this);
        const QColor color = hasFocus() || m_hovered ? tokens.accent : tokens.border;
        const int thickness = hasFocus() ? 2 : 1;
        const int y = (height() - thickness) / 2;
        painter.fillRect(0, y, width(), thickness, color);
    }

    void enterEventHook(QEnterEvent* event) override
    {
        Ui::RpWidget::enterEventHook(event);
        m_hovered = true;
        update();
    }

    void leaveEventHook(QEvent* event) override
    {
        Ui::RpWidget::leaveEventHook(event);
        m_hovered = false;
        update();
    }

    void focusInEvent(QFocusEvent* event) override
    {
        Ui::RpWidget::focusInEvent(event);
        update();
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        Ui::RpWidget::focusOutEvent(event);
        update();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartY = event->globalPosition().y();
            m_dragStartExtent = m_currentExtent();
            event->accept();
            return;
        }
        Ui::RpWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging) {
            m_setExtent(m_dragStartExtent
                        + qRound(event->globalPosition().y() - m_dragStartY));
            event->accept();
            return;
        }
        Ui::RpWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && m_dragging) {
            m_dragging = false;
            event->accept();
            return;
        }
        Ui::RpWidget::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        int extent = m_currentExtent();
        switch (event->key()) {
        case Qt::Key_Up:
            extent -= kKeyboardStep;
            break;
        case Qt::Key_Down:
            extent += kKeyboardStep;
            break;
        case Qt::Key_PageUp:
            extent -= kLargeKeyboardStep;
            break;
        case Qt::Key_PageDown:
            extent += kLargeKeyboardStep;
            break;
        case Qt::Key_Home:
            extent = 0;
            break;
        case Qt::Key_End:
            extent = m_maximumExtent();
            break;
        default:
            Ui::RpWidget::keyPressEvent(event);
            return;
        }
        m_setExtent(extent);
        event->accept();
    }

private:
    std::function<int()> m_currentExtent;
    std::function<int()> m_maximumExtent;
    std::function<void(int)> m_setExtent;
    qreal m_dragStartY = 0;
    int m_dragStartExtent = 0;
    bool m_dragging = false;
    bool m_hovered = false;
};

} // namespace

ZaryaSplitView::ZaryaSplitView(QWidget* first, QWidget* second, QWidget* parent)
    : QWidget(parent)
    , m_first(first)
    , m_second(second)
{
    Q_ASSERT(m_first);
    Q_ASSERT(m_second);
    m_first->setParent(this);
    m_second->setParent(this);
    m_handle = new SplitHandle(
        this,
        [this] { return m_first ? m_first->height() : 0; },
        [this] { return availableExtent(); },
        [this](int extent) { setFirstExtent(extent); });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, m_handle,
            [handle = m_handle] { handle->update(); });
    relayout();
}

void ZaryaSplitView::setSizes(const QList<int>& sizes)
{
    if (sizes.size() < 2) {
        return;
    }
    const qint64 first = std::max(sizes.at(0), 0);
    const qint64 second = std::max(sizes.at(1), 0);
    const qint64 total = first + second;
    if (total <= 0) {
        return;
    }
    m_ratio = static_cast<double>(first) / static_cast<double>(total);
    relayout();
}

QList<int> ZaryaSplitView::sizes() const
{
    return {m_first ? m_first->height() : 0, m_second ? m_second->height() : 0};
}

QByteArray ZaryaSplitView::saveState() const
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    const QList<int> current = sizes();
    stream << kStateMagic << kStateVersion << qint32(current.value(0))
           << qint32(current.value(1));
    return result;
}

bool ZaryaSplitView::restoreState(const QByteArray& state)
{
    QDataStream stream(state);
    stream.setVersion(QDataStream::Qt_6_0);
    quint32 magic = 0;
    quint32 version = 0;
    qint32 first = 0;
    qint32 second = 0;
    stream >> magic >> version;
    if (stream.status() == QDataStream::Ok && magic == kStateMagic
        && version == kStateVersion) {
        stream >> first >> second;
        if (stream.status() == QDataStream::Ok && first >= 0 && second >= 0
            && first + second > 0) {
            setSizes({first, second});
            return true;
        }
        return false;
    }

    // Preserve the user's existing position when migrating the legacy split view.
    QDataStream legacy(state);
    legacy.setVersion(QDataStream::Qt_5_0);
    qint32 legacyMagic = 0;
    qint32 legacyVersion = 0;
    QList<int> legacySizes;
    legacy >> legacyMagic >> legacyVersion >> legacySizes;
    if (legacy.status() == QDataStream::Ok && legacyMagic == kQSplitterStateMagic
        && legacyVersion == 1 && legacySizes.size() >= 2) {
        setSizes({legacySizes.at(0), legacySizes.at(1)});
        return true;
    }
    return false;
}

void ZaryaSplitView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void ZaryaSplitView::setFirstExtent(int extent)
{
    const int available = availableExtent();
    if (available <= 0) {
        return;
    }
    m_ratio = static_cast<double>(std::clamp(extent, 0, available)) / available;
    relayout();
}

void ZaryaSplitView::relayout()
{
    if (!m_first || !m_second || !m_handle) {
        return;
    }
    const int available = availableExtent();
    const int minimumFirst = std::min(m_first->minimumHeight(), available);
    const int minimumSecond = std::min(m_second->minimumHeight(), available);
    int first = qRound(m_ratio * available);
    if (minimumFirst + minimumSecond <= available) {
        first = std::clamp(first, minimumFirst, available - minimumSecond);
    } else {
        first = std::clamp(first, 0, available);
    }
    const int second = available - first;
    m_first->setGeometry(0, 0, width(), first);
    m_handle->setGeometry(0, first, width(), kHandleHeight);
    m_second->setGeometry(0, first + kHandleHeight, width(), second);
    m_handle->raise();
}

int ZaryaSplitView::availableExtent() const
{
    return std::max(height() - kHandleHeight, 0);
}

} // namespace zarya
