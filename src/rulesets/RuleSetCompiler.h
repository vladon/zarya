#pragma once

#include <QString>

namespace zarya {

struct RuleSetCompileResult {
    bool success = false;
    QString output;
    QString errorMessage;
};

class RuleSetCompiler {
public:
    RuleSetCompileResult compileJsonToSrs(const QString& singBoxPath,
                                            const QString& sourceJsonPath,
                                            const QString& outputSrsPath,
                                            int timeoutMs = 30000) const;
};

} // namespace zarya
