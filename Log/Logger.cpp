#include "Logger.h"
#include <algorithm>
#include "LogStatement.h"
#include "PreMessage.h"
#include "ILogStatement.h"
#include "LoggerData.h"
#include "ILoggerWriter.h"
#include "LogData.h"
#include "Configuration.h"
#include "CharUtilities.h"
namespace Sys
{
    namespace Logging
    {
#define MAX_LEVEL   255
        void Logger::log(const char * level, const UTF8 * msg, String & name, bool writeImmediately) const
        {
            String levelS(level);
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            this->log(config->getLevels().at(levelS), msg, levelS, name, writeImmediately);
            config->removeRef();
        }
        void Logger::log(unsigned int level, const UTF8 *msg, const String &sLevel, const String &name, bool writeIm) const
        {
            if (checkLevel(level))
            {
                Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
                writeIm |= std::find_if(config->getImmLevels().cbegin(), config->getImmLevels().cend(), [&sLevel](const String& str)
                {
                    return sLevel == str;
                }) != config->getImmLevels().cend();
                loggerManager.log(config, sLevel.c_str(), msg, name.c_str(), writeIm);
            }
        }
        void Logger::log(unsigned short level, const UTF8 * msg) const
        {
            if (checkLevel(level))
            {
                int maxDis = 0x7fffffff;
                const char* lv;
                Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
                auto str = std::find_if(config->getLevels().cbegin(), config->getLevels().cend(), [level, &maxDis, &lv](const auto& lev)
                {
                    int dis;
                    if (lev.second == level)
                    {
                        lv = lev.first.c_str();
                        return true;
                    }
                    else if (lev.second > level)
                    {
                        dis = lev.second - level;
                    }
                    else
                    {
                        dis = level - lev.second;
                    }
                    if (maxDis > dis)
                    {
                        maxDis = dis;
                        lv = lev.first.c_str();
                    }
                    return false;
                });
                lv = createStr(lv);
                config->removeRef();
                this->log(lv, msg);
                delete[] lv;
            }
        }
        void Logger::log(const String & level, const UTF8 * msg) const
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            this->log(config->getLevels().at(level), msg, level, name, false);
            config->removeRef();
        }
        void Logger::log(const char * level, const UTF8 * msg) const
        {
            this->log(String(level), msg);
        }
        void Logger::log(const char * level, const UTF8 * msg, const String & name) const
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            this->log(config->getLevels().at(String(level)), msg, String(level), name, false);
            config->removeRef();
        }
        void Logger::log(const char * level, const UTF8 * msg, bool writeImmediately) const
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            this->log(config->getLevels().at(String(level)), msg, String(level), name, writeImmediately);
            config->removeRef();
        }
        void Logger::log(unsigned short level, const std::function<String()>& msg) const
        {
            this->log(level, msg().c_str());
        }
        void Logger::log(const String & level, const std::function<String()>& msg) const
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            this->log(config->getLevels().at(level), msg);
            config->removeRef();
        }
        void Logger::log(const char * level, const std::function<String()>& msg) const
        {
            this->log(String(level), msg);
        }
        void Logger::log(const char * level, const std::function<String()>& msg, const String & name) const
        {
            this->log(level, msg().c_str(), name);
        }
        void Logger::log(const char * level, const std::function<String()>&msg, bool writeImmediately) const
        {
            this->log(level, msg().c_str(), writeImmediately);
        }
        void Logger::debug(const UTF8 * msg) const
        {
            this->log("debug", msg);
        }
        void Logger::fatal(const UTF8 * msg) const
        {
            this->log("fatal", msg);
        }
        void Logger::error(const UTF8 * msg) const
        {
            this->log("error", msg);
        }
        void Logger::info(const UTF8 * msg) const
        {
            this->log("info", msg);
        }
        void Logger::warn(const UTF8 * msg) const
        {
            this->log("warn", msg);
        }
        void Logger::trace(const UTF8 * msg) const
        {
            this->log("trace", msg);
        }
        void Logger::debug(const UTF8 * msg, bool writeImmediately) const
        {
            this->log("debug", msg, writeImmediately);
        }
        void Logger::fatal(const UTF8 * msg, bool writeImmediately) const
        {
            this->log("fatal", msg, writeImmediately);
        }
        void Logger::error(const UTF8 * msg, bool writeImmediately) const
        {
            this->log("error", msg, writeImmediately);
        }
        void Logger::info(const UTF8 * msg, bool writeImmediately) const
        {
            this->log("info", msg, writeImmediately);
        }
        void Logger::warn(const UTF8 * msg, bool writeImmediately) const
        {
            this->log("warn", msg, writeImmediately);
        }
        void Logger::trace(const UTF8 * msg, bool writeImmediately) const
        {
            this->log("trace", msg, writeImmediately);
        }
        void Logger::debug(const UTF8 * msg, const String & name) const
        {
            this->log("debug", msg, name);
        }
        void Logger::fatal(const UTF8 * msg, const String & name) const
        {
            this->log("fatal", msg, name);
        }
        void Logger::error(const UTF8 * msg, const String & name) const
        {
            this->log("error", msg, name);
        }
        void Logger::info(const UTF8 * msg, const String & name) const
        {
            this->log("info", msg, name);
        }
        void Logger::warn(const UTF8 * msg, const String & name) const
        {
            this->log("warn", msg, name);
        }
        void Logger::trace(const UTF8 * msg, const String & name) const
        {
            this->log("trace", msg, name);
        }
        void Logger::setLogFilter(const LogFilter & logFilter)
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            config->setFilter(logFilter);
            config->removeRef();
        }
        bool Logger::checkLevel(unsigned int level) const
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            unsigned char l = config->getLevel();
            config->removeRef();
            return l >= level;
        }
        void Logger::reloadConfig(const ILoggerData &logger, int loggerNumber)
        {
            Configuration* conf;
            bool removeWriter = true;
            {
                std::vector<String> immLevels = std::move(logger.getImmediateLevels(loggerNumber));
                std::map<String, int> levels;
                levels[String("debug")] = 170;
                levels[String("trace")] = 200;
                levels[String("info")] = 140;
                levels[String("warn")] = 100;
                levels[String("error")] = 50;
                levels[String("fatal")] = 20;
                std::vector<ILoggerData::CustomLevel> customs = logger.getCustomLevel();
                for (const auto& levelCustom : customs)
                {
                    levels[levelCustom.name] = levelCustom.level;
                }
                ILoggerWriter *writer = nullptr;
                const auto& stream = logger.getStreams(loggerNumber);
                if (fileName == nullptr || strcmp(fileName, stream.fileParam.fileName) != 0)
                {
                    writer = logger.getStreams(loggerNumber).getLoggerWriter();
                    if (stream.streamType == ILoggerData::StreamType::FILE)
                    {
                        delete[]fileName;
                        fileName = createStr(stream.fileParam.fileName);
                    }
                }
                if (writer == nullptr&&config.load(std::memory_order_acquire) != nullptr)
                {
                    writer = const_cast<ILoggerWriter*>(config.load(std::memory_order_acquire)->getLoggerWriter());
                    removeWriter = false;
                }
                String lev = logger.getLevel(loggerNumber);
                const auto& lvl = levels.find(lev);
                unsigned char level;
                if (lvl == levels.cend())
                {
                    level = atoi(lev.c_str());
                }
                else
                {
                    level = (*lvl).second;
                }
                if (level > MAX_LEVEL)
                {
                    level = MAX_LEVEL;
                }
                PreMessage *p = new PreMessage(logger.getPaterens(loggerNumber).c_str(), logger.getLoggerName(loggerNumber).c_str());
                LogFilter f = logger.getLogFilter(loggerNumber);
                conf = new Configuration(p, writer, level, std::move(levels), std::move(immLevels), std::move(f),logger.getMaxWaitingLogs(loggerNumber));
                conf = config.exchange(conf, std::memory_order_release);
            }
            if (conf != nullptr)
            {
                if (!removeWriter)
                {
                    conf->clearRemoveWriter();
                }
                conf->removeRef();
            }
        }
        LogFilter Logger::getLogFilter() const
        {
            Configuration* config = this->config.load(std::memory_order_acquire)->addRef();
            LogFilter ret = config->startUsingFilter();
            config->endUsingFilter();
            config->removeRef();
            return ret;
        }
        Logger::Logger(ConcurrencyLevel level) :config(), fileName(nullptr), logNumber(0), loggerManager(level)
        {
        }
        Logger::Logger(int concurrencyLevel) : config(), fileName(nullptr), logNumber(0), loggerManager(concurrencyLevel)
        {
        }
        Logger::Logger(const ILoggerData & logger, int loggerNumber, ConcurrencyLevel level) : Logger(level)
        {
            reloadConfig(logger, loggerNumber);
        }
        Logger::Logger(const ILoggerData &logger, int loggerNumber, const String &name, ConcurrencyLevel level) : Logger(logger, loggerNumber, level)
        {
            this->name = name;
        }
        Logger::Logger(const ILoggerData & logger, int loggerNumber, int concurrencyLevel) : Logger(concurrencyLevel)
        {
            reloadConfig(logger, loggerNumber);
        }
        Logger::Logger(const ILoggerData & logger, int loggerNumber, const String & name, int concurrencyLevel) : Logger(logger, loggerNumber, concurrencyLevel)
        {
            this->name = name;
        }
        Logger::Logger(Logger &&copy) : config(copy.config.load(std::memory_order_acquire)), name(std::move(copy.name)), loggerManager(std::move(copy.loggerManager))
        {
            copy.config.store(nullptr, std::memory_order_relaxed);
        }
        Logger::~Logger()
        {
            config.load()->removeRef();
            delete[] fileName;
        }
    }
}