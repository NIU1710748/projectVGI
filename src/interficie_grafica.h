#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/nfd.h"              
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <GL/glew.h>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>

#include "../include/gl/SOIL2.h"
#include "music.h"

// ---- Forward declarations per a gamepad + ratolí virtual ----
struct GamepadState {
    bool connected = false;

    float lx = 0.0f, ly = 0.0f;   // stick esquerre
    float rx = 0.0f, ry = 0.0f;   // stick dret
    float lt = 0.0f, rt = 0.0f;   // gatillos [0..1]

    bool btnA = false;
    bool btnB = false;
    bool btnX = false;
    bool btnY = false;
    bool btnLB = false;
    bool btnRB = false;
    bool btnBack = false;
    bool btnStart = false;
    bool btnLThumb = false;
    bool btnRThumb = false;
};


// Aquests venen de main.cpp (definits allà)
extern GamepadState g_Pad;
extern GamepadState g_PadPrev;

extern bool g_UsePadMouse;
extern bool g_InventariObert;

// Funcions definides a main.cpp
void UpdateGamepad(float dt);
void UpdatePadMouseForImGui(GLFWwindow* window);



// Interficies
enum class GameState { INIT, MENU, GAME, OPTIONS, CLUE, LOADING, INSPECTING, END };
GameState act_state = GameState::INIT; // Per veure en quin estat està el joc

// IDs de textura per cada estat
GLuint texInit = 0;
GLuint texMenu = 0;
GLuint texOptions = 0;
GLuint texLoading = 0;

//-------- Fonts de text ---------//

void InicialitzarImGuiFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;

    static const ImWchar ranges[] = { 0x0020, 0x00FF,0, };

    io.Fonts->AddFontFromFileTTF("../EntornVGI/Fonts/Titania-Regular.ttf", 15.0f, &config, ranges);
}
static ImFont* g_font_playball = nullptr;
static ImFont* g_font_junigarden = nullptr;
static ImFont* g_font_titania = nullptr;
static ImFont* g_font_aurora = nullptr;
static int g_font_txt = 0;

void LoadAllFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0 };

    g_font_playball = io.Fonts->AddFontFromFileTTF("../EntornVGI/Fonts/Titania-Regular.ttf", 15.0f, &cfg, ranges);
    g_font_junigarden = io.Fonts->AddFontFromFileTTF("../EntornVGI/Fonts/JunigardenSerif.otf", 16.0f, &cfg, ranges);
    g_font_titania = io.Fonts->AddFontFromFileTTF("../EntornVGI/Fonts/Playball.ttf", 16.0f, &cfg, ranges);
    g_font_aurora = io.Fonts->AddFontFromFileTTF("../EntornVGI/Fonts/Aurora.otf", 16.0f, &cfg, ranges);
    printf("[Carreguant] fonts per joc");

    io.Fonts->Build();
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_CreateFontsTexture();

    io.FontDefault = g_font_playball;
}

void ApplySelectedFont()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFont* chosen = nullptr;
    switch (g_font_txt) {
    case 0: chosen = g_font_titania; break;
    case 1: chosen = g_font_junigarden; break;
    case 2: chosen = g_font_playball; break;
    case 3: chosen = g_font_aurora; break;
    }
    if (chosen) io.FontDefault = chosen;
}

// -------------------- Utilitats de textures --------------------

void deleteTexture(GLuint& textureID)
{
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
}

GLuint loadTextureReturnID(const char* path)
{
    GLuint texID = SOIL_load_OGL_texture(
        path,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (texID == 0)
        std::cout << "Error carregant la textura amb SOIL2: "
        << SOIL_last_result() << std::endl;

    return texID;
}

// Carregar totes les textures al principi
void initTextures()
{
    texInit = loadTextureReturnID("../EntornVGI/textures/backgrounds/pantalla_inici.png");
    texMenu = loadTextureReturnID("../EntornVGI/textures/backgrounds/menu_llibre.png");
    texOptions = loadTextureReturnID("../EntornVGI/textures/backgrounds/menu_llibre.png");
    texLoading = loadTextureReturnID("../EntornVGI/textures/backgrounds/planol_vaixell.png");
}

// Alliberar totes les textures al final
void cleanupTextures()
{
    deleteTexture(texInit);
    deleteTexture(texMenu);
    deleteTexture(texOptions);
    deleteTexture(texLoading);
}

// -------------------- Dibuix --------------------

void draw_image(GLuint textureID, int width, int height, int x = 0, int y = 0)
{
    if (textureID != 0)
    {
        ImVec2 imagePos(x, y);
        ImVec2 imageSize(width, height);
        ImGui::GetBackgroundDrawList()->AddImage(
            (ImTextureID)(intptr_t)textureID,
            imagePos,
            ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
    }
}

void SetupHUD(GLFWwindow* window, int& display_h,
    float sizeFactorX = 0.25f, float sizeFactorY = 0.5f)
{
    int display_w;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w * sizeFactorX, display_h * sizeFactorY);
    ImVec2 hudPos((display_w - hudSize.x) * 0.5f, (display_h - hudSize.y) * 0.5f);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
}

/// SO FUNCIONS
static ImGuiID g_LastHoveredID = 0;

bool ButtonWithSound(const char* label, const ImVec2& size)
{
    // 1. Dibujamos el botón y guardamos si ha sido clicado
    bool pressed = ImGui::Button(label, size);

    // ---------------------------------------------------------
    // NUEVO: Lógica del sonido al hacer CLIC
    // ---------------------------------------------------------
    if (pressed)
    {
        PlaySoundOnce(ID_MENU_SELECT);
    }

    // ---------------------------------------------------------
    // Lógica del sonido al hacer HOVER (Igual que antes)
    // ---------------------------------------------------------
    ImGuiID currentID = ImGui::GetID(label);

    if (ImGui::IsItemHovered())
    {
        // Si entramos al botón y no estaba sonando ya...
        if (g_LastHoveredID != currentID)
        {
            PlaySoundOnce(ID_MENU_HOVER);
            g_LastHoveredID = currentID;
        }
    }
    else
    {
        // Si salimos del botón, reseteamos para que pueda volver a sonar
        if (g_LastHoveredID == currentID)
        {
            g_LastHoveredID = 0;
        }
    }

    return pressed;
}


// -------------------- Render Inici --------------------

void renderInici(GLFWwindow* window)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        UpdateGamepad(dtPad);
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
            UpdatePadMouseForImGui(window);
    }


    int display_w, display_h;
    glfwGetWindowSize(window, &display_w, &display_h);

    draw_image(texInit, display_w, display_h);

    // La finestra cobreix tota la imatge
    ImVec2 hudSize(display_w, display_h);
    ImVec2 hudPos(0, 0);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::Begin("Inici", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar);

    ImVec2 buttonSize(display_w / 8.0f, display_h / 14.0f);
    float fontScale = display_h / 400.0f;
    ImGui::SetWindowFontScale(fontScale);


    float quadrantX = display_w * 0.6f;
    float quadrantY = display_h * 0.5f;
    float quadrantWidth = display_w * 0.5f;
    float quadrantHeight = display_h * 0.5f;

    float spacing = 50.0f;
    int numButtons = 2;
    float totalHeight = buttonSize.y * numButtons + spacing * (numButtons - 1);

    // Centre del quadrant
    float centerX = quadrantX + quadrantWidth * 0.5f;
    float centerY = quadrantY + quadrantHeight * 0.5f;

    // Posició inicial centrada
    float startX = centerX - buttonSize.x * 0.5f;
    float startY = centerY - totalHeight * 0.5f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));

    ImGui::SetCursorPos(ImVec2(startX, startY));
    if (ButtonWithSound("Jugar", buttonSize)) act_state = GameState::LOADING;

    ImGui::SetCursorPos(ImVec2(startX, startY + buttonSize.y + spacing));
    if (ButtonWithSound("Opciones", buttonSize)) act_state = GameState::OPTIONS;

    ImGui::SetCursorPos(ImVec2(startX, startY + buttonSize.y + spacing *4));
    if (ButtonWithSound("Sortir", buttonSize))  glfwSetWindowShouldClose(window, GLFW_TRUE);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::End();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}
// -------------------- Render Menu --------------------

void renderMenu(GLFWwindow* window)
{
    // --- Entrada de GLFW ---
    glfwPollEvents();

    // --- Començar frame ImGui ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        // Actualitzem l'estat del gamepad (sticks + botons)
        UpdateGamepad(dtPad);

        // Al menú sempre volem poder moure el cursor amb el pad si està connectat
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
        {
            // Mou el cursor d'ImGui + el cursor del sistema amb el stick dret
            UpdatePadMouseForImGui(window);
        }
    }

    int display_w = 0, display_h = 0;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w, display_h);
    ImVec2 hudPos(0, 0);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Fons del menú
    draw_image(texMenu, ImGui::GetIO().DisplaySize.x, display_h);

    ImGui::Begin("Menu", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar);

    ImVec2 actualSize = ImGui::GetWindowSize();
    ImVec2 buttonSize(actualSize.x / 4.0f, actualSize.y / 8.0f);

    float rightHalfX = display_w * 0.5f;
    float rightHalfWidth = display_w * 0.5f;

    float centerX = (rightHalfX + (rightHalfWidth - buttonSize.x) * 0.5f) - 20.0f;

    float offsetY = display_h * 0.12f;
    ImGui::SetCursorPosY(offsetY);

    float fontScale = display_h / 350.0f;
    ImGui::SetWindowFontScale(fontScale);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));

    ImGui::SetCursorPosX(centerX);
    if (ButtonWithSound("Continuar", buttonSize))
        act_state = GameState::GAME;
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGui::SetCursorPosX(centerX);
    if (ButtonWithSound("Pistas", buttonSize))
        act_state = GameState::CLUE;
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGui::SetCursorPosX(centerX);
    if (ButtonWithSound("Opcions", buttonSize))
        act_state = GameState::OPTIONS;
    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGui::SetCursorPosX(centerX);
    if (ButtonWithSound("Sortir", buttonSize))
    {
        Sleep(1000);
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::End();

    // --- Render final ---
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

// -------------------- Render Options --------------------

void renderOptions(GLFWwindow* window)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        UpdateGamepad(dtPad);
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
            UpdatePadMouseForImGui(window);
    }

    int display_w = 0, display_h = 0;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w, display_h);
    ImVec2 hudPos(0, 0);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    draw_image(texMenu, ImGui::GetIO().DisplaySize.x, display_h);

    ImGui::Begin("Options", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar);

    ImVec2 actualSize = ImGui::GetWindowSize();
    ImVec2 buttonSize(actualSize.x / 4.0f, actualSize.y / 8.0f);

    float rightHalfX = display_w * 0.5f;
    float rightHalfWidth = display_w * 0.5f;

    float centerX = (rightHalfX + (rightHalfWidth - buttonSize.x) * 0.5f) - 20.0f;

    float offsetY = display_h * 0.2f;
    ImGui::SetCursorPosY(offsetY);

    float fontScale = display_h / 350.0f;
    ImGui::SetWindowFontScale(fontScale);

    // -- Estil buttons
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));

    // -- Estil Sliders y Ratio
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.5f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // punt interior



    // --- SLIDER MÚSICA ---
    ImGui::SetCursorPosX(centerX);
    ImGui::Text("Volum Musica");
    ImGui::SameLine();

    ImGui::PushItemWidth(int(actualSize.x / 6));
    if (ImGui::SliderInt("##vol_music", &g_musicVolume, 0, 100))
        SetVolume(ID_MUSIC, g_musicVolume * 0.15);
    ImGui::PopItemWidth(); // CERRAR WIDTH DE MÚSICA

    // --- SEPARADOR ---
    ImGui::NewLine();
    offsetY = ImGui::GetCursorPosY(); // Capturar posición actual
    ImGui::SetCursorPosY(offsetY);

    // --- SLIDER EFECTOS (SFX) ---
    ImGui::SetCursorPosX(centerX);
    ImGui::Text("Volum Efectes");
    ImGui::SameLine();

    ImGui::PushItemWidth(int(actualSize.x / 6));
    if (ImGui::SliderInt("##vol_sfx", &g_sfxVolume, 0, 100))
    {
        // Aplicamos volumen máster a todos los demás efectos usando el array
        for (size_t i = 0; i < SFX_COUNT; ++i) {
            SetVolume(SFX_IDS[i], g_sfxVolume);
        }

    }

    ImGui::NewLine();
    ImGui::SetCursorPosX(centerX);
    ImGui::Text("Brillantor ");


    ImGui::SetCursorPosX(centerX);
    ImGui::Text("Tipus de Font: ");

    ImGui::SetCursorPosX(centerX);
    if (ImGui::RadioButton("estil_1", g_font_txt == 0)) { g_font_txt = 0; ApplySelectedFont(); }
    ImGui::SameLine();
    if (ImGui::RadioButton("estil_2", g_font_txt == 1)) { g_font_txt = 1; ApplySelectedFont(); }

    ImGui::SetCursorPosX(centerX);
    if (ImGui::RadioButton("estil_3", g_font_txt == 2)) { g_font_txt = 2; ApplySelectedFont(); }
    ImGui::SameLine();
    if (ImGui::RadioButton("estil_4", g_font_txt == 3)) { g_font_txt = 3; ApplySelectedFont(); }



    //ImGui::SliderFloat("Brillantor", &0, 0.5f, 1.0f);

    offsetY = display_h * 0.6f;
    ImGui::SetCursorPosY(offsetY);
    ImGui::SetCursorPosX(centerX);
    if (ButtonWithSound("Enrere", buttonSize)) act_state = GameState::MENU;

    // ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(10);
    ImGui::End();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

// -------------------- Render Item Descriptions --------------------

void renderItemDescription(GLFWwindow* window, const std::string& itemName, const std::string& itemDescription)
{
    // --- Entrada de GLFW ---
    glfwPollEvents();

    // --- Començar frame ImGui ---
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        // Actualitzem l'estat del gamepad (sticks + botons)
        UpdateGamepad(dtPad);

        // Permetre el cursor amb pad si està connectat
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
        {
            // Mou el cursor d'ImGui + el cursor del sistema amb el stick dret
            UpdatePadMouseForImGui(window);
        }
    }

    // Obtenir mida de la finestra
    int display_w = 0, display_h = 0;
    glfwGetWindowSize(window, &display_w, &display_h);

    // Mida de la finestra de descripció (percentatge de la pantalla)
    ImVec2 windowSize(display_w * 0.4f, display_h * 0.7f);

    // Posicionar al centre de la pantalla
    ImVec2 windowPos(display_w - windowSize.x - 40.0f, (display_h - windowSize.y) * 0.5f);

    // Estil de la finestra (fons semi-transparent amb vores arrodonides)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 15.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.6f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    // Flags per a la finestra (sense barra de títol, sense redimensionar)
    ImGui::Begin("Item Description", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar);

    // Estil per al text
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Nom de l'ítem (centrat i més gran)
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // Font per defecte o una més gran si tens
    ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize(itemName.c_str()).x) * 0.5f);
    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "%s", itemName.c_str());
    ImGui::PopFont();

    // Separador
    ImGui::Separator();
    ImGui::Spacing();

    // Descripció de l'ítem (text ajustat)
    ImGui::PushTextWrapPos(windowSize.x - 40.0f); // Wrap text dins la finestra
    ImGui::TextWrapped("%s", itemDescription.c_str());
    ImGui::PopTextWrapPos();

    ImGui::PopStyleColor(); // ImGuiCol_Text

    // Botó per tancar (al peu de la finestra)
    ImGui::SetCursorPosY(windowSize.y - 60.0f);
    ImGui::Separator();
    ImGui::Spacing();

    ImVec2 buttonSize(windowSize.x * 0.3f, 35.0f);
    ImGui::SetCursorPosX((windowSize.x - buttonSize.x) * 0.5f);

    // Estil del botó
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.5f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.6f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.4f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

    if (ImGui::Button("Tancar", buttonSize))
    {
        // Aquí pots afegir la lògica per tancar la finestra de descripció
        // Per exemple: showItemDescription = false;
    }

    // Si vols permetre tancar amb la tecla ESC
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        // Lògica per tancar la finestra
    }

    ImGui::PopStyleVar(); // FrameRounding
    ImGui::PopStyleColor(3); // Colors del botó

    ImGui::End();

    // Restaurar estils
    ImGui::PopStyleColor(3); // Colors de la finestra
    ImGui::PopStyleVar(2); // WindowRounding i WindowPadding

    // --- Render final ---
    // IMPORTANT: No fem glClear aquí perquè volem veure el joc al fons
    // El joc ja hauria de ser renderitzat abans de cridar aquesta funció

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}


//----------- Extres-----
GLuint texCandelabreOn = 0;
GLuint texCandelabreOff = 0;
GLuint texPistas = 0;
GLuint texCrono = 0;
GLuint texEnd = 0;
GLuint texMarcSimbols = 0;
GLuint texInventariFons = 0;

void initExtraTextures()
{
    texCandelabreOn = loadTextureReturnID("../EntornVGI/textures/hud/candelabre.png");
    texCandelabreOff = loadTextureReturnID("../EntornVGI/textures/hud/candelabre - tancat.png");
    texPistas = loadTextureReturnID("../EntornVGI/textures/backgrounds/llibre_vuit.png");
    texCrono = loadTextureReturnID("../EntornVGI/textures/hud/cronometre.png");
    texEnd = loadTextureReturnID("../EntornVGI/textures/backgrounds/end_balena.png");
    texMarcSimbols = loadTextureReturnID("../EntornVGI/textures/minijocs/marc_simbols.png");
    texInventariFons = loadTextureReturnID("../EntornVGI/textures/inventory/inventari_bag.png");
    printf("[Carreguant] imatges per joc");
}


void cleanupExtraTextures()
{
    deleteTexture(texCandelabreOn);
    deleteTexture(texCandelabreOff);
    deleteTexture(texPistas);
    deleteTexture(texCrono);
    deleteTexture(texEnd);
    deleteTexture(texMarcSimbols);
    deleteTexture(texInventariFons);
}


void renderInstruccions(GLFWwindow* window)
{
    int display_w, display_h;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w / 2.5f, display_h / 10.0f);
    ImVec2 hudPos(0.0f, display_h - hudSize.y);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.2f);

    ImGui::Begin("Instrucciones", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);

    float fontScale = display_h / 600.0f;
    ImGui::SetWindowFontScale(fontScale);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

    ImGui::TextWrapped("(ESC) per obrir menu");
    ImGui::TextWrapped("(I) obrir inventari");

    ImGui::PopStyleColor();
    ImGui::End();
}

// -------------------- Render Candelabre --------------------
void renderCandelabre(GLFWwindow* window, bool& g_HeadlightEnabled)
{
    int display_w, display_h;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w / 200.0f, display_h / 200.0f);
    ImVec2 hudPos(display_w, display_h - hudSize.y);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);

    GLuint tex = g_HeadlightEnabled ? texCandelabreOn : texCandelabreOff;
    draw_image(tex, display_w / 6.0f, display_h / 6.0f,
        hudPos.x / 1.25f, hudPos.y / 1.25f);

    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::Begin("candelabre", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::End();
}

// -------------------- Render Loading --------------------
void renderLoading(GLFWwindow* window, float progress)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        UpdateGamepad(dtPad);
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
            UpdatePadMouseForImGui(window);
    }

    int display_w, display_h;
    glfwGetWindowSize(window, &display_w, &display_h);

    draw_image(texLoading, display_w, display_h);

    ImVec2 winSize(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(winSize);

    ImGui::Begin("Loading", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground);


    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.4f, 0.4f, 0.4f, 0.8f)); // color de la barra
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.8f, 0.8f, 0.8f));       // fons de la barra

    ImVec2 barSize(display_w * 0.5f, 30); // mida de la barra
    ImGui::SetCursorPos(ImVec2((winSize.x - barSize.x) * 0.5f, winSize.y * 0.95f));
    ImGui::ProgressBar(progress, barSize, "Carregant...");

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    ImGui::End();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

// ---------------Cofre contrasenya -------------

GLuint simbol_textures[4];
bool simbols_loaded = false;
std::vector<int> ranures = { 0,0,0,0 };
std::vector<int> solucio = { 2,1,0,3 };
int numSymbols = 4;
bool cofre_on = false;
bool joc_quadres_finalitzat = false;
bool joc_palanques_finalitzat = false;
std::vector<bool> portes_obertes = { false, false, false, false };

bool g_ShowReward = false;
double g_RewardStartTime = 0.0;
double g_RewardDuration = 3.0; // segons

void claimRewards(ImGuiIO& io, int index)
{
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.18f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.0f)
    );

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));

    ImGui::Begin("Reward", nullptr, flags);
    switch (index)
    {
    case 0: ImGui::Text("Has aconseguit una clau rovellada!"); break;
    case 1: ImGui::Text("Has aconseguit una destral!"); break;
    case 2: ImGui::Text("Has aconseguit una clau luxosa!"); break;
    }

    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}


void renderCofreContrasena(GLFWwindow* window)
{
    if (!simbols_loaded)
    {
        for (int i = 0; i < numSymbols; i++)
        {
            std::string path = "../EntornVGI/textures/minijocs/simbol_" + std::to_string(i) + ".png";
            simbol_textures[i] = loadTextureReturnID(path.c_str());
        }
        simbols_loaded = true;
    }

    int display_w = 0, display_h = 0;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w, display_h);
    ImVec2 hudPos(0, 0);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));

    ImGui::Begin("Cofre", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);

    // --- Dibuixar el marc centrat ---
    ImVec2 marcSize(display_w * 0.6f, display_h * 0.6f); // 60% de la pantalla
    ImVec2 marcPos((display_w - marcSize.x) * 0.5f, (display_h - marcSize.y) * 0.5f);

    ImGui::GetBackgroundDrawList()->AddImage(
        (ImTextureID)(intptr_t)texMarcSimbols,
        marcPos,
        ImVec2(marcPos.x + marcSize.x, marcPos.y + marcSize.y));

    // --- Mida de cada símbol ---
    ImVec2 simbolSize(100, 128);
    float spacing = 10.0f;

    float totalWidth = 4 * simbolSize.x + 3 * spacing;
    float startX = marcPos.x + (marcSize.x - totalWidth) * 0.5f;
    float startY = marcPos.y + (marcSize.y - simbolSize.y) * 0.4f;

    ImGui::SetCursorPos(ImVec2(startX, startY));

    for (int i = 0; i < 4; i++)
    {
        ImGui::BeginGroup();

        if (ImGui::ArrowButton(("##up" + std::to_string(i)).c_str(), ImGuiDir_Up))
            ranures[i] = (ranures[i] + 1) % numSymbols;

        int idx = ranures[i];
        ImGui::Image((void*)(intptr_t)simbol_textures[idx], simbolSize);

        if (ImGui::ArrowButton(("##down" + std::to_string(i)).c_str(), ImGuiDir_Down))
            ranures[i] = (ranures[i] - 1 + numSymbols) % numSymbols;

        ImGui::EndGroup();

        if (i < 3) ImGui::SameLine(0.0f, spacing);
    }

    // --- Popup de contrasenya ---
    if (ranures == solucio)
    {
        PlaySoundOnce(ID_CHEST);
        PlaySoundOnce(ID_ITEM);
        cofre_on = false;
        joc_quadres_finalitzat = true;
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
}



// ---------- Cronometre ---------------
float g_DuradaCronometre = 15.0f * 60.0f;
float g_TempsTranscorregut = 0.0f;

void renderCronometre(GLFWwindow* window)
{
    float tempsRestant = glm::max(0.0f, g_DuradaCronometre - g_TempsTranscorregut);

    int display_w = 0, display_h = 0;
    glfwGetWindowSize(window, &display_w, &display_h);

    int minuts = static_cast<int>(tempsRestant) / 60;
    int segons = static_cast<int>(tempsRestant) % 60;

    // La finestra ocupa tota la pantalla
    ImVec2 cronoPos(0, 0);
    ImVec2 cronoSize(display_w, display_h);

    ImGui::SetNextWindowPos(cronoPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(cronoSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    if (ImGui::Begin("Cronometre", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings))
    {
        ImVec2 imageSize(display_w / 7.0f, display_h / 4.2f); // mida de la imatge
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::Image((void*)(intptr_t)texCrono, imageSize);

        ImVec2 textSize = ImGui::CalcTextSize("00:00");
        float textX = (imageSize.x - textSize.x) * 0.5f;
        float textY = (imageSize.y - textSize.y) * 0.65f;
        ImGui::SetCursorPos(ImVec2(textX, textY));

        float fontScale = display_h / 500.0f;
        ImGui::SetWindowFontScale(fontScale);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        ImGui::Text("%02d:%02d", minuts, segons);
        ImGui::PopStyleColor();
    }
    ImGui::End();

    if (g_TempsTranscorregut >= g_DuradaCronometre)
        act_state = GameState::END;
}

void renderFinalJoc(GLFWwindow* window)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        UpdateGamepad(dtPad);
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
            UpdatePadMouseForImGui(window);
    }


    int display_h = 0;
    SetupHUD(window, display_h);

    draw_image(texEnd, ImGui::GetIO().DisplaySize.x, display_h);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f); // transparent

    ImGui::Begin("End", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar);

    ImVec2 actualSize = ImGui::GetWindowSize();
    ImVec2 buttonSize(actualSize.x / 4.0f, actualSize.y / 10.0f);

    // Escalat de font més gran
    float fontScale = display_h / 250.0f;
    ImGui::SetWindowFontScale(fontScale);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));

    // --- Centrat vertical i horitzontal ---
    ImVec2 textSize = ImGui::CalcTextSize("Has Perdut!");
    float totalHeight = textSize.y + 20.0f + buttonSize.y; // text + espai + botó
    float startY = (actualSize.y - totalHeight) * 0.5f;

    // Text centrat
    ImGui::SetCursorPos(ImVec2((actualSize.x - textSize.x) * 0.5f, startY));
    ImGui::Text("Has Perdut!");

    // Botó centrat sota el text
    ImGui::SetCursorPos(ImVec2((actualSize.x - buttonSize.x) * 0.5f, startY + textSize.y + 20.0f));
    if (ButtonWithSound("Sortir", buttonSize))
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    ImGui::End();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}




int idx_clue = 1;
// -------------------- Render Pistas --------------------
void renderPistas(GLFWwindow* window)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGuiIO& io = ImGui::GetIO();
        float dtPad = io.DeltaTime;
        if (dtPad <= 0.0f) dtPad = 1.0f / 60.0f;

        UpdateGamepad(dtPad);
        g_UsePadMouse = g_Pad.connected;

        if (g_UsePadMouse)
            UpdatePadMouseForImGui(window);
    }
    int display_w = 0, display_h = 0;
    glfwGetWindowSize(window, &display_w, &display_h);

    ImVec2 hudSize(display_w, display_h);
    ImVec2 hudPos(0, 0);

    ImGui::SetNextWindowPos(hudPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(hudSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    draw_image(texPistas, ImGui::GetIO().DisplaySize.x, display_h);

    ImGui::Begin("Pistes", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoTitleBar);

    ImVec2 actualSize = ImGui::GetWindowSize();
    ImVec2 buttonSize(actualSize.x / 4.0f, actualSize.y / 8.0f);

    float rightHalfX = display_w * 0.5f;
    float rightHalfWidth = display_w * 0.5f;

    float centerX = (rightHalfX + (rightHalfWidth - buttonSize.x) * 0.5f) - 120.0f;

    float offsetY = display_h * 0.2f;
    ImGui::SetCursorPosY(offsetY);

    float fontScale = display_h / 350.0f;
    ImGui::SetWindowFontScale(fontScale);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f); // radio de redondeo en píxeles
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));


    ImGui::SetCursorPosX(centerX);
    ImGui::Text("Pista %d", idx_clue);
    ImGui::Spacing();

    if (idx_clue == 1)
    {
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("La habitacio esta tancada amb clau.");
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("Hi ha un cofre amb contrasenya,");
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("potser si trobo la contrasenya amb");
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("podre sortir d'aqui.");

    }
    else if (idx_clue == 2)
    {
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("Una clau rovellada.");
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("Sembla que coincideix amb una de les");
        ImGui::SetCursorPosX(centerX);
        ImGui::Text("portes.");
    }

    offsetY = display_h * 0.6f;
    ImGui::SetCursorPosY(offsetY);
    ImGui::SetCursorPosX(centerX);
    if (ButtonWithSound("Enrere", buttonSize)) act_state = GameState::MENU;

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::End();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}
