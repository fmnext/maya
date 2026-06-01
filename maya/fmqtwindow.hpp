#pragma once

#ifndef FMQTWINDOW_H
#define FMQTWINDOW_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#include <Windows.h>
#include <maya/MQtUtil.h>

const float DEFAULT_DPI = 96.0;

class FMQtWindow
{
public:
    FMQtWindow() = default;

    ~FMQtWindow() = default;

    static QWidget* mainWindow();

    static int dpiScale(int size);
    static float dpiScale(float size);

private:
    
};

#endif //Q_OS_WIN

#endif // FMQTWINDOW_H