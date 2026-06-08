#include "ui/onboarding/FirstRunChecklistWidget.h"

#include "ui/onboarding/FirstRunState.h"
#include "runtime/RuntimeBackendType.h"

#include <QLabel>
#include <QVBoxLayout>

namespace zarya {

FirstRunChecklistWidget::FirstRunChecklistWidget(QWidget* parent)
    : QWidget(parent)
{
    m_body = new QLabel(this);
    m_body->setWordWrap(true);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_body);
}

void FirstRunChecklistWidget::updateFromState(const FirstRunState& state, int profileCount,
                                              bool xrayInstalled, const QString& xrayVersion)
{
    const QString coreLine =
        xrayInstalled
            ? QStringLiteral("Core: Xray installed (%1)").arg(xrayVersion.isEmpty() ? QStringLiteral("unknown version") : xrayVersion)
            : QStringLiteral("Core: Xray missing");
    const QString profilesLine = profileCount > 0
                                     ? QStringLiteral("Profiles: %1 profile(s)").arg(profileCount)
                                     : QStringLiteral("Profiles: none");
    const QString runtimeLine =
        state.runtimeMode == RuntimeMode::TunSingBoxExperimental
            ? QStringLiteral("Runtime: Experimental TUN via sing-box")
            : QStringLiteral("Runtime: System proxy via Xray");

    m_body->setText(QStringLiteral("%1\n%2\nRouting: selected\nDNS: selected\n%3")
                        .arg(coreLine, profilesLine, runtimeLine));
}

} // namespace zarya
