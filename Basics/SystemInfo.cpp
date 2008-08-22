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

/**
  \file    SystemInfo.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \date    July 2008
*/

#include "SystemInfo.h"

SystemInfo::SystemInfo()
{
  m_iProgrammBitWith = sizeof(void*)*8;
  m_iNumberofCPUs = GetNumCPUs();
  m_iCPUMemSize   = GetCPUMemSize();
  m_iGPUMemSize   = GetGPUMemory();
}

unsigned int SystemInfo::GetNumCPUs() {
  #ifdef _WIN32
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo); 
    return siSysInfo.dwNumberOfProcessors;
  #else
    // TODO
    return 0;
  #endif
}

UINT64 SystemInfo::GetCPUMemSize() {
  #ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);
    return statex.ullTotalPhys;
  #else
    // TODO
    return 0;
  #endif
}

#ifdef _WIN32
  #define INITGUID
  #include <windows.h>
  #include <string.h>
  #include <stdio.h>
  #include <assert.h>
  #include <oleauto.h>
  #include <initguid.h>
  #include <wbemidl.h>
  #include <multimon.h>

  #pragma comment( lib, "wbemuuid.lib" )

  typedef BOOL ( WINAPI* PfnCoSetProxyBlanket )( IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc,
                                                 OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel,
                                                 RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities );
  UINT64 SystemInfo::GetGPUMemory( )
  {

    UINT64 pdwAdapterRam;

    HRESULT hr;
    bool bGotMemory = false;
    HRESULT hrCoInitialize = S_OK;
    IWbemLocator* pIWbemLocator = NULL;
    IWbemServices* pIWbemServices = NULL;
    BSTR pNamespace = NULL;

    pdwAdapterRam = 0;
    hrCoInitialize = CoInitialize( 0 );

    hr = CoCreateInstance( CLSID_WbemLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           ( LPVOID* )&pIWbemLocator );
  #ifdef PRINTF_DEBUGGING
    if( FAILED( hr ) ) wprintf( L"WMI: CoCreateInstance failed: 0x%0.8x\n", hr );
  #endif

    if( SUCCEEDED( hr ) && pIWbemLocator )
    {
        // Using the locator, connect to WMI in the given namespace.
        pNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );

        hr = pIWbemLocator->ConnectServer( pNamespace, NULL, NULL, 0L,
                                           0L, NULL, NULL, &pIWbemServices );
        if( SUCCEEDED( hr ) && pIWbemServices != NULL )
        {
            HINSTANCE hinstOle32 = NULL;

            hinstOle32 = LoadLibraryW( L"ole32.dll" );
            if( hinstOle32 )
            {
                PfnCoSetProxyBlanket pfnCoSetProxyBlanket = NULL;

                pfnCoSetProxyBlanket = ( PfnCoSetProxyBlanket )GetProcAddress( hinstOle32, "CoSetProxyBlanket" );
                if( pfnCoSetProxyBlanket != NULL )
                {
                    // Switch security level to IMPERSONATE. 
                    pfnCoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                          RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );
                }

                FreeLibrary( hinstOle32 );
            }

            IEnumWbemClassObject* pEnumVideoControllers = NULL;
            BSTR pClassName = NULL;

            pClassName = SysAllocString( L"Win32_VideoController" );

            hr = pIWbemServices->CreateInstanceEnum( pClassName, 0,
                                                     NULL, &pEnumVideoControllers );

            if( SUCCEEDED( hr ) && pEnumVideoControllers )
            {
                IWbemClassObject* pVideoControllers[10] = {0};
                DWORD uReturned = 0;
                BSTR pPropName = NULL;

                // Get the first one in the list
                pEnumVideoControllers->Reset();
                hr = pEnumVideoControllers->Next( 5000,             // timeout in 5 seconds
                                                  10,                  // return the first 10
                                                  pVideoControllers,
                                                  &uReturned );

                VARIANT var;
                if( SUCCEEDED( hr ) )
                {
                    bool bFound = false;
                    for( UINT iController = 0; iController < uReturned; iController++ )
                    {
                        pPropName = SysAllocString( L"PNPDeviceID" );
                        hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, NULL, NULL );
                        if( SUCCEEDED( hr ) )
                        {
                                bFound = true;
                        }
                        VariantClear( &var );
                        if( pPropName ) SysFreeString( pPropName );

                        if( bFound )
                        {
                            pPropName = SysAllocString( L"AdapterRAM" );
                            hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, NULL, NULL );
                            if( SUCCEEDED( hr ) )
                            {
                                bGotMemory = true;
                                pdwAdapterRam = var.ulVal;
                            }
                            VariantClear( &var );
                            if( pPropName ) SysFreeString( pPropName );
                            break;
                        }
                        pVideoControllers[iController]->Release();
                    }
                }
            }

            if( pClassName )
                SysFreeString( pClassName );
            pEnumVideoControllers->Release();
        }

        if( pNamespace )
            SysFreeString( pNamespace );
        pIWbemServices->Release();
    }

    pIWbemLocator->Release();

    if( SUCCEEDED( hrCoInitialize ) )
        CoUninitialize();

    if( bGotMemory )
        return pdwAdapterRam;
    else
        return 0;
  }

#else
  UINT64 SystemInfo::GetGPUMemory( )
  {
    // TODO
    return 0;
  }
#endif
