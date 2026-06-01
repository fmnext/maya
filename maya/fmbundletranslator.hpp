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

#pragma once
#include <fstream>
#include <iostream>
#include <ios>
#include <regex>
#include <QPointer>
#include <QFile>

#include <maya/MPxFileTranslator.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MFileIO.h>
#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MComputation.h>
#include <maya/MNamespace.h>
#include <maya/MQtUtil.h>

#include "fmoptionsdialog.h"
#include "MeshResolver.hpp"

//This is the backbone for creating a MPxFileTranslator
class FMBundleTranslator : public MPxFileTranslator {
public:

    //Constructor
    FMBundleTranslator() {};
    //Destructor
    ~FMBundleTranslator() override {};

    //This tells maya that the translator can read files.
    //Basically, you can import or load with your translator.
    bool haveReadMethod() const override { return true; }

    //This tells maya that the translator can write files.
    //Basically, you can export or save with your translator.
    bool haveWriteMethod() const override { return false; }

    //If this method returns true, and the lep file is referenced in a scene, the write method will be
    //called when a write operation is performed on the parent file.  This use is for users who wish
    //to implement a custom file referencing system.
    //For this example, we will return false as we will use Maya's file referencing system.
    bool haveReferenceMethod() const override { return false; }

    //If this method returns true, it means we support namespaces.
    bool haveNamespaceSupport() const override { return true; }

    //This method is used by Maya to create instances of the translator.
    static void* creator();

    //This returns the default extension ".json" in this case.
    MString defaultExtension() const override;

    //If this method returns true it means that the translator can handle opening files 
    //as well as importing them.
    //If the method returns false then only imports are handled. The difference between 
    //an open and an import is that the scene is cleared(e.g. 'file -new') prior to an 
    //open, which may affect the behaviour of the translator.
    bool canBeOpened() const override { return true; }

    //This function is called by maya when import or open is called.
    MStatus reader(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override;

    //This function is called by maya when export or save is called.
    MStatus writer(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override { return MStatus::kNotImplemented; };
};