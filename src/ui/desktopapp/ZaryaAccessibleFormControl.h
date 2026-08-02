#pragma once

class QString;

namespace zarya {

class ZaryaAccessibleFormControl {
public:
    virtual ~ZaryaAccessibleFormControl() = default;
    virtual void setAccessibleLabel(const QString& label) = 0;
};

} // namespace zarya
