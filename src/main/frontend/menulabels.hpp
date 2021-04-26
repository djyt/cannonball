#pragma once

// ------------------------------------------------------------------------------------------------
// Text Labels for menus
// ------------------------------------------------------------------------------------------------

// Back Labels
const static char* ENTRY_BACK = "BACK";

// Main Menu
const static char* ENTRY_PLAYGAME = "PLAY GAME";
const static char* ENTRY_GAMEMODES = "GAME MODES";
const static char* ENTRY_SETTINGS = "SETTINGS";
const static char* ENTRY_ABOUT = "ABOUT";
const static char* ENTRY_EXIT = "EXIT";

// Main Menu (cabinet)
const static char* ENTRY_DIPS       = "DIP SWITCHES";
const static char* ENTRY_CABTESTS   = "HARDWARE TESTS";
const static char* ENTRY_EXSETTINGS = "EXTRA SETTINGS";

// Game Modes Menu
const static char* ENTRY_ENHANCED = "SET ENHANCED MODE";
const static char* ENTRY_ORIGINAL = "SET ORIGINAL MODE";
const static char* ENTRY_CONT = "CONTINUOUS MODE";
const static char* ENTRY_TIMETRIAL = "TIME TRIAL MODE";

// Time Trial Menu
const static char* ENTRY_START = "START TIME TRIAL";
const static char* ENTRY_LAPS = "NO OF LAPS ";

// Continuous Menu
const static char* ENTRY_START_CONT = "START CONTINUOUS MODE";

// Settings Menu
const static char* ENTRY_VIDEO = "VIDEO";
const static char* ENTRY_SOUND = "SOUND";
const static char* ENTRY_CONTROLS = "CONTROLS";
const static char* ENTRY_ENGINE = "GAME ENGINE";
const static char* ENTRY_SCORES = "CLEAR HISCORES";
const static char* ENTRY_SAVE = "SAVE AND RETURN";
const static char* ENTRY_ENHANCE = "ENHANCEMENTS";

// SMARTYPI Extra Settings
const static char* ENTRY_S_CAB = "CABINET TYPE ";
const static char* ENTRY_FREEPLAY = "FREEPLAY ";
const static char* ENTRY_S_MOTOR = "MOTOR TEST";
const static char* ENTRY_S_INPUTS = "INPUT TEST";
const static char* ENTRY_S_OUTPUTS = "OUTPUT TEST";
const static char* ENTRY_S_CRT = "CRT TEST";
const static char* ENTRY_TIMER = "TIMING FIXES ";
const static char* ENTRY_S_BUGS = "BUG FIXES ";

// Video Menu
const static char* ENTRY_FPS = "FRAME RATE ";
const static char* ENTRY_FULLSCREEN = "FULL SCREEN ";
const static char* ENTRY_WIDESCREEN = "WIDESCREEN ";
const static char* ENTRY_HIRES = "HIRES ";
const static char* ENTRY_SCALE = "WINDOW SCALE ";
const static char* ENTRY_SCANLINES = "SCANLINES ";

// Sound Menu
const static char* ENTRY_MUTE = "SOUND ";
const static char* ENTRY_BGM = "BGM VOL ";
const static char* ENTRY_SFX = "SFX VOL ";
const static char* ENTRY_ADVERTISE = "ATTRACT SOUND ";
const static char* ENTRY_PREVIEWSND = "PREVIEW MUSIC ";
const static char* ENTRY_FIXSAMPLES = "FIX SAMPLES ";
const static char* ENTRY_MUSICTEST = "MUSIC TEST";

// Controls Menu
const static char* ENTRY_GEAR = "GEAR ";
const static char* ENTRY_CONFIGUREGP = "CONFIGURE GAMEPAD";
const static char* ENTRY_REDEFKEY = "REDEFINE KEYS";
const static char* ENTRY_DSTEER = "DIGITAL STEER SPEED ";
const static char* ENTRY_DPEDAL = "DIGITAL PEDAL SPEED ";

// GamePad Menu
const static char* ENTRY_ANALOG = "ANALOG ";
const static char* ENTRY_RUMBLE = "RUMBLE STRENGTH ";
const static char* ENTRY_REDEFJOY = "REDEFINE GAMEPAD";

// Game Engine Menu
const static char* ENTRY_TRACKS = "TRACKS ";
const static char* ENTRY_TIME = "TIME ";
const static char* ENTRY_TRAFFIC = "TRAFFIC ";
const static char* ENTRY_SUB_ENHANCEMENTS = "ENHANCEMENTS";
const static char* ENTRY_SUB_HANDLING = "CAR SETUP";

// Game Engine: Enhancements Sub-Menu
const static char* ENTRY_ATTRACT = "NEW ATTRACT ";
const static char* ENTRY_PROTOTYPE = "PROTOTYPE STAGE 1 ";
const static char* ENTRY_OBJECTS = "OBJECTS ";

// Game Engine: Car Setup Sub-Menu
const static char* ENTRY_GRIP    = "GRIPPY TYRES ";
const static char* ENTRY_OFFROAD = "OFFROAD TYRES ";
const static char* ENTRY_BUMPER  = "STRONG BUMPER ";
const static char* ENTRY_TURBO   = "FASTER CAR ";
const static char* ENTRY_COLOR   = "COLOR ";

const static char* COLOR_LABELS[5] = { "RED", "BLUE", "YELLOW", "GREEN", "CYAN" };

// Music Test Menu
const static char* ENTRY_MUSIC1 = "PLAY TRACK";
const static char* ENTRY_MUSIC2 = "TRACK - ";
const static char* ENTRY_MUSIC3 = "LAST WAVE";

const static char* DIP_DIFFICULTY[4] = { "EASY", "NORMAL", "HARD", "HARDEST" };
const static char* GEAR_LABELS[4] = {"MANUAL", "MANUAL CABINET", "MANUAL 2 BUTTONS", "AUTOMATIC"};
const static char* FPS_LABELS[3] = { "30 FPS", "ORIGINAL", "60 FPS" };
const static char* ANALOG_LABELS[3] = { "OFF", "ON", "ON WHEEL ONLY" };
const static char* VIDEO_LABELS[3] = { "OFF", "ON", "STRETCH" };
const static char* RUMBLE_LABELS[5] = { "OFF", "LOW", "MED", "HIGH", "FULL" };