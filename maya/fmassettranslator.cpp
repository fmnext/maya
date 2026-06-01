// ===========================================================================
// Copyright (c) 2024 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
//-
// ==========================================================================
//
// ==========================================================================
//+

#include "fmassettranslator.hpp"

//Creates one instance of the FMAssetTranslator
void* FMAssetTranslator::creator()
{
    return new FMAssetTranslator();
}

MStatus FMAssetTranslator::reader(const MFileObject& file, const MString& options, MPxFileTranslator::FileAccessMode mode)
{
    MStatus status;
    std::string filename = file.expandedFullName().asChar();

    QApplication* app = qApp;

    QPointer<FMMainWindow> qPtrMainWindow;

    if (qPtrMainWindow.isNull())
    {
        qPtrMainWindow = new FMMainWindow(FMQtWindow::mainWindow());

#if QT_VERSION <= QT_VERSION_CHECK(5, 15, 2)
        qPtrMainWindow->setWindowTitle("Forza Enhanced Qt5");
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        qPtrMainWindow->setWindowTitle("Forza Enhanced Qt6");
#endif

        DCCManager::SetImporterFrameRate();

        QFile f(":/assets/custom.qss");
        if (f.open(QFile::ReadOnly))
        {
            qPtrMainWindow->setStyleSheet(f.readAll());
        }

        qPtrMainWindow->setModel(filename);
        qPtrMainWindow->show();
    }

    return MS::kSuccess;
}

MString FMAssetTranslator::defaultExtension() const
{
    return "zip";
}
