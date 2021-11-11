#ifndef __LOG_H_
#define __LOG_H_

#include<fstream>
#include<string>
using namespace std;

class Log {
    string logFile;
public:
    void setFileName(string fileName);
    void printLog(string data);
    void clearLog();
};

void Log::setFileName(string fileName) {
    logFile = fileName;
}

void Log::printLog(string data) {
    ofstream ofs;
    ofs.open(logFile, ofstream::out | ofstream::app);
    ofs << data;
    ofs.close();
}

void Log::clearLog() {
    std::ofstream ofs;
    ofs.open(logFile, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

#endif