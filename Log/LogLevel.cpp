#include "LogLevel.h"
#include "CharUtilities.h"
namespace Sys
{
    namespace Logging
    {
        UTF8* LogLevel::levelStr = "@`~";
        const UTF8 * LogLevel::getData(const UTF8 * pattern) const
        {
            return levelStr;
        }
    }
}