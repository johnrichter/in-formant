#ifndef MAIN_CONTEXT_SESSIONFILE_H
#define MAIN_CONTEXT_SESSIONFILE_H

#include <QString>
#include <QStringList>

namespace Main {

    // The name of the scratch file of the sound of a session. The process number is in the name.
    // A later run uses the number to find the files of a run that ended in a crash. The tail
    // XXXXXX becomes a random text when QTemporaryFile creates the file.
    inline QString sessionFileTemplate(qint64 pid)
    {
        return QStringLiteral("informant-session-%1-XXXXXX.raw").arg(pid);
    }

    // The process number in the name of a scratch file, from the name without the extension.
    // The result is false for a name from an older version, which has no process number.
    inline bool sessionFilePid(const QString &baseName, qint64 &pid)
    {
        const QStringList parts = baseName.split('-');
        if (parts.size() < 4) return false;
        bool ok = false;
        const qint64 value = parts[2].toLongLong(&ok);
        if (ok) pid = value;
        return ok;
    }

}

#endif // MAIN_CONTEXT_SESSIONFILE_H
