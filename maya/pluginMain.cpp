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

#include <maya/MFnPlugin.h>
#include "preinclude.hpp"
#include "fmassettranslator.hpp"
#include "fmbundletranslator.hpp"

#define AssetTranslatorName "ForzaTech Asset Importer"
#define BundleTranslatorName "ForzaTech Bundle Importer"

MStatus initializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj, PLUGIN_COMPANY, FT_MAYA_VERSION_STR, "Any");

    status = plugin.registerFileTranslator(AssetTranslatorName, NULL, FMAssetTranslator::creator, NULL, NULL, true);
    status = plugin.registerFileTranslator(BundleTranslatorName, NULL, FMBundleTranslator::creator, NULL, NULL, true);

    if (!status)
    {
        status.perror("registerPlugin");
        return status;
    }

    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus   status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterFileTranslator(AssetTranslatorName);
    status = plugin.deregisterFileTranslator(BundleTranslatorName);

    if (!status)
    {
        status.perror("deregisterPlugin");
        return status;
    }

    return status;
}