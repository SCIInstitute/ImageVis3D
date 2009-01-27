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


//!    File   : RenderWindowDX.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#if defined(_WIN32) && defined(USE_DIRECTX)

#pragma comment( lib, "d3d10.lib" )
#ifdef _DEBUG
  #pragma comment( lib, "d3dx10d.lib" )
#else
  #pragma comment( lib, "d3dx10.lib" )
#endif


#include "RenderWindowDX.h"
#include "ImageVis3D.h"

#include <QtGui/QtGui>
#include <assert.h>
#include <sstream>
#include "../Tuvok/Renderer/GL/GLFrameCapture.h"

using namespace std;

RenderWindowDX::RenderWindowDX(MasterController& masterController,
                               MasterController::EVolumeRendererType eType,
                               const QString& dataset,
                               unsigned int iCounter,
                               bool bUseOnlyPowerOfTwo,
                               bool bDownSampleTo8Bits,
                               bool bDisableBorder,
                               QWidget* parent,
                               Qt::WindowFlags flags) :
  QWidget(parent, flags),
  RenderWindow(masterController, eType, dataset, iCounter, parent),
  m_hInst(NULL),
  m_driverType(D3D10_DRIVER_TYPE_NULL),
  m_pd3dDevice(NULL),
  m_pSwapChain(NULL),
  m_pRenderTargetView(NULL)

{  
  setBaseSize( sizeHint() );
  m_Renderer = masterController.RequestNewVolumerenderer(eType, bUseOnlyPowerOfTwo, bDownSampleTo8Bits, bDisableBorder);

  if (m_Renderer) {
    m_Renderer->LoadDataset(m_strDataset.toStdString());
    InitializeRenderer();
    SetupArcBall();
  } m_bRenderSubsysOK = false;

  setObjectName("RenderWindowDX");  // this is used by WidgetToRenderWin() to detect the type
  setWindowTitle(m_strID);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

RenderWindowDX::~RenderWindowDX() 
{
}

void RenderWindowDX::InitializeRenderer()
{
  if (m_Renderer == NULL) 
    m_bRenderSubsysOK = false;
  else
    m_bRenderSubsysOK = m_Renderer->Initialize();

  if (!m_bRenderSubsysOK) {
    m_Renderer->Cleanup();
    m_MasterController.ReleaseVolumerenderer(m_Renderer);
    m_Renderer = NULL;
  } 
}

void RenderWindowDX::ForceRepaint() {
  repaint();
#ifdef TUVOK_OS_APPLE
  QCoreApplication::processEvents();
#endif
}

void RenderWindowDX::ToggleFullscreen() {
  // TODO
}

void RenderWindowDX::resizeEvent ( QResizeEvent * event ) {
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

    D3D10_DRIVER_TYPE driverTypes[] =
    {
        D3D10_DRIVER_TYPE_HARDWARE,
        D3D10_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = sizeof( driverTypes ) / sizeof( driverTypes[0] );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof(sd) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = event->size().width();
    sd.BufferDesc.Height = event->size().height();
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = this->winId();
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D10CreateDeviceAndSwapChain( NULL, m_driverType, NULL, createDeviceFlags,
                                            D3D10_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return;  // TODO report failiure

    // Create a render target view
    ID3D10Texture2D* pBackBuffer;
    hr = m_pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return;  // TODO: report failiure

    hr = m_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &m_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return;  // TODO: report failiure

    m_pd3dDevice->OMSetRenderTargets( 1, &m_pRenderTargetView, NULL );

    // Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = event->size().width();
    vp.Height = event->size().height();
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_pd3dDevice->RSSetViewports( 1, &vp );
}

#endif // _WIN32 && USE_DIRECTX