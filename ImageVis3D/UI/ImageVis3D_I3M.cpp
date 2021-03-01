/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.


   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


//!    File   : ImageVis3D_I3M.cpp
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2009
//
//!    Copyright (C) 2009 DFKI, MMCI, SCI Institute

#include "ImageVis3D.h"

#include "../Tuvok/Basics/SysTools.h"
#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaTuvokTypes.h"
#include "../Tuvok/LuaScripting/TuvokSpecific/LuaDatasetProxy.h"

#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtWidgets/QMessageBox>

#include "PleaseWait.h"
#include "Renderer/RenderMesh.h" // we only need this include for proper down cast from RenderMesh to Mesh
#include "MobileGeoConverter.h"
#include "../IO/ZipFile.h"

using namespace tuvok;
using namespace std;

std::wstring MainWindow::ConvertTF(const std::wstring& strSource1DTFilename,
                             const std::wstring& strTargetDir,
                             const std::wstring& strTargetFullFilename,
                             PleaseWaitDialog& pleaseWait) {

  pleaseWait.SetText("Converting transfer function, please wait  ...");

  std::wstring filenameOnly = SysTools::ChangeExt(
      SysTools::GetFilename(strTargetFullFilename),L"i3m.1dt"
      );

  std::wstring strTarget1DTFilename = strTargetDir+L"/"+filenameOnly;
  MESSAGE("Converting transferfunction to %s", SysTools::toNarrow(strTarget1DTFilename).c_str());

  TransferFunction1D tfIn(strSource1DTFilename);
  if (tfIn.GetSize() > 256) {
    // resample 1D tf to 8bit
    tfIn.Resample(256);
  } else {
    // if it is 8bit already simply fill it to 256 entries
    tfIn.FillOrTruncate(256);
  }
  if (!tfIn.Save(strTarget1DTFilename)) return L"";


  MESSAGE("Saved 8bit transferfunction to %s",SysTools::toNarrow(strTarget1DTFilename).c_str());
  return strTarget1DTFilename;
}

std::wstring MainWindow::ConvertDataToI3M(LuaClassInstance currentDataset,
                                    const std::wstring& strTargetDir,
                                    PleaseWaitDialog& pleaseWait,
                                    bool bOverrideExisting) {
  shared_ptr<LuaScripting> ss = m_MasterController.LuaScript();
  if (ss->cexecRet<LuaDatasetProxy::DatasetType>(
          currentDataset.fqName() + ".getDSType") != LuaDatasetProxy::UVF) {
    T_ERROR("MainWindow::ConvertDataToI3M can only accept UVF datasets.");
    return L"";
  }

  std::wstring dsFilename = ss->cexecRet<wstring>(currentDataset.fqName() + ".fullpath");
  pleaseWait.SetText(QString("Converting:") + QString::fromStdWString(dsFilename));

  // UVF to I3M

  // first, find the smalest LOD with every dimension
  // larger or equal to 128 (if possible)
  int iLODLevel = static_cast<int>(
      ss->cexecRet<uint64_t>(currentDataset.fqName() + ".getLODLevelCount")) - 1;
  for (;iLODLevel>0;iLODLevel--) {
    UINTVECTOR3 vLODSize = UINTVECTOR3(ss->cexecRet<UINT64VECTOR3>(
            currentDataset.fqName() + ".getDomainSize", 
              static_cast<size_t>(iLODLevel), size_t(0)));
    if (vLODSize.x >= 128 &&
        vLODSize.y >= 128 &&
        vLODSize.z >= 128) break;
  }

  wstring filenameOnly = SysTools::GetFilename(dsFilename);
  wstring strTargetFilename = strTargetDir+L"/"+
                             SysTools::ChangeExt(filenameOnly,L"i3m");

  if (!bOverrideExisting && SysTools::FileExists(strTargetFilename)) {
    strTargetFilename = SysTools::FindNextSequenceName(
                                                    strTargetFilename
                                                    );
  }

  if (!ss->cexecRet<bool>("tuvok.io.exportDataset",
                          currentDataset, 
                          static_cast<uint64_t>(iLODLevel), 
                          strTargetFilename,
                          m_strTempDir))
  {
    return L"";
  }

  return strTargetFilename;
}

namespace {
  template<typename T>
  shared_ptr<G3D::GeometrySoA> mergeMeshType(Mesh::EMeshType meshType, const vector<shared_ptr<T>>& meshes)
  {
    // Find first mesh with given primitive type.
    size_t m = 0;
    for (; m < meshes.size(); m++)
      if (meshes[m]->GetMeshType() == meshType)
        break;
    if (m >= meshes.size())
      return nullptr; // Primitive type not found.

    MobileGeoConverter mgc;
    float * colors = nullptr;
    shared_ptr<G3D::GeometrySoA> g3d = mgc.ConvertToNative(*meshes[m], colors, true);
    // Now the color array is handled by g3d instance which is a copy a the first mesh.
    // The color array will be deleted with the g3d instance.
    colors = nullptr;

    // Merge all other meshes into the first one.
    for (size_t i = m+1; i < meshes.size(); i++)
    {
      if (meshes[m]->GetMeshType() != meshes[i]->GetMeshType())
        continue;
      shared_ptr<const G3D::GeometrySoA> geo = mgc.ConvertToNative(*meshes[i], colors);
      if (!G3D::merge(g3d.get(), geo.get())) {
        T_ERROR("Could not merge mesh %d with mesh %d.", i, m);
      }
      // Clean up color data that we might have created.
      if (colors != nullptr) {
        delete[] colors;
        colors = nullptr;
      }
      // The instance geo will be deleted by shared_ptr d'tor.
      // Do not call G3D::clean on it because it directly references the mesh's data.
    }
    return g3d;
  }
}

void MainWindow::TransferToI3M() {
  if (!m_pActiveRenderWin) return;

  shared_ptr<LuaScripting> ss(m_MasterController.LuaScript());
  LuaClassInstance ds = m_pActiveRenderWin->GetRendererDataset();
  if (ss->cexecRet<LuaDatasetProxy::DatasetType>(
          ds.fqName() + ".getDSType") == LuaDatasetProxy::UVF) {
    QSettings settings;
    QString strLastDir = settings.value("Folders/I3MServer",
                                        ".").toString();

    QString directoryName =
      QFileDialog::getExistingDirectory(this, "Select Dataset Server folder.",
                                        strLastDir);

    if (directoryName.isEmpty()) return;

    wstring strTargetDir = directoryName.toStdWString();
    
    settings.setValue("Folders/I3MServer", directoryName);


    PleaseWaitDialog pleaseWait(this);
    pleaseWait.SetText("Preparing data  ...");
    pleaseWait.AttachLabel(&m_MasterController);

    wstring tempVolume = ConvertDataToI3M(ds, m_strTempDir, pleaseWait, true);

    if (tempVolume == L"") {
      QMessageBox errorMessage;
      errorMessage.setText("Unable to convert the dataset "
                           "into the given directory.");
      errorMessage.setIcon(QMessageBox::Critical);
      errorMessage.exec();  
      T_ERROR("Unable to convert the dataset "
               "into the given directory.");
    }

    wstring strTemp1DTFilename = m_strTempDir + L"i3mexport.1dt";
    m_1DTransferFunction->SaveToFile(QString::fromStdWString(strTemp1DTFilename));

    wstring tempTF = ConvertTF(strTemp1DTFilename, m_strTempDir,
                           ss->cexecRet<wstring>(ds.fqName() + ".fullpath"),
                           pleaseWait);
    SysTools::RemoveFile(strTemp1DTFilename);

    if (tempTF == L"") {
      QMessageBox errorMessage;
      errorMessage.setText("Unable to convert the transferfunction "
                           "into the given directory.");
      errorMessage.setIcon(QMessageBox::Critical);
      errorMessage.exec();  
      T_ERROR("Unable to convert the transferfunction "
               "into the given directory.");
    }

    wstring filenameOnly = SysTools::RemoveExt(SysTools::GetFilename(
      ss->cexecRet<wstring>(ds.fqName() + ".fullpath")));

    // zip volume and TF together
    ZipFile i3m;
    i3m.openZip(SysTools::toNarrow(strTargetDir + L"/" + filenameOnly + L".i3m.zip"));
    i3m.copyFileToZip(SysTools::toNarrow(tempVolume), SysTools::toNarrow(filenameOnly + L".i3m"));
    i3m.copyFileToZip(SysTools::toNarrow(tempTF), SysTools::toNarrow(filenameOnly + L".1dt"));
    i3m.close();

    // remove temp files
    SysTools::RemoveFile(tempVolume);
    SysTools::RemoveFile(tempTF);

    pleaseWait.SetText("Exporting Meshes ...");

#if 0
    // Old version:
    // Export the mesh as stored on disk.
    const std::vector<std::shared_ptr<Mesh>> meshes = 
        ss->cexecRet<std::vector<std::shared_ptr<Mesh>>>(
            ds.fqName() + ".getMeshes");
#else
    // Changed by Alex:
    // Export the visible rendered mesh with updated colors as we see it.
    // The export will bake the colors into the exported mesh anyway.
    const std::vector<shared_ptr<RenderMesh>> meshes = m_pActiveRenderWin->GetRendererMeshes();
#endif

    shared_ptr<G3D::GeometrySoA> triangles = mergeMeshType(Mesh::MT_TRIANGLES, meshes);
    shared_ptr<G3D::GeometrySoA> lines = mergeMeshType(Mesh::MT_LINES, meshes);

    // write to temp files
    if (triangles != nullptr) {
      G3D::write(SysTools::toNarrow(m_strTempDir + L"/" + filenameOnly + L".triangles.g3d"), triangles.get());
      G3D::clean(triangles.get());
    }
    if (lines != nullptr) {
      G3D::write(SysTools::toNarrow(m_strTempDir + L"/" + filenameOnly + L".lines.g3d"), lines.get());
      G3D::clean(lines.get());
    }

    // zip the files
    ZipFile g3d;
    g3d.openZip(SysTools::toNarrow(strTargetDir + L"/" + filenameOnly + L".g3d.zip"));
    g3d.copyFileToZip(SysTools::toNarrow(m_strTempDir + L"/" + filenameOnly + L".triangles.g3d"), SysTools::toNarrow(filenameOnly + L".triangles.g3d"));
    g3d.copyFileToZip(SysTools::toNarrow(m_strTempDir + L"/" + filenameOnly + L".lines.g3d"), SysTools::toNarrow(filenameOnly + L".lines.g3d"));
    g3d.close();

    // remove temp files
    SysTools::RemoveFile(m_strTempDir + L"/" + filenameOnly + L".triangles.g3d");
    SysTools::RemoveFile(m_strTempDir + L"/" + filenameOnly + L".lines.g3d");

  } else {
    QMessageBox errorMessage;
    errorMessage.setText("ImageVis3D Mobile Device Transfer only supported for UVF datasets.");
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.exec();  
    T_ERROR("ImageVis3D Mobile Device Transfer only supported for UVF datasets.");
  }
}
