#include "LoggerManager.h"
#include <algorithm>
#include "CharUtilities.h"
#if defined(LINUX)
#include <climits>
#endif
#include "Configuration.h"
#include "LogData.h"
#include "Logger.h"

namespace Sys
{
    namespace Logging
    {
        int getThreadCount(ConcurrencyLevel level);
        Sys::Logging::LoggerManager::LoggerManager(ConcurrencyLevel level) : LoggerManager(getThreadCount(level))
        {
        }
        LoggerManager::LoggerManager(int numOfThreads)
                : logNumber(0),
                  numberOfWaitingLogs(0),
                  isExit(false),
                  processLog(),
                  outputProcess(),
                  workerThreads(numOfThreads),
                  pData(),
                  ipData(),
                  oData()
        {
            isOutputRun.clear();
            if (numOfThreads > 0) {
                workerThreads[0] = std::thread([&]{ printerWorker(); });
                for (size_t i = 1; i < numOfThreads; i++)
                {
                    workerThreads[i] = std::thread([&]{ processWorker(); });
                }
            }
        }
        LoggerManager::LoggerManager(LoggerManager &&)
        {
        }
        Sys::Logging::LoggerManager::~LoggerManager()
        {
            printf("m1");
            isExit = true;
            for (size_t i = 0; i < workerThreads.size(); i++)
            {
                pData.enqueue(nullptr);
            }
            for (auto &workers : workerThreads)
            {
                if (workers.joinable())
                {
                    workers.join();
                }
            }
            printf("m2");
            while (!pData.isEmpty())
            {
                LogData *data = pData.dequeue();
                if (data != nullptr)
                {
                    addToOutput(data);
                    delete data;
                }
            }
            printf("m3");
            printAllToOutput();
            printf("m4");
        }
        void Sys::Logging::LoggerManager::log(Configuration *config,
                                              const UTF8 *level,
                                              const UTF8 *msg,
                                              const UTF8 *name,
                                              bool writeImmediately) const
        {
            // if should write immediately or the level write immediately
            bool isWriteImmediately = writeImmediately || !workerThreads.size() ||
                                      !(std::find_if(config->getImmLevels().cbegin(),
                                                     config->getImmLevels().cend(),
                                                     [level](const String &str) {
                                                         return !strcmp(level, str.c_str());
                                                     }) == config->getImmLevels().cend());
            log(LogData(config, msg, level, name, isWriteImmediately, logNumber++));
        }
        void LoggerManager::addToOutput(LogData *logData) const
        {
            String msg;
            bool passToOutput = processLog.processLog(*logData, msg);
            if (passToOutput)
            {
                oData.enqueue(new LogOutput(logData->getConfig(),
                                            std::move(msg),
                                            logData->getWriteImmediately(),
                                            logData->getLogNumber()));
            }
            else
            {
                logData->getConfig()->removeRef();
            }
        }
        void LoggerManager::printerWorker()
        {
            LogData *data;
            do {
                while (data = ipData.dequeue())
                {
                    addToOutput(data);
                    delete data;
                }
                printAllToOutput();
                data = pData.dequeueW();
                if (data)
                {
                    addToOutput(data);
                    delete data;
                }
            } while (!isExit && data != nullptr);
            printAllToOutput();
        }
        void LoggerManager::processWorker()
        {
            LogData *data;
            do {
                data = pData.dequeueW();
                if (data) {
                    addToOutput(data);
                    delete data;
                }
            } while (!isExit && data != nullptr);
        }
        void Sys::Logging::LoggerManager::log(LogData&& logData) const
        {
            if (!workerThreads.size())
            {
                String msg;
                bool passToOutput = processLog.processLog(logData, msg);
                if (passToOutput)
                {
                    outputProcess.outputLog(LogOutput(logData.getConfig(),
                        std::move(msg),
                        logData.getWriteImmediately(),
                        logData.getLogNumber()));
                }
                else
                {
                    logData.getConfig()->removeRef();
                }
            }
            else
            {
                while (numberOfWaitingLogs > logData.getConfig()->getMaxWaitingLogs())
                {
                    LogData* data;
                    data = this->pData.dequeue();
                    if (data)
                    {
                        addToOutput(data);
                        delete data;
                    }
                    else
                    {
                        printAllToOutput();
                    }
                }
                ++numberOfWaitingLogs;
                LogData* pLogData = new LogData(std::move(logData));
                if (logData.getWriteImmediately())
                {
                    ipData.enqueue(pLogData);
                    pData.enqueue(nullptr);// to wake up worker thread
                }
                else
                {
                    pData.enqueue(pLogData);
                }
            }
        }
        void Sys::Logging::LoggerManager::log(Configuration *config,
                                              const UTF8 *level,
                                              const std::function<UTF8 *()> &msg,
                                              const UTF8 *name,
                                              bool writeImmediately) const
        {
            this->log(config, level, msg(), name, writeImmediately);
        }
        void LoggerManager::printAllToOutput() const
        {
            if (!oData.isEmpty() && !isOutputRun.test_and_set())
            {
                while (!oData.isEmpty())
                {
                    LogOutput *logOutput = oData.dequeue();
                    outputProcess.outputLog(std::move(*logOutput));
                    delete logOutput;
                    --numberOfWaitingLogs;
                }
                isOutputRun.clear();
            }
        }
        int getThreadCount(ConcurrencyLevel level)
        {
            unsigned int hardwareThread = std::thread::hardware_concurrency();
            unsigned int count;
            switch (level)
            {
                case Sys::Logging::ConcurrencyLevel::NONE:count = 0;
                    break;
                case Sys::Logging::ConcurrencyLevel::ONE:count = 1;
                    break;
                case Sys::Logging::ConcurrencyLevel::VERY_LOW:count = hardwareThread / 8 + 1;
                    break;
                case Sys::Logging::ConcurrencyLevel::LOW:count = hardwareThread / 4 + 1;
                    break;
                case Sys::Logging::ConcurrencyLevel::MEDIUM:count = hardwareThread > 3 ? hardwareThread / 2 : 1;
                    break;
                case Sys::Logging::ConcurrencyLevel::HIGH:
                    count = hardwareThread > 4 ? hardwareThread > 6 ? (unsigned int)(hardwareThread /
                                                                                     1.7) : hardwareThread -
                                                                                            2 : 3;
                    break;
                case Sys::Logging::ConcurrencyLevel::VERY_HIGH:
                    count = hardwareThread > 5 ? hardwareThread > 8 ? (unsigned int)(hardwareThread /
                                                                                     1.4) : hardwareThread - 1 : 4;
                    break;
                case Sys::Logging::ConcurrencyLevel::ALL:count = UINT_MAX;
                    break;
            }
            return std::min(count, hardwareThread);
        }
    }
}
