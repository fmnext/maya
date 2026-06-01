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

#include "fmbundletranslator.hpp"

//Creates one instance of the ForzaTechModelTranslator
void* FMBundleTranslator::creator()
{
    return new FMBundleTranslator();
}

MStatus FMBundleTranslator::reader(const MFileObject& file, const MString& options, MPxFileTranslator::FileAccessMode mode)
{
    MStatus status;
    std::string filename = file.expandedFullName().asChar();

    auto data = fmnext::BundleReader(filename);

    if (data.Init())
    {
		QPointer<FMOptionsDialog> dialog;
		if (dialog.isNull())
		{
			dialog = new FMOptionsDialog(MQtUtil::mainWindow());
			dialog->setFixedSize(QSize(MQtUtil::dpiScale(400), MQtUtil::dpiScale(200)));
			dialog->setModelData(filename);
			dialog->show();
		}
    }

    return MStatus::kSuccess;
}

MString FMBundleTranslator::defaultExtension() const
{
    return "modelbin";
}
