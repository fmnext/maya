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
#include "preinclude.hpp"

#include <regex>
#include <array>
#include <any>

#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MFnMesh.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MQuaternion.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVector.h>
#include <maya/MDoubleArray.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <maya/MFnSet.h>
#include <maya/MDagModifier.h>
#include <maya/MFnTransform.h>
#include <maya/MTypeId.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MColorArray.h>
#include <maya/MComputation.h>
#include <maya/MAngle.h>
#include <maya/MDagPath.h>
#include <maya/MFnIkJoint.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnPhongShader.h>
#include <maya/MFnStandardSurfaceShader.h>
#include <maya/MPlugArray.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnComponentListData.h>
#include <maya/MDataHandle.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMeshData.h>
#include <maya/MAnimControl.h>
#include <maya/MTime.h>
#include <maya/MAnimUtil.h>

#include <QTreeWidgetItem>
#include <DirectXMath.h>

#include "granny.h"

#define FT_MAYA_MAJOR_VERSION    2
#define FT_MAYA_MINOR_VERSION    7
#define FT_MAYA_PATCH_VERSION    0
#define FT_MAYA_BUILD_NUMBER     0
#define FT_MAYA_VERSION_VERSION  "2.7.0"
#define FT_MAYA_VERSION_NUMBER   270
#define FT_MAYA_SCM_BRANCH       "Maya"
#define FT_MAYA_SCM_TAGS         "release version-2"
#define FT_MAYA_SCM_DATETIME     "2026-05-30T00:00:00.000Z"

namespace fmnext
{
    struct MaterialInstace
    {
        MaterialInstace() = default;
        MaterialInstace(std::string& path, std::shared_ptr<BundleReader::BundleData> local, std::shared_ptr<BundleReader::BundleData> instance) : path(path), local(local), instace(instance) {}

        std::string path;
        std::shared_ptr<BundleReader::BundleData> local;
        std::shared_ptr<BundleReader::BundleData> instace;
    };

    struct ModelItem
    {
        ModelItem() = default;
        ModelItem(QTreeWidgetItem* item, uint32_t type, std::shared_ptr<SceneReader::CarRenderModel11> model, std::shared_ptr<BundleReader::BundleData> bundle, std::unordered_map<int32_t, MaterialInstace> materials, const std::string& schema) : item(item), type(type), model(model), bundle(bundle), materials(materials), schema(schema) {}

        QTreeWidgetItem* item;
        uint32_t type;
        std::shared_ptr<SceneReader::CarRenderModel11> model;
        std::shared_ptr<BundleReader::BundleData> bundle;
        std::unordered_map<int32_t, MaterialInstace> materials;
        std::string schema;
    };
}

class DCCManager
{
public:
    DCCManager() = default;
	~DCCManager() = default;

    static MStatus getObjectByName_(const MString& name, MObject& object)
    {
        object = MObject::kNullObj;

        MSelectionList sList;
        MStatus status = sList.add(name);
        if (status == MS::kSuccess)
        {
            status = sList.getDependNode(0, object);
        }
        sList.clear();

        return status;
    }

    static MStatus getObjectByName(const MString& name, MObject& object)
    {
        //object = MObject::kNullObj;
        MItDependencyNodes nodesIter;

        for (; !nodesIter.isDone(); nodesIter.next())
        {
            MObject node = nodesIter.thisNode();
            MFnDependencyNode nodeFn(node);

            if (nodeFn.name() == name)
            {
                object = node;

                return MS::kSuccess;
            }
        }

        return MS::kNotFound;
    }

    static MObject createMesh(const std::vector<DirectX::XMFLOAT3>& vertices, const std::vector<uint32_t>& indices, const std::vector<DirectX::XMFLOAT3>& normals, const std::vector<std::vector<DirectX::XMFLOAT2>>& uvs, const std::string& Name, bool useQuads)
    {
        MStatus status;

        MFnMesh meshFn;
        MFnDependencyNode depNodeFn;
        MFloatPointArray vertexArray;
        MVectorArray normalsArray;
        MIntArray polygonCounts, polygonConnects;
        MIntArray uvCounts, uvIds;
        MIntArray vertexList;
        MColorArray vColors;

        int geometry = (useQuads) ? 4 : 3;
        int numVertices = static_cast<int>(vertices.size());
        int numIndices = static_cast<int>(indices.size());
        int numPolygons = static_cast<int>(numIndices / geometry);

        // Vertices
        vertexArray.setLength(numVertices);

        // Normals
        normalsArray.setLength(numVertices);

        // Vertex List
        vertexList.setLength(numVertices);

        for (int i = 0; i < numVertices; ++i)
        {
            vertexArray.set(MFloatPoint(vertices[i].x, vertices[i].z, vertices[i].y), i);
            normalsArray.set(MVector(normals[i].x, normals[i].z, normals[i].y), i);
            vertexList.set(i, i);
        }

        for (int i = 0; i < numIndices; i += geometry)
        {
            int v0 = indices[i + 0];
            int v1 = indices[i + 1];
            int v2 = indices[i + 2];
            int v3 = (geometry == 4) ? indices[i + 3] : 0xffffffff;

            polygonConnects.append(v0);
            polygonConnects.append(v1);
            polygonConnects.append(v2);

            uvIds.append(v0);
            uvIds.append(v1);
            uvIds.append(v2);

            switch (v3)
            {
            case 0xffffffff:
            {
                polygonCounts.append(3);
                uvCounts.append(3);
                break;
            }
            default:
            {
                polygonConnects.append(v3);
                uvIds.append(v3);

                polygonCounts.append(4);
                uvCounts.append(4);
                break;
            }
            }
        }

        std::vector<MFloatArray> uArray(uvs.size());
        std::vector<MFloatArray> vArray(uvs.size());

        for (size_t id = 0; id < uvs.size(); ++id)
        {
            // UVs Set {ID}
            if (!uvs[id].empty())
            {
                uArray[id].setLength((int)uvs[id].size());
                vArray[id].setLength((int)uvs[id].size());

                for (int i = 0; i < uvs[id].size(); ++i)
                {
                    uArray[id].set(uvs[id][i].x, i);
                    vArray[id].set(1 - uvs[id][i].y, i);
                }
            }
        }

        MObject obj = meshFn.create(numVertices, numPolygons, vertexArray, polygonCounts, polygonConnects);

        MStatus normalsetstatus = meshFn.setVertexNormals(normalsArray, vertexList, MSpace::kObject);

        // Get the current UV set name
        MStringArray uvSetsNames;
        meshFn.getUVSetNames(uvSetsNames);
        MString uvSetName = uvSetsNames[0];
        MString initialUvSet = "UVChannel_1";

        // Set the new UV set name
        meshFn.renameUVSet(uvSetName, initialUvSet); // Rename the uv set name
        meshFn.clearUVs(&initialUvSet);

        for (size_t id = 0; id < uvs.size(); ++id)
        {
            // UVs Set {ID}
            if (!uvs[id].empty())
            {
                // Creates a new UV set name
                MString uvSet = "UVChannel_";
                uvSet += std::to_string(id + 1).c_str();

                if (uvSet != initialUvSet)
                {
                    meshFn.createUVSet(uvSet);
                }

                meshFn.setCurrentUVSetName(uvSet);

                MStatus uvsetstatus = meshFn.setUVs(uArray[id], vArray[id], &uvSet);
                MStatus uvassigntatus = meshFn.assignUVs(uvCounts, uvIds);
            }
        }

        // Reset UV Layout selection
        meshFn.setCurrentUVSetName(initialUvSet);

        meshFn.updateSurface();
        //meshFn.setObject(obj);
        depNodeFn.setObject(obj);

        MString updated_name = updateNodeName(Name).c_str();

        depNodeFn.setName(updated_name);

        return obj;
    }

    static MStatus ExecuteCommand(MString mCommand)
    {
        if (!MGlobal::executeCommand(mCommand, false))
        {
            return MStatus::kFailure;
        }
        return MStatus::kSuccess;
    }

    static MStatus objExists(const std::string& mName)
    {
        MItDependencyNodes  nodesIter;

        for (; !nodesIter.isDone(); nodesIter.next())
        {
            MObject node = nodesIter.thisNode();
            MFnDependencyNode nodeFn(node);

            if (nodeFn.name() == mName.c_str())
            {
                return MS::kSuccess;
            }
        }

        return MS::kNotFound;
    }

    static MObject createDagNode(const MString& name)
    {
        MFnDagNode fnDagNode;
        MObject locatorObj;
        locatorObj = fnDagNode.create("transform", name);
        //fnDagNode.setName(name);

        return locatorObj;
    }

    static MStatus deleteDagNode(MDagPath& dagPath)
    {
        MObject obj = dagPath.node();
        return MGlobal::deleteNode(obj);
    }

    static MStatus setNodeTransformation(MObject& locatorObj, DirectX::XMMATRIX xmMatrix)
    {
        MStatus status;
        MFnDagNode fnDagNode;

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        const MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));
        const MQuaternion rotation = MQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat));
        const double scale[3]{ DirectX::XMVectorGetX(outScale), DirectX::XMVectorGetZ(outScale), DirectX::XMVectorGetY(outScale) };

        MQuaternion rotation_inv = rotation.inverse();

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation_inv, MSpace::kTransform);
        transformFn.setScale(scale);

        return status;
    }

    static MStatus setNodeTransforms(MObject& locatorObj, const MVector& position, const MVector& rotation, const MVector& scale)
    {
        MFnTransform transformFn(locatorObj);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        const double scale_scope[3] = { scale.x, scale.y, scale.z };
        const double rotation_scopes[3] = { MAngle(rotation.x, MAngle::kDegrees).asRadians(), MAngle(rotation.y, MAngle::kDegrees).asRadians(), MAngle(rotation.z, MAngle::kDegrees).asRadians() };

        // Degrees to Radians calculation
        //MVector vRotation = rotation * 3.141592 / 180.0;
        //MEulerRotation rotation_scope = MEulerRotation(rotation);

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation_scopes, order);
        transformFn.setScale(scale_scope);

        return MS::kSuccess;
    }

    static MStatus parentObjectByName(const std::string& parent, const std::string& child)
    {
        MObject parent_object;
        MObject child_object;
        MStatus result;

        getObjectByName(parent.c_str(), parent_object);
        getObjectByName(child.c_str(), child_object);

        // Check if both of the objects exists
        if (!parent_object.isNull() && !child_object.isNull())
        {
            MFnDagNode fnDagNode(parent_object);

            // Check if the objects does not have the same name.
            if (fnDagNode.name() == child.c_str())
            {
                result = MStatus::kFailure;
                //MGlobal::displayWarning("Duplicated node: " + child.c_str());
            }
            else
            {
                result = fnDagNode.addChild(child_object);
            }
        }

        return result;
    }

    static MStatus parentObject(MObject& parent, MObject& child)
    {
        MStatus result;

        // Check if both of the objects exists
        if (!parent.isNull() && !child.isNull())
        {
            MFnDagNode fnDagNode(parent);

            result = fnDagNode.addChild(child);
        }

        return result;
    }

    static MObject createJoint(const std::string& mName, const DirectX::XMMATRIX& xmMatrix)
    {
        MStatus status;
        MFnDagNode fnDagNode;
        MObject locatorObj;
        locatorObj = fnDagNode.create("joint", mName.c_str());

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        const MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));
        const MQuaternion rotation = MQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat));
        const double scale[3]{ DirectX::XMVectorGetX(outScale), DirectX::XMVectorGetZ(outScale), DirectX::XMVectorGetY(outScale) };

        MQuaternion rotation_inv = rotation.inverse();

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation_inv, MSpace::kTransform);
        transformFn.setScale(scale);

        const MColor skelColor = MColor(1.0, 0.0, 0.0, 1.0);
        fnDagNode.setObjectColorType(MFnDagNode::kUseRGBColor);
        fnDagNode.setObjectColor(skelColor);

        return locatorObj;
    }

    static MStatus createJoint(std::string& mName, DirectX::XMMATRIX& xmMatrix, MObject& locatorObj)
    {
        MStatus status;
        MFnDagNode fnDagNode;
        locatorObj = fnDagNode.create("joint", mName.c_str());

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        const MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));
        const MQuaternion rotation = MQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat));
        const double scale[3]{ DirectX::XMVectorGetX(outScale), DirectX::XMVectorGetZ(outScale), DirectX::XMVectorGetY(outScale) };

        MQuaternion rotation_inv = rotation.inverse();

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation_inv, MSpace::kTransform);
        transformFn.setScale(scale);

        const MColor skelColor = MColor(1.0, 0.0, 0.0, 1.0);
        fnDagNode.setObjectColorType(MFnDagNode::kUseRGBColor);
        fnDagNode.setObjectColor(skelColor);

        return status;
    }

    static MStatus createLocator(const std::string& mName, DirectX::XMMATRIX xmMatrix)
    {
        MStatus status;

        MObject locatorObj;
        MDagModifier dagMod;

        locatorObj = dagMod.createNode("locator", MObject::kNullObj, &status);
        status = dagMod.doIt();

        if (status != MS::kSuccess)
        {
            MString theError("Failed to create locator node ");

            MGlobal::displayError(theError);
        }

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));

        double newScale[3];
        newScale[0] = DirectX::XMVectorGetX(outScale);
        newScale[1] = DirectX::XMVectorGetY(outScale);
        newScale[2] = DirectX::XMVectorGetZ(outScale);

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotationQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat), MSpace::kTransform);
        transformFn.setScale(newScale);

        MFnDagNode fnDagNode(locatorObj);
        fnDagNode.setName(mName.c_str());

        return status;
    }

    static MStatus createLocator(DirectX::XMMATRIX xmMatrix, MObject& locatorObj)
    {
        MStatus status;
        MDagModifier dagMod;

        locatorObj = dagMod.createNode("locator", MObject::kNullObj, &status);
        status = dagMod.doIt();

        if (status != MS::kSuccess)
        {
            MString theError("Failed to create locator node ");

            MGlobal::displayError(theError);
        }

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));

        double newScale[3];
        newScale[0] = DirectX::XMVectorGetX(outScale);
        newScale[1] = DirectX::XMVectorGetY(outScale);
        newScale[2] = DirectX::XMVectorGetZ(outScale);

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotationQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat), MSpace::kTransform);
        transformFn.setScale(newScale);

        return status;
    }

    static MStatus createForzaLocator(DirectX::XMMATRIX xmMatrix, MObject& locatorObj)
    {
        MStatus status;
        MDagModifier dagMod;

        locatorObj = dagMod.createNode("locator", MObject::kNullObj, &status);
        status = dagMod.doIt();

        if (status != MS::kSuccess)
        {
            MString theError("Failed to create locator node ");

            MGlobal::displayError(theError);
        }

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        const MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));
        const MQuaternion rotation = MQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat));

        std::array<double, 3> newScale{};
        newScale[0] = DirectX::XMVectorGetX(outScale);
        newScale[1] = DirectX::XMVectorGetY(outScale);
        newScale[2] = DirectX::XMVectorGetZ(outScale);

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation.inverse(), MSpace::kTransform);
        transformFn.setScale(newScale.data());

        return status;
    }

    static MStatus createForzaDummy(DirectX::XMMATRIX xmMatrix, MObject& locatorObj)
    {
        MStatus status;
        MDagModifier dagMod;

        locatorObj = dagMod.createNode("transform", MObject::kNullObj, &status);
        status = dagMod.doIt();

        if (status != MS::kSuccess)
        {
            MString theError("Failed to create locator node ");

            MGlobal::displayError(theError);
        }

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        const MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetZ(outTrans), DirectX::XMVectorGetY(outTrans));
        const MQuaternion rotation = MQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetW(outRotQuat));

        std::array<double, 3> newScale;
        newScale[0] = DirectX::XMVectorGetX(outScale);
        newScale[1] = DirectX::XMVectorGetY(outScale);
        newScale[2] = DirectX::XMVectorGetZ(outScale);

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation.inverse(), MSpace::kTransform);
        transformFn.setScale(newScale.data());

        return status;
    }

    static MObject createLocator()
    {
        MStatus status;
        MDagModifier dagMod;
        MObject locatorObj = dagMod.createNode("transform", MObject::kNullObj, &status);

        status = dagMod.doIt();

        if (status != MS::kSuccess)
        {
            MString theError("Failed to create locator node ");

            MGlobal::displayError(theError);
        }

        return locatorObj;
    }

    static MStatus setBoneTransform(std::string& mName, DirectX::XMMATRIX& xmMatrix)
    {
        MStatus status;
        MObject object;
        getObjectByName(mName.c_str(), object);


        MFnIkJoint jointFn;
        jointFn.create(MObject::kNullObj, &status);
        jointFn.setName(mName.c_str());

        DirectX::XMVECTOR outScale, outRotQuat, outTrans;
        DirectX::XMMatrixDecompose(&outScale, &outRotQuat, &outTrans, xmMatrix);

        MVector position = MVector(DirectX::XMVectorGetX(outTrans), DirectX::XMVectorGetY(outTrans), DirectX::XMVectorGetZ(outTrans));

        jointFn.setTranslation(position, MSpace::kTransform);
        jointFn.setRotationQuaternion(DirectX::XMVectorGetX(outRotQuat), DirectX::XMVectorGetY(outRotQuat), DirectX::XMVectorGetZ(outRotQuat), DirectX::XMVectorGetW(outRotQuat), MSpace::kTransform);

        return status;
    }

    static std::string GetContainerDirection(const std::string& container)
    {
        std::vector<std::string> lNodePos = { "LF", "RF", "RR", "LR", "LM", "RM" };

        for (const auto& position : lNodePos) {
            if (std::regex_search(container, std::regex(position))) {
                return position;
            }
        }

        return std::string();
    }

    static fmnext::PartLocation GetPartDirection(const std::string& containerName)
    {
        std::string lcontainerPos = GetContainerDirection(containerName);
        std::vector<std::string> lNodePos = { "LF", "RF", "RR", "LR", "LM", "RM" };
        std::unordered_map<std::string, fmnext::PartLocation> lpartMap
        {
            { "LF", fmnext::PartLocation(fmnext::PartLocation::FRONT, fmnext::PartLocation::LEFT) },
            { "RF", fmnext::PartLocation(fmnext::PartLocation::FRONT, fmnext::PartLocation::RIGHT) },

            { "LR", fmnext::PartLocation(fmnext::PartLocation::REAR, fmnext::PartLocation::LEFT) },
            { "RR", fmnext::PartLocation(fmnext::PartLocation::REAR, fmnext::PartLocation::RIGHT) },

            { "LM", fmnext::PartLocation(fmnext::PartLocation::MID, fmnext::PartLocation::LEFT) },
            { "RM", fmnext::PartLocation(fmnext::PartLocation::MID, fmnext::PartLocation::RIGHT) }
        };

        for (const auto& position : lNodePos) {
            if (std::regex_match(lcontainerPos, std::regex(position))) {
                if (const auto& result = lpartMap.find(position); result != lpartMap.end()) {
                    return result->second;
                }
            }
        }

        return fmnext::PartLocation();
    }

    static MObject createMaterialfromMemory(const std::string& Name, const std::shared_ptr<fmnext::BundleReader::BundleData>& primary_material)
    {
        // Create a Shading Group
        // Create the shading group from the list
        MSelectionList selList;
        MFnSet setFn;
        MObject shadingGroup = setFn.create(selList, MFnSet::kRenderableOnly, false);

        MString updated_name = updateNodeName(Name).c_str();

        MString shaderName;
        shaderName.format("^1s_SG", updated_name);

        setFn.setName(shaderName);

        // Connect the mesh to the shading group//
        // Set material to a phong function set
        MObject obj;
        MFnStandardSurfaceShader materialFn;
        obj = materialFn.create();
        materialFn.setName(updated_name);

        if (primary_material)
        {
            //MGlobal::displayWarning("Using shader information");
            std::vector<DirectX::XMFLOAT4> colors;

            for (const auto& param : primary_material->ShaderParameters)
            {
                if (param.type == fmnext::ShaderParameter_Color)
                {
                    colors.push_back(std::any_cast<DirectX::XMFLOAT4>(param.value));

                    DirectX::XMFLOAT4 diffuse = std::any_cast<DirectX::XMFLOAT4>(param.value);
                }

                if (param.type == fmnext::ShaderParameter_Texture2D)
                {
                    std::string path = std::any_cast<std::string>(param.value);
                    //MGlobal::displayInfo(path.c_str());
                }
            }

            // base
            materialFn.setBase(0.5);
            if (!colors.empty())
            {
                materialFn.setBaseColor(MColor(colors[0].x, colors[0].y, colors[0].z));
            }
            else 
            {
                materialFn.setBaseColor(MColor(0.106f, 0.086f, 0.082f));
            }

            // specular
            materialFn.setSpecularRoughness(1.f);
            materialFn.setSpecularIOR(1.520f);

            // coat
            materialFn.setCoat(0.f);
            materialFn.setCoatRoughness(0.f);
            materialFn.setCoatIOR(1.5f);
            materialFn.setCoatAnisotropy(0.5f);



            /*

            if (!colors.empty())
            {
                // base
                materialFn.setBase(0.5);
                materialFn.setBaseColor(MColor(colors[0].x, colors[0].y, colors[0].z));

                // specular
                materialFn.setSpecularRoughness(0.1f);
                materialFn.setSpecularIOR(1.520f);

                // coat
                materialFn.setCoat(1.0);
                materialFn.setCoatRoughness(0.f);
                materialFn.setCoatIOR(1.5f);
                materialFn.setCoatAnisotropy(0.5f);
            }
            */

        }
        //;materialFn.setObject(obj);

        /*
        if (!primary_material && !secondary_material)
        {
            for (auto& param : secondary_material->ShaderParameters)
            {
                if (param.type == ForzaTech::ShaderParameter_Color)
                {
                    DirectX::XMFLOAT4 diffuse = std::any_cast<DirectX::XMFLOAT4>(param.value);
                }
            }
        }
        */


        // Status the operations
        MStatus status;

        // Connect the shader
        // Break the default connection that is created
        MPlugArray plug_array;
        MPlug surfaceShader = setFn.findPlug("surfaceShader", status);
        surfaceShader.connectedTo(plug_array, true, true);

        // Create a modifier to break/create connections
        MDGModifier dgModifier;
        dgModifier.disconnect(plug_array[0], surfaceShader);

        // Connect outColor to surfaceShader
        MPlug materialPlug = materialFn.findPlug("outColor", status);
        dgModifier.connect(materialPlug, surfaceShader);

        // Invoke dgModifier...
        dgModifier.doIt();

        return shadingGroup;
    }
    
    static MObject createShadingGroup(const MString& Name)
    {
        MStatus status;

        MFnSet fnSet;
        MSelectionList selList;
        MObject shadingGroup = fnSet.create(selList, MFnSet::kRenderableOnly, &status);
        if (status != MS::kSuccess)
        {
            MString theError("Could not create shading engine: ");
            theError += Name;
            MGlobal::displayError(theError);

            return shadingGroup;
        }

        fnSet.setName(Name);

        return shadingGroup;
    }

    static MObject createShaderFrom(const std::string& Name)
    {
        // Create a Shading Group
        // Create the shading group from the list
        MSelectionList selList;
        MFnSet setFn;
        MObject shadingGroup = setFn.create(selList, MFnSet::kRenderableOnly, false);
        MString updated_name = updateNodeName(Name).c_str();

        MString shaderName;
        shaderName.format("^1s_SG", updated_name);

        setFn.setName(shaderName);

        // Connect the mesh to the shading group//
        // Set material to a phong function set
        MObject obj;
        MFnStandardSurfaceShader materialFn;
        obj = materialFn.create();
        materialFn.setName(updated_name);
        //materialFn.setObject(obj);

        // Status the operations
        MStatus status;

        // Connect the shader
        // Break the default connection that is created
        MPlugArray plug_array;
        MPlug surfaceShader = setFn.findPlug("surfaceShader", status);
        surfaceShader.connectedTo(plug_array, true, true);

        // Create a modifier to break/create connections
        MDGModifier dgModifier;
        dgModifier.disconnect(plug_array[0], surfaceShader);

        // Connect outColor to surfaceShader
        MPlug materialPlug = materialFn.findPlug("outColor", status);
        dgModifier.connect(materialPlug, surfaceShader);

        // Invoke dgModifier...
        dgModifier.doIt();

        return shadingGroup;
    }

    static MObject createMaterialShader(const std::string& Name)
    {
        // Status the operations
        MStatus status;

        // Create a Shading Group
        // Create the shading group from the list
        MSelectionList selList;
        MFnSet setFn;
        MObject shadingGroup = setFn.create(selList, MFnSet::kRenderableOnly, false);

        MString updated_name = updateNodeName(Name).c_str();

        MString shaderName;
        shaderName.format("^1s_SG", updated_name);

        setFn.setName(shaderName);

        // Connect the mesh to the shading group//
        // Set material to a phong function set
        MObject obj;
        MFnStandardSurfaceShader materialFn;
        obj = materialFn.create();
        materialFn.setName(updated_name);
        materialFn.setBaseColor(MColor(0.451f, 0.451f, 0.451f));
        //materialFn.setObject(obj);

        // Connect the shader
        // Break the default connection that is created
        MPlugArray plug_array;
        MPlug surfaceShader = setFn.findPlug("surfaceShader", status);
        surfaceShader.connectedTo(plug_array, true, true);

        // Create a modifier to break/create connections
        MDGModifier dgModifier;
        dgModifier.disconnect(plug_array[0], surfaceShader);

        // Connect outColor to surfaceShader
        MPlug materialPlug = materialFn.findPlug("outColor", status);
        dgModifier.connect(materialPlug, surfaceShader);

        // Invoke dgModifier...
        dgModifier.doIt();

        return shadingGroup;
    }

    static MObject createCarpaintfromMemory(const std::string& Name, const DirectX::XMFLOAT3& carpaint)
    {
        // Create a Shading Group
        // Create the shading group from the list
        MSelectionList selList;
        MFnSet setFn;
        MObject shadingGroup = setFn.create(selList, MFnSet::kRenderableOnly, false);
        MString updated_name = updateNodeName(Name).c_str();

        MString shaderName;
        shaderName.format("^1s_SG", updated_name);

        setFn.setName(shaderName);

        // Connect the mesh to the shading group//
        // Set material to a phong function set
        MObject obj;
        MFnStandardSurfaceShader materialFn;
        obj = materialFn.create();
        materialFn.setName(updated_name);

        // base
        materialFn.setBase(0.5f);
        materialFn.setBaseColor(MColor(carpaint.x, carpaint.y, carpaint.z));

        // specular
        materialFn.setSpecularRoughness(0.1f);
        materialFn.setSpecularIOR(1.520f);

        // coat
        materialFn.setCoat(1.0f);
        materialFn.setCoatRoughness(0.f);
        materialFn.setCoatIOR(1.5f);
        materialFn.setCoatAnisotropy(0.5f);

        //materialFn.setObject(obj);

        // Status the operations
        MStatus status;

        // Connect the shader
        // Break the default connection that is created
        MPlugArray plug_array;
        MPlug surfaceShader = setFn.findPlug("surfaceShader", status);
        surfaceShader.connectedTo(plug_array, true, true);

        // Create a modifier to break/create connections
        MDGModifier dgModifier;
        dgModifier.disconnect(plug_array[0], surfaceShader);

        // Connect outColor to surfaceShader
        MPlug materialPlug = materialFn.findPlug("outColor", status);
        dgModifier.connect(materialPlug, surfaceShader);

        // Invoke dgModifier...
        dgModifier.doIt();

        return shadingGroup;
    }

    static MObject createGlassfromMemory(const std::string& Name)
    {
        // Create a Shading Group
        // Create the shading group from the list
        MSelectionList selList;
        MFnSet setFn;
        MObject shadingGroup = setFn.create(selList, MFnSet::kRenderableOnly, false);

        MString updated_name = updateNodeName(Name).c_str();

        MString shaderName;
        shaderName.format("^1s_SG", updated_name);

        setFn.setName(shaderName);

        // Connect the mesh to the shading group//
        // Set material to a phong function set
        MObject obj;
        MFnStandardSurfaceShader materialFn;
        obj = materialFn.create();
        materialFn.setName(updated_name);

        // base
        materialFn.setBase(0.5);
        //materialFn.setBaseColor(MColor(colors[0].x, colors[0].y, colors[0].z));

        // specular
        materialFn.setSpecularRoughness(0.0f);
        materialFn.setSpecularIOR(1.52f);
        materialFn.setSpecularAnisotropy(0.5f);

        // coat
        materialFn.setCoat(0);
        materialFn.setCoatRoughness(0.f);
        materialFn.setCoatIOR(1.f);
        materialFn.setCoatAnisotropy(0.5f);
        materialFn.setCoatAffectRoughness(1.f);

        materialFn.setTransmission(1.f);


        //materialFn.setObject(obj);


        // Status the operations
        MStatus status;

        // Connect the shader
        // Break the default connection that is created
        MPlugArray plug_array;
        MPlug surfaceShader = setFn.findPlug("surfaceShader", status);
        surfaceShader.connectedTo(plug_array, true, true);

        // Create a modifier to break/create connections
        MDGModifier dgModifier;
        dgModifier.disconnect(plug_array[0], surfaceShader);

        // Connect outColor to surfaceShader
        MPlug materialPlug = materialFn.findPlug("outColor", status);
        dgModifier.connect(materialPlug, surfaceShader);

        // Invoke dgModifier...
        dgModifier.doIt();

        return shadingGroup;
    }

    static MStatus parentMeshtoShader(MObject& mesh_obj, MObject& shader_obj)
    {
        MStatus status;
        MFnSet set(shader_obj);
        status = set.addMember(mesh_obj);

        if (!status)
        {
            MGlobal::displayError(MString("Failed to add mesh to initalShadingGroup. \n"));
            return status;
        }

        return status;
    }

    static void SetImporterFrameRate()
    {
        MString	result;

        if (MGlobal::executeCommand("currentUnit -q -fullName -time", result))
        {
            if (result != MString("ntscf"))
            {
                MGlobal::executeCommand("currentUnit -time ntscf", result);
            }
        }
        else
        {
            MGlobal::displayWarning("Could not get current linear units.");
        }

    }

    static void LoadSkeleton(const std::vector<fmnext::BundleReader::Bones>& skeleton)
    {
        std::vector<MObject> joints;
        joints.reserve(skeleton.size());

        for (size_t i = 0; i < skeleton.size(); ++i)
        {
            std::string bone_name = (skeleton[i].name == "<root>") ? "BundleRootBone" : skeleton[i].name;

            MObject locatorObj = createJoint(bone_name, skeleton[i].transform);

            joints.push_back(locatorObj);

            if (skeleton[i].parent_index != GrannyNoParentBone)
            {
                int ParentIndex = skeleton[i].parent_index;

                MFnDagNode fnDagNode(joints[ParentIndex]);

                if (fnDagNode.addChild(joints[i]))
                {
                    //MString message;
                    //message.format(MString("Joint ^1s is attached to ^2s"), Name.c_str(), fnDagNode.name());

                    //MGlobal::displayWarning(message);
                }
            }
        }
    }

    static MStatus setPlayback(double min, double max)
    {
        MStatus status = MS::kSuccess;
        MAnimControl anim;
        MTime minTime, maxTime, curTime;
        minTime.setValue(min);
        maxTime.setValue(max);

        status = anim.setPlaybackSpeed(1);

        status = anim.setMinTime(minTime);
        status = anim.setAnimationStartTime(minTime);

        if (max > 0)
        {
            status = anim.setMaxTime(maxTime);
            status = anim.setAnimationEndTime(maxTime);
        }

        return status;
    }

    static MStatus getDagPathByName(const MString& name, MDagPath& dagPath)
    {
        MItDependencyNodes nodesIter;

        for (; !nodesIter.isDone(); nodesIter.next())
        {
            MObject node = nodesIter.thisNode();
            MFnDagNode nodeFn(node);

            if (nodeFn.name() == name)
            {
                nodeFn.getPath(dagPath);

                return MS::kSuccess;
            }
        }

        return MS::kNotFound;
    }

    static void handleSkeleton(granny_skeleton* Skeleton)
    {
        std::string skel_name(Skeleton->Name);

        if (!objExists(skel_name))
        {
            std::vector<MObject> joints;
            joints.reserve(Skeleton->BoneCount);

            for (size_t i = 0; i < Skeleton->BoneCount; ++i)
            {
                granny_transform LocalTransform = Skeleton->Bones[i].LocalTransform; // Position[3], Orientation[4], ScaleShear[3]
                std::string Name(Skeleton->Bones[i].Name);

                MObject locatorObj = createJoint(Name, LocalTransform);

                joints.push_back(locatorObj);

                if (Skeleton->Bones[i].ParentIndex != GrannyNoParentBone)
                {
                    granny_int32 ParentIndex = Skeleton->Bones[i].ParentIndex;

                    MFnDagNode fnDagNode(joints[ParentIndex]);

                    if (fnDagNode.addChild(joints[i]))
                    {
                        //MString message;
                        //message.format(MString("Joint ^1s is attached to ^2s"), Name.c_str(), fnDagNode.name());

                        //MGlobal::displayWarning(message);
                    }
                }
            }
        }
        else 
        {
            MString message;
            message.format("^1s already exists, ignoring next... \n", skel_name.c_str());
            MGlobal::displayWarning(message);
        }
    }

    static void handleAnimations(granny_skeleton* Skeleton = nullptr, granny_animation* Animation = nullptr, const std::vector<MObject>& joints = {})
    {
        MStatus status;

        if (Skeleton && Animation)
        {
            // Assume there is only a single animation/track on each gr2
            granny_track_group* TrackGroup = Animation->TrackGroups[0];

            uint32_t TotalSamples = (uint32_t)(Animation->Duration * 30); // amount of frames

            //setPlayback(0, TotalSamples);

            for (granny_int32 TrackIndex = 0; TrackIndex < TrackGroup->TransformTrackCount; ++TrackIndex) {

                granny_int32x BoneIndex{};
                if (!GrannyFindBoneByName(Skeleton, TrackGroup->TransformTracks[TrackIndex].Name, &BoneIndex)) {
                    MString boneName = TrackGroup->TransformTracks[TrackIndex].Name;

                    MString message = "Unable to find bone ";
                    message += boneName;
                    message += " \n";

                    MGlobal::displayError(message);
                }

                if (BoneIndex != GrannyNoParentBone)
                {
                    MFnDagNode fnSet(joints[BoneIndex], &status);
                    MGlobal::displayInfo(fnSet.name());
                    MDagPath mObject;

                    status = fnSet.getPath(mObject);

                    if (!status) {
                        MString message = "Unable to find dagPath ";
                        message += fnSet.name();
                        message += " \n";

                        MGlobal::displayError(message);
                    }


                    MString attrName("translateX");
                    const MObject attrX = fnSet.attribute(attrName, &status);
                    if (!status) {
                        MGlobal::displayError("Failure to find attribute\n");
                    }
                    MFnAnimCurve acFnSetX;
                    acFnSetX.create(mObject.transform(), attrX, NULL, &status);
                    if (!status) {
                        MGlobal::displayError("Failure creating MFnAnimCurve function set (translateX)\n");
                    }

                    attrName.set("translateY");
                    const MObject attrY = fnSet.attribute(attrName, &status);
                    if (!status) {
                        MGlobal::displayError("Failure to find attribute\n");
                    }
                    MFnAnimCurve acFnSetY;
                    acFnSetY.create(mObject.transform(), attrY, NULL, &status);
                    if (!status) {
                        MGlobal::displayError("Failure creating MFnAnimCurve function set (translateZ)\n");
                    }

                    attrName.set("translateZ");
                    const MObject attrZ = fnSet.attribute(attrName, &status);
                    if (!status) {
                        MGlobal::displayError("Failure to find attribute\n");
                    }
                    MFnAnimCurve acFnSetZ;
                    acFnSetZ.create(mObject.transform(), attrZ, NULL, &status);
                    if (!status) {
                        MGlobal::displayError("Failure creating MFnAnimCurve function set (translateZ)\n");
                    }

                    const MObject attrrotX = fnSet.attribute("rotateX", &status);
                    if (!status) {
                        MGlobal::displayError("Failure to find attribute\n");
                    }
                    MFnAnimCurve acFnSetRotX;
                    acFnSetRotX.create(mObject.transform(), attrrotX, NULL, &status);
                    if (!status) {
                        MGlobal::displayError("Failure creating MFnAnimCurve function set (rotateX)\n");
                    }

                    const MObject attrrotY = fnSet.attribute("rotateY", &status);
                    if (!status) {
                        MGlobal::displayError("Failure to find attribute\n");
                    }
                    MFnAnimCurve acFnSetRotY;
                    acFnSetRotY.create(mObject.transform(), attrrotY, NULL, &status);
                    if (!status) {
                        MGlobal::displayError("Failure creating MFnAnimCurve function set (rotateZ)\n");
                    }

                    const MObject attrrotZ = fnSet.attribute("rotateZ", &status);
                    if (!status) {
                        MGlobal::displayError("Failure to find attribute\n");
                    }
                    MFnAnimCurve acFnSetRotZ;
                    acFnSetRotZ.create(mObject.transform(), attrrotZ, NULL, &status);
                    if (!status) {
                        MGlobal::displayError("Failure creating MFnAnimCurve function set (rotateZ)\n");
                    }


                    for (uint32_t CurrentFrame = 0; CurrentFrame < TotalSamples; ++CurrentFrame)
                    {
                        granny_real32 CurrentTime = CurrentFrame * (Animation->Duration / (TotalSamples - 1));

                        // Get the three curves that specify the transform
                        granny_curve2* PositionCurve = &TrackGroup->TransformTracks[TrackIndex].PositionCurve;
                        granny_curve2* OrientationCurve = &TrackGroup->TransformTracks[TrackIndex].OrientationCurve;
                        granny_curve2* ScaleShearCurve = &TrackGroup->TransformTracks[TrackIndex].ScaleShearCurve; // going to ignore this for a while

                        granny_transform EvalResult;
                        GrannyMakeIdentity(&EvalResult);

                        GrannyEvaluateCurveAtT(3, false, false, PositionCurve, true, Animation->Duration, CurrentTime, EvalResult.Position, GrannyCurveIdentityPosition);
                        GrannyEvaluateCurveAtT(4, false, false, OrientationCurve, true, Animation->Duration, CurrentTime, EvalResult.Orientation, GrannyCurveIdentityOrientation);

                        MTime keyFrame((double)CurrentFrame, MTime::k30FPS);

                        if ((MS::kSuccess != acFnSetX.addKeyframe(keyFrame, EvalResult.Position[0])) || (MS::kSuccess != acFnSetY.addKeyframe(keyFrame, EvalResult.Position[2])) || (MS::kSuccess != acFnSetZ.addKeyframe(keyFrame, EvalResult.Position[1]))) {
                            MGlobal::displayError("Error setting the keyframe\n");
                        }

                        const MQuaternion rotation = MQuaternion(EvalResult.Orientation[0], EvalResult.Orientation[2], EvalResult.Orientation[1], EvalResult.Orientation[3]);

                        auto rotate = rotation.inverse().asEulerRotation();

                        if ((MS::kSuccess != acFnSetRotX.addKeyframe(keyFrame, rotate.x)) || (MS::kSuccess != acFnSetRotY.addKeyframe(keyFrame, rotate.y)) || (MS::kSuccess != acFnSetRotZ.addKeyframe(keyFrame, rotate.z))) {
                            MGlobal::displayError("Error setting the keyframe\n");
                        }

                        //printf("Track: %s, Frame: %i: Position[% .3f, % .3f, % .3f]\n", TrackGroup->TransformTracks[TrackIndex].Name, CurrentFrame, EvalResult.Position[0], EvalResult.Position[1], EvalResult.Position[2]);
                        //printf("Time: %.3f: Rotation[% .3f, % .3f, % .3f, % .3f]\n", CurrentTime, EvalResult.Orientation[0], EvalResult.Orientation[1], EvalResult.Orientation[2], EvalResult.Orientation[3]);
                    }
                }


            }

        }

    }

    static bool skeletonExists(granny_skeleton* Skeleton = nullptr)
    {
        std::string skel_name(Skeleton->Name);

        if (objExists(skel_name))
        {
            return true;
        }

        return false;
    }

    static MStatus handleAnimation(granny_skeleton* Skeleton = nullptr, granny_animation* Animation = nullptr)
    {
        MStatus status;

        if (Skeleton && Animation)
        {
            // Assume there is only a single animation/track on each gr2
            granny_track_group* TrackGroup = Animation->TrackGroups[0];

            uint32_t TotalSamples = (uint32_t)(Animation->Duration * 60); // amount of frames

            //setPlayback(0, TotalSamples);

            MAnimControl anim;

            for (granny_int32 TrackIndex = 0; TrackIndex < TrackGroup->TransformTrackCount; ++TrackIndex) {

                std::string boneName = TrackGroup->TransformTracks[TrackIndex].Name;

                granny_int32x BoneIndex{};
                if (!GrannyFindBoneByName(Skeleton, TrackGroup->TransformTracks[TrackIndex].Name, &BoneIndex)) {
                    
                    MString message = "Granny: Unable to find bone ";
                    message += boneName.c_str();
                    message += " \n";

                    MGlobal::displayWarning(message);

                    continue;
                }

                if (BoneIndex != GrannyNoParentBone)
                {
                    MStatus status;

                    MDagPath mObject;
                    status = getDagPathByName(updateNodeName(boneName).c_str(), mObject);
                    if (!status) {
                        MString message = "Failed to find MDagPath for bone ";
                        message += boneName.c_str();
                        message += " \n";

                        MGlobal::displayError(message);
                    }

                    MFnDagNode fnSet(mObject, &status);
                    if (!status) {
                        MGlobal::displayError("Failed to create function set\n");
                    }

                    /*
                    MFnDagNode fnSet(joints[BoneIndex], &status);
                    MGlobal::displayInfo(fnSet.name());
                    MDagPath mObject;

                    status = fnSet.getPath(mObject);

                    if (!status) {
                        MString message = "Unable to find dagPath ";
                        message += fnSet.name();
                        message += " \n";

                        MGlobal::displayError(message);
                    }
                    */

                    const MObject attrX = fnSet.attribute("translateX", &status);
                    if (!status) {
                        MGlobal::displayError("Failed to find attribute\n");
                    }

                    const MObject attrY = fnSet.attribute("translateY", &status);
                    if (!status) {
                        MGlobal::displayError("Failed to find attribute\n");
                    }

                    const MObject attrZ = fnSet.attribute("translateZ", &status);
                    if (!status) {
                        MGlobal::displayError("Failed to find attribute\n");
                    }

                    MPlug attrXPlug = MPlug(fnSet.object(), attrX);
                    MPlug attrYPlug = MPlug(fnSet.object(), attrY);
                    MPlug attrZPlug = MPlug(fnSet.object(), attrZ);

                    MFnAnimCurve acFnSetX;
                    if (!attrXPlug.isConnected()) {
                        acFnSetX.create(mObject.transform(), attrX, NULL, &status);
                        if (!status) {
                            MGlobal::displayError("Failed creating MFnAnimCurve function set (translateX)\n");
                        }
                    }
                    else {
                        MObjectArray animations;
                        MAnimUtil::findAnimation(attrXPlug, animations);

                        for (MObject& animCurveNode : animations)
                        {
                            if (!animCurveNode.hasFn(MFn::kAnimCurve)) {
                                continue;
                            }

                            acFnSetX.setObject(animCurveNode);
                        }
                    }


                    MFnAnimCurve acFnSetY;
                    if (!attrYPlug.isConnected()) {
                        acFnSetY.create(mObject.transform(), attrY, NULL, &status);
                        if (!status) {
                            MGlobal::displayError("Failed creating MFnAnimCurve function set (translateY)\n");
                        }
                    }
                    else {
                        MObjectArray animations;
                        MAnimUtil::findAnimation(attrYPlug, animations);

                        for (MObject& animCurveNode : animations)
                        {
                            if (!animCurveNode.hasFn(MFn::kAnimCurve)) {
                                continue;
                            }

                            acFnSetY.setObject(animCurveNode);
                        }
                    }

                    MFnAnimCurve acFnSetZ;
                    if (!attrZPlug.isConnected()) {
                        acFnSetZ.create(mObject.transform(), attrZ, NULL, &status);
                        if (!status) {
                            MGlobal::displayError("Failed creating MFnAnimCurve function set (translateZ)\n");
                        }
                    }
                    else {
                        MObjectArray animations;
                        MAnimUtil::findAnimation(attrZPlug, animations);

                        for (MObject& animCurveNode : animations)
                        {
                            if (!animCurveNode.hasFn(MFn::kAnimCurve)) {
                                continue;
                            }

                            acFnSetZ.setObject(animCurveNode);
                        }
                    }
                   
                    const MObject attrRotX = fnSet.attribute("rotateX", &status);
                    if (!status) {
                        MGlobal::displayError("Failed to find attribute\n");
                    }

                    const MObject attrRotY = fnSet.attribute("rotateY", &status);
                    if (!status) {
                        MGlobal::displayError("Failed to find attribute\n");
                    }

                    const MObject attrRotZ = fnSet.attribute("rotateZ", &status);
                    if (!status) {
                        MGlobal::displayError("Failed to find attribute\n");
                    }

                    MPlug attrRotXPlug = MPlug(fnSet.object(), attrRotX);
                    MPlug attrRotYPlug = MPlug(fnSet.object(), attrRotY);
                    MPlug attrRotZPlug = MPlug(fnSet.object(), attrRotZ);



                    MFnAnimCurve acFnRotSetX;
                    if (!attrRotXPlug.isConnected()) {
                        acFnRotSetX.create(mObject.transform(), attrRotX, NULL, &status);
                        if (!status) {
                            MGlobal::displayError("Failed creating MFnAnimCurve function set (rotateX)\n");
                        }
                    }
                    else {
                        MObjectArray animations;
                        MAnimUtil::findAnimation(attrRotXPlug, animations);

                        for (MObject& animCurveNode : animations)
                        {
                            if (!animCurveNode.hasFn(MFn::kAnimCurve)) {
                                continue;
                            }

                            acFnRotSetX.setObject(animCurveNode);
                        }
                    }


                    MFnAnimCurve acFnRotSetY;
                    if (!attrRotYPlug.isConnected()) {
                        acFnRotSetY.create(mObject.transform(), attrRotY, NULL, &status);
                        if (!status) {
                            MGlobal::displayError("Failed creating MFnAnimCurve function set (rotateY)\n");
                        }
                    }
                    else {
                        MObjectArray animations;
                        MAnimUtil::findAnimation(attrRotYPlug, animations);

                        for (MObject& animCurveNode : animations)
                        {
                            if (!animCurveNode.hasFn(MFn::kAnimCurve)) {
                                continue;
                            }

                            acFnRotSetY.setObject(animCurveNode);
                        }
                    }

                    MFnAnimCurve acFnRotSetZ;
                    if (!attrRotZPlug.isConnected()) {
                        acFnRotSetZ.create(mObject.transform(), attrRotZ, NULL, &status);
                        if (!status) {
                            MGlobal::displayError("Failed creating MFnAnimCurve function set (rotateZ)\n");
                        }
                    }
                    else {
                        MObjectArray animations;
                        MAnimUtil::findAnimation(attrRotZPlug, animations);

                        for (MObject& animCurveNode : animations)
                        {
                            if (!animCurveNode.hasFn(MFn::kAnimCurve)) {
                                continue;
                            }

                            acFnRotSetZ.setObject(animCurveNode);
                        }
                    }














                    for (uint32_t CurrentFrame = 0; CurrentFrame < TotalSamples; ++CurrentFrame)
                    {
                        granny_real32 CurrentTime = CurrentFrame * (Animation->Duration / (TotalSamples - 1));

                        // Get the three curves that specify the transform
                        granny_curve2* PositionCurve = &TrackGroup->TransformTracks[TrackIndex].PositionCurve;
                        granny_curve2* OrientationCurve = &TrackGroup->TransformTracks[TrackIndex].OrientationCurve;
                        granny_curve2* ScaleShearCurve = &TrackGroup->TransformTracks[TrackIndex].ScaleShearCurve; // going to ignore this for a while

                        granny_transform EvalResult;
                        GrannyMakeIdentity(&EvalResult);

                        GrannyEvaluateCurveAtT(3, false, false, PositionCurve, true, Animation->Duration, CurrentTime, EvalResult.Position, GrannyCurveIdentityPosition);
                        GrannyEvaluateCurveAtT(4, false, false, OrientationCurve, true, Animation->Duration, CurrentTime, EvalResult.Orientation, GrannyCurveIdentityOrientation);

                        MTime keyFrame((double)anim.currentTime().value() + CurrentFrame, MTime::k60FPS);

                        if ((MS::kSuccess != acFnSetX.addKeyframe(keyFrame, EvalResult.Position[0])) || (MS::kSuccess != acFnSetY.addKeyframe(keyFrame, EvalResult.Position[2])) || (MS::kSuccess != acFnSetZ.addKeyframe(keyFrame, EvalResult.Position[1]))) {
                            MGlobal::displayError("Error setting the keyframe\n");
                        }

                        const MQuaternion rotation = MQuaternion(EvalResult.Orientation[0], EvalResult.Orientation[2], EvalResult.Orientation[1], EvalResult.Orientation[3]);

                        auto rotate = rotation.inverse().asEulerRotation();

                        if ((MS::kSuccess != acFnRotSetX.addKeyframe(keyFrame, rotate.x)) || (MS::kSuccess != acFnRotSetY.addKeyframe(keyFrame, rotate.y)) || (MS::kSuccess != acFnRotSetZ.addKeyframe(keyFrame, rotate.z))) {
                            MGlobal::displayError("Error setting the keyframe\n");
                        }

                        //printf("Track: %s, Frame: %i: Position[% .3f, % .3f, % .3f]\n", TrackGroup->TransformTracks[TrackIndex].Name, CurrentFrame, EvalResult.Position[0], EvalResult.Position[1], EvalResult.Position[2]);
                        //printf("Time: %.3f: Rotation[% .3f, % .3f, % .3f, % .3f]\n", CurrentTime, EvalResult.Orientation[0], EvalResult.Orientation[1], EvalResult.Orientation[2], EvalResult.Orientation[3]);
                    }
                }


            }

            return MStatus::kSuccess;
        }

        return MStatus::kFailure;
    }

    static std::string updateNodeName(const std::string& str)
    {
        return std::regex_replace(str, std::regex("([^a-zA-Z0-9\\w]+|\\s+|^[0-9]+)"), std::string("_"));
    }

    static std::filesystem::path deduplicatePath(const std::string& path) {
        std::filesystem::path result(path);
        int index = 1;

        while (std::filesystem::exists(result))
        {
            std::string filename = std::filesystem::path(path).stem().string();
            filename += "_";
            filename += std::to_string(index);
            filename += std::filesystem::path(path).extension().string();

            result = std::filesystem::path(path).replace_filename(filename);

            ++index;
        }

        return result;
    }

private:
    static MObject createJoint(const std::string& mName, granny_transform& LocalTransform)
    {
        MStatus status;
        MFnDagNode fnDagNode;
        std::string nodeName = updateNodeName(mName);
        MObject locatorObj = fnDagNode.create("joint", nodeName.c_str());

        MFnTransform transformFn(locatorObj, &status);
        MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

        const MVector position = MVector(LocalTransform.Position[0], LocalTransform.Position[2], LocalTransform.Position[1]);
        const MQuaternion rotation = MQuaternion(LocalTransform.Orientation[0], LocalTransform.Orientation[2], LocalTransform.Orientation[1], LocalTransform.Orientation[3]);

        std::array<double, 3> scale;
        scale[0] = static_cast<double>(*LocalTransform.ScaleShear[0]);
        scale[1] = static_cast<double>(*LocalTransform.ScaleShear[2]);
        scale[2] = static_cast<double>(*LocalTransform.ScaleShear[1]);

        transformFn.setTranslation(position, MSpace::kTransform);
        transformFn.setRotation(rotation.inverse(), MSpace::kTransform);
        //transformFn.setScale(scale.data());

        const MColor skelColor = MColor(1.0, 0.0, 0.0, 1.0);
        fnDagNode.setObjectColorType(MFnDagNode::kUseRGBColor);
        fnDagNode.setObjectColor(skelColor);

        return locatorObj;
    }

};

