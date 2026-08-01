#include "ui/onboarding/FirstRunChecklistWidget.h"

#include "ui/onboarding/FirstRunState.h"
#include "runtime/RuntimeBackendType.h"
#include "ui/desktopapp/ZaryaFormControls.h"

#include <QVBoxLayout>

namespace zarya {

FirstRunChecklistWidget::FirstRunChecklistWidget(QWidget* parent)
    : QWidget(parent)
{
    m_body = new ZaryaBodyText({}, this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_body);
}

void FirstRunChecklistWidget::updateFromState(const FirstRunState& state, int profileCount,
                                              bool xrayInstalled, const QString& xrayVersion)
{
    const QString coreLine =
        xrayInstalled
            ? tr("Core: Xray installed (%1)")
                  .arg(xrayVersion.isEmpty() ? tr("unknown version") : xrayVersion)
            : tr("Core: Xray missing");
    const QString profilesLine = profileCount > 0
        ? tr("Profiles: %1 profile(s)").arg(profileCount)
        : tr("Profiles: none");
    const QString runtimeLine =
        state.runtimeMode == RuntimeMode::TunSingBoxExperimental
            ? tr("Runtime: Experimental TUN via sing-box")
            : tr("Runtime: System proxy via Xray");

    m_body->setText(tr("%1\n%2\nRouting: selected\nDNS: selected\n%3")
                        .arg(coreLine, profilesLine, runtimeLine));
}

} // namespace zarya
