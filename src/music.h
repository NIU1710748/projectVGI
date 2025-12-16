#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <map>
#include <windows.h>

// Identificadores de Sonido
#define ID_MUSIC      L"bg_music"
#define ID_STEPS      L"fx_steps"
#define ID_ITEM       L"fx_item"
#define ID_STAIRS     L"fx_stairs"
#define ID_CHEST      L"fx_chest"
#define ID_QUACK      L"fx_quack"
#define ID_DOOR       L"fx_door"
#define ID_FLASH_ON   L"fx_flash_on"
#define ID_FLASH_OFF  L"fx_flash_off"
#define ID_GUN		  L"fx_gun"
#define ID_OPEN_BOOK  L"fx_open_book"
#define ID_CLOSE_BOOK L"fx_close_book"
#define ID_MENU_HOVER L"fx_menu_hover"
#define ID_MENU_SELECT L"fx_menu_select"
#define ID_WAKING L"fx_waking"
#define ID_LEATHER L"fx_leather"
#define ID_MUSIC_2 L"bg_music_2"
#define ID_LEVER L"fx_lever"
#define ID_CLICK L"fx_click"
#define ID_FINAL L"fx_final"
#define ID_GADGET L"fx_gadget"
#define ID_WOOD L"fx_wood"



// --- VARIABLES GLOBALES DE VOLUMEN (Compartidas con ImGui) ---
extern int g_musicVolume; // Volumen Música (0-100)
extern int g_sfxVolume;   // Volumen Efectos (0-100)
extern const std::wstring SFX_IDS[];
extern const size_t SFX_COUNT;

// --- NUEVA ESTRUCTURA: CONFIGURACIÓN DE VOLUMEN BASE ---
struct SoundConfig {
    int defaultVolume; // Volumen inicial (0-100)
    bool isMusic;      // Para saber si debe buclear o no
};

// Gestión del sistema
void InitAudio();
void ShutdownAudio();

// Carga de audio (Ahora requiere la configuración base)
bool LoadAudio(const std::wstring& path, const std::wstring& id, int defaultVolume);

// --- REPRODUCCIÓN ---

// Reproduce música en bucle (Chequeado manualmente)
void PlayMusicLoop(const std::wstring& id);

// Reproduce UNA VEZ (Optimizado para pasos)
void PlaySoundOnce(const std::wstring& id);

// Detener sonido
void StopSound(const std::wstring& id);

// Estado
bool IsPlaying(const std::wstring& id);

// --- VOLUMEN (Ahora aplica volumen base + máster) ---
void SetVolume(const std::wstring& id, int masterVolume);

#endif