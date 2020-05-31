/***************************************************************************
    Microsoft DirectX 8 Force Feedback (aka Haptic) Support
    
    - Currently, SDL does not support haptic devices. So this is Win32 only.

    - DirectX 8 still works on Windows XP, so I'm not attempting to support
      a higher version for now. 

    Ref: http://msdn.microsoft.com/en-us/library/windows/desktop/ee417563%28v=vs.85%29.aspx
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "ffeedback.hpp"
#include <stdio.h>

//-----------------------------------------------------------------------------
// Dummy Functions For Non-Windows Builds
//-----------------------------------------------------------------------------
#ifndef WIN32
// Linux Force Feedback support
// V0.1
// (C) Ismas may 2020, first year of COVID-19
// License IWTM (IT Works To Me)
// Shamelessly ripped from linuxconsole utils fftest.c (C) Johan Deneux

namespace forcefeedback {
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/ioctl.h>
    #include <linux/input.h>

    #define N_EFFECTS 6

    struct ff_effect effects[N_EFFECTS];
    struct input_event play, stop, gain;
    int fd;
    const char * devpath = "/dev/input/event";
    char device_file_name[20];
    int i;
    bool supported = false;

    bool init(int max_force, int min_force, int force_duration)
    {
        //printf("INIT max %d min %d duration %d\n",max_force, min_force, force_duration);
        // Locate the wheel by testing 
        memset(&gain, 0, sizeof(gain));
        gain.type = EV_FF;
        gain.code = FF_GAIN;
        gain.value = 0xFFFF; /* [0, 0xFFFF]) */
   
        for (i=0;i<99;i++) {
            sprintf(device_file_name,"%s%d",devpath,i);
            fd = open(device_file_name, O_RDWR);
            if (fd == -1) continue;
            fflush(stdout);
            if (write(fd, &gain, sizeof(gain)) == sizeof(gain)) 
                break;
        }
        if (fd!=-1) {
            printf("Located rumbler %s\n",device_file_name);
            supported = true;
        }
        else {
            perror("Sorry, no rumbler wheel");    
            return false;  
        }

        // Load rumble effects
        memset(&effects[0],0,sizeof(effects[0]));
        // Rotative effect for crashes, stronguest one 
        effects[0].type = FF_PERIODIC;
        effects[0].id = -1;
        effects[0].u.periodic.waveform = FF_SINE;
        effects[0].u.periodic.period = 10;  /* 0.1 second */
        effects[0].u.periodic.magnitude = 0x7fff;   /* 0.5 * Maximum magnitude */
        effects[0].u.periodic.offset = 0;
        effects[0].u.periodic.phase = 0;
        effects[0].direction = 0x4000;  /* Along X axis */
        effects[0].u.periodic.envelope.attack_length = 1000;
        effects[0].u.periodic.envelope.attack_level = 0x7fff;
        effects[0].u.periodic.envelope.fade_length = 1000;
        effects[0].u.periodic.envelope.fade_level = 0x7fff;
        effects[0].trigger.button = 0;
        effects[0].trigger.interval = 0;
        effects[0].replay.length = 500;  /* 0.5 seconds */
        effects[0].replay.delay = 000;
        fflush(stdout);
        if (ioctl(fd, EVIOCSFF, &effects[0]) == -1) {
            perror("Rumble load 0 error:");
        } 
        // Now progressively softer rumbles for drifts 
        for (i=1;i<6;i++) {
            effects[i].type = FF_RUMBLE;
            effects[i].id = -1;
            effects[i].u.rumble.strong_magnitude = max_force/i;
            effects[i].u.rumble.weak_magnitude = min_force/i;
            effects[i].replay.length = force_duration;
            effects[i].replay.delay = 0;
            fflush(stdout);
            if (ioctl(fd, EVIOCSFF, &effects[i]) == -1) {
                perror("Rumble load 1-5 error:");
            } 
        }
        return supported;
    }

    int  set(int command, int force)         {

        // PLAY EFFECT
        // force [0,5] (0 strongest 5 soft)
        // command unused, it's for real motor hardware
        memset(&play,0,sizeof(play));
        play.type = EV_FF;
        play.code = effects[force-1].id; 
        play.value = 1;            
        if (write(fd, (const void*) &play, sizeof(play)) == -1) {
            perror("Play effect");
            return false;
        }
        return true;
    }

    void close() {  
        // Just that  
        close(fd);
    }

    bool is_supported()            { return supported; } // supported or not
};

//-----------------------------------------------------------------------------
// DirectX 8 Code Below
//-----------------------------------------------------------------------------
#else

// DirectX 8 Needed (Windows XP and up)
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <SDL_syswm.h> // Used to get window handle for DirectX

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
LPDIRECTINPUTEFFECT   g_pEffect   = NULL;           // Force Feedback Effect
DWORD                 g_dwNumForceFeedbackAxis = 0;

bool                  g_supported = false;         //  Is Haptic Device Supported?
bool                  g_supported = true;         //  Is Haptic Device Supported?

//-----------------------------------------------------------------------------
// User Configurable Values
//-----------------------------------------------------------------------------

int g_max_force;      // Maximum Force To Apply (0 to DI_FFNOMINALMAX)
int g_min_force;      // Minimum Force To Apply (0 to g_max_force)
int g_force_duration; // Length of each effect. (1/x seconds)

bool init(int max_force, int min_force, int force_duration)
{
    g_max_force      = max_force;
    g_min_force      = min_force;
    g_force_duration = force_duration;

    if (!g_supported)
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

    return g_supported;
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

    g_supported = false;
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
                                                     DISCL_BACKGROUND ) ) )
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
    if (g_force_duration <= 0) 
        g_force_duration = 20;
 
    DWORD            dwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    LONG             lDirection[2] = { 0, 0 };
    DICONSTANTFORCE  cf            = { 0 };    
    DIEFFECT         eff;
    ZeroMemory( &eff, sizeof(eff) );
    eff.dwSize                  = sizeof(DIEFFECT) ;
    eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS; // Using X/Y style coords
    eff.dwDuration              = DI_SECONDS/g_force_duration;           // Duration: 20th of a second (use INFINITE for never ending)
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

    // Denote Force Feedback Device Found & Supported
    g_supported = true;

    return S_OK;
}

// xdirection   = Direction to turn wheel in
//                < 8: Left
//                  8: Centre
//                > 8: Right
// Higher force = Less aggressive steer
int set(int xdirection, int force)
{
    if (!g_supported || g_pEffect == NULL || force < 0)
        return -1;

    LONG lDirection[2] = { 0, 0 }; // centred by default
       
    if (xdirection > 0x08)         // push right
        lDirection[0] = 1;  
    else if (xdirection < 0x08)    // push left
        lDirection[0] = -1;  

    // 7 possible force positions, so divide maximum amount to subtract by 7.
    LONG magnitude = g_max_force - (((g_max_force-g_min_force) / 7) * force);

    // Cap within range
    if (magnitude > DI_FFNOMINALMAX)
        magnitude = DI_FFNOMINALMAX;
    else if (magnitude < -DI_FFNOMINALMAX)
        magnitude = -DI_FFNOMINALMAX;

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

// Is Haptic Device Supported?
bool is_supported()
{
    return g_supported;
}

};

#endif
