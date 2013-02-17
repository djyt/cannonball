//#ifdef WIN32

// DirectX 8 Needed.
// This version works on Windows XP, so best not to go for a higher version.
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <SDL_syswm.h> // Used to get window handle for DirectX
#include <iostream>

#include "ffeedback.hpp"

namespace forcefeedback
{
//-----------------------------------------------------------------------------
// Function prototypes 
//-----------------------------------------------------------------------------
HRESULT InitDirectInput( HWND hDlg );
BOOL CALLBACK EnumFFDevicesCallback( const DIDEVICEINSTANCE* pInst, VOID* pContext );
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
HRESULT       InitForceEffects();

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

LPDIRECTINPUT8        g_pDI       = NULL;         
LPDIRECTINPUTDEVICE8  g_pDevice   = NULL;
LPDIRECTINPUTEFFECT   g_pEffect   = NULL; // Force Feedback Effect

DWORD                 g_dwNumForceFeedbackAxis = 0;

int diMaxForce  = 10000;
int diMinForce  = -1000;

// Lower = turn more slowly
int diForceFreq = 1;

void init()
{
    // Platform Specific SDL code to get a window handle
    SDL_SysWMinfo i;
    SDL_VERSION(&i.version); 
    if (SDL_GetWMInfo(&i))
    {
        HWND hwnd = i.window;
        InitDirectInput(hwnd);
    }
}

void close()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    if( g_pDevice ) 
        g_pDevice->Unacquire();
    
    // Release any DirectInput objects.
    SAFE_RELEASE( g_pEffect );
    SAFE_RELEASE( g_pDevice );
    SAFE_RELEASE( g_pDI );
}

HRESULT InitDirectInput( HWND hDlg )
{
    HRESULT hr;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                                         IID_IDirectInput8, (VOID**)&g_pDI, NULL ) ) )
    {
        return hr;
    }

    // Look for a force feedback device we can use
    if( FAILED( hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,
                                        EnumFFDevicesCallback, NULL,
                                        DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK )))
    {
        return hr;
    }

    if( NULL == g_pDevice )
    {
        // Force Feedback Device Not Found
        return S_OK;
    }

    // Set the data format to "simple joystick" - a predefined data format. A
    // data format specifies which controls on a device we are interested in,
    // and how they should be reported.
    //
    // This tells DirectInput that we will be passing a DIJOYSTATE structure to
    // IDirectInputDevice8::GetDeviceState(). Even though we won't actually do
    // it in this sample. But setting the data format is important so that the
    // DIJOFS_* values work properly.
    if( FAILED( hr = g_pDevice->SetDataFormat( &c_dfDIJoystick ) ) )
        return hr;

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    // Exclusive access is required in order to perform force feedback.
    if( FAILED( hr = g_pDevice->SetCooperativeLevel( hDlg,
                                                     DISCL_EXCLUSIVE | 
                                                     DISCL_FOREGROUND ) ) )
    {
        return hr;
    }

    // Since we will be playing force feedback effects, we should disable the
    // auto-centering spring.
    DIPROPDWORD dipdw;
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = FALSE;
    if( FAILED( hr = g_pDevice->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph ) ) )
        return hr;

    // Enumerate and count the axes of the joystick 
    if ( FAILED( hr = g_pDevice->EnumObjects( EnumAxesCallback, 
                                              (VOID*)&g_dwNumForceFeedbackAxis, DIDFT_AXIS ) ) )
        return hr;

    // We only support one/two axis joysticks
    if( g_dwNumForceFeedbackAxis > 2 )
        g_dwNumForceFeedbackAxis = 2;

    g_pDevice->Acquire();
    return InitForceEffects();
}

//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//       each force feedback enabled axis
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                VOID* pContext )
{
    DWORD* pdwNumForceFeedbackAxis = (DWORD*) pContext;

    if( (pdidoi->dwFlags & DIDOI_FFACTUATOR) != 0 )
        (*pdwNumForceFeedbackAxis)++;

    return DIENUM_CONTINUE;
}


//-----------------------------------------------------------------------------
// Name: EnumFFDevicesCallback()
// Desc: Called once for each enumerated force feedback device. If we find
//       one, create a device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumFFDevicesCallback( const DIDEVICEINSTANCE* pInst, 
                                            VOID* pContext )
{
    LPDIRECTINPUTDEVICE8 pDevice;
    HRESULT              hr;

    // Obtain an interface to the enumerated force feedback device.
    hr = g_pDI->CreateDevice( pInst->guidInstance, &pDevice, NULL );

    // If it failed, then we can't use this device for some
    // bizarre reason.  (Maybe the user unplugged it while we
    // were in the middle of enumerating it.)  So continue enumerating
    if( FAILED(hr) ) 
        return DIENUM_CONTINUE;

    // We successfully created an IDirectInputDevice8.  So stop looking 
    // for another one.
    g_pDevice = pDevice;

    return DIENUM_STOP;
}

HRESULT InitForceEffects()
{
    HRESULT     hr;

    // Cap Default Values
    if (diForceFreq <= 0) 
        diForceFreq = 20;

    if (diMaxForce > DI_FFNOMINALMAX || diMaxForce < -DI_FFNOMINALMAX) 
        diMaxForce = DI_FFNOMINALMAX;
    if (diMinForce > DI_FFNOMINALMAX || diMinForce < -DI_FFNOMINALMAX) 
        diMinForce = 0;
 
    DWORD            dwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    LONG             lDirection[2] = { 0, 0 };
    DICONSTANTFORCE  cf            = { 0 };    
    DIEFFECT         eff;
    ZeroMemory( &eff, sizeof(eff) );
    eff.dwSize                  = sizeof(DIEFFECT) ;
    eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;    // Using X/Y style coords
    eff.dwDuration              = DI_SECONDS/diForceFreq;    // Duration: 20th of a second
    //    eff.dwDuration        = INFINITE;    // Duration: never-ending
    eff.dwSamplePeriod          = 0;
    eff.dwGain                  = DI_FFNOMINALMAX;
    eff.dwTriggerButton         = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes                   = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = dwAxes;
    eff.rglDirection            = lDirection;
    eff.lpEnvelope              = 0;    
    eff.cbTypeSpecificParams    = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams   = &cf;
    eff.dwStartDelay            = 0;

    if (FAILED(hr = g_pDevice->CreateEffect(GUID_ConstantForce, &eff, &g_pEffect, NULL)))
        return hr;

    if (g_pEffect == NULL)
        return E_FAIL;

    std::cout << "all ok!" << std::endl;
    return S_OK;
}

// xdirection   = Direction to turn wheel in
//                < 8: Left
//                  8: Centre
//                > 8: Right
// Higher force = Less aggressive steer
int set(int xdirection, int force)
{
    if (g_pEffect == NULL) return -1;
    if (force < 0) return -1;

    LONG lDirection[2] = { 0, 0 }; // centred by default
       
    if (xdirection > 0x08)         // push right
        lDirection[0] = 1;  
    else if (xdirection < 0x08)    // push left
        lDirection[0] = -1;  

    LONG magnitude = diMaxForce - ((diMaxForce-diMinForce) / 7*force);

    DICONSTANTFORCE cf;    // Type-specific parameters
    cf.lMagnitude = magnitude;

    DIEFFECT eff;
    ZeroMemory( &eff, sizeof(eff) );
    eff.dwSize                = sizeof(DIEFFECT); 
    eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE); 
    eff.lpvTypeSpecificParams = &cf;
    eff.cAxes                 = g_dwNumForceFeedbackAxis;
    eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;    // Using X/Y style coords
    eff.rglDirection          = lDirection;

    return g_pEffect->SetParameters(&eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
}

};

//#endif