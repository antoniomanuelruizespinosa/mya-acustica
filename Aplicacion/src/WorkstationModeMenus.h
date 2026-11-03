#pragma once
#include <string>
#include <vector>

namespace WorkstationModeMenus {
struct Item {
    std::string es,en;
    int id=0;
    bool available=false;
    std::vector<Item> children;
};
struct Menu { std::string es,en; std::vector<Item> items; };

inline const std::vector<Menu>& analyzer(){
    static const std::vector<Menu> menus{
        {"Archivo","File",{
            {"Nuevo Proyecto .mya","New .mya Project"},
            {"Cargar Configuración","Load Configuration",200,true},
            {"Guardar Configuración Como...","Save Configuration As...",200,true},
            {"Importar Traza","Import Trace",16013,true},
            {"Salir","Quit",16011,true}}},
        {"Configuración","Config",{
            {"Configuración de Audio I-O","Audio I-O Configuration",201,true},
            {"Medición y Micrófonos","Measurement and Microphones",290,true},
            {"Configuración Ambiental (Temperatura / Humedad)","Environmental Setup (Temperature / Humidity)",16050,true},
            {"Opciones de Visualización","Display Options",0,true,{
                {"Ventana Única","Single Window",16020,true},
                {"Doble Ventana (RTA + Espectrograma)","Dual Window (RTA + Spectrogram)",16021,true},
                {"Grosor de Traza","Trace Thickness",0,true,{
                    {"1 px","1 px",15001,true},{"2 px","2 px",15002,true},
                    {"3 px","3 px",15003,true},{"4 px","4 px",15004,true}}}}}}}
    };
    return menus;
}

inline const std::vector<Menu>& dsp(){
    static const std::vector<Menu> menus{
        {"Archivo","File",{
            {"Nuevo Proyecto DSP","New DSP Project"},
            {"Abrir Proyecto DSP...","Open DSP Project..."},
            {"Guardar Proyecto","Save Project"},
            {"Guardar Proyecto Como...","Save Project As..."},
            {"Importar Respuesta al Impulso","Import Impulse Response"}}},
        {"Configuración","Config",{
            {"Matriz de Ruteo DSP","DSP Routing Matrix"},
            {"Configuración Ambiental (Temperatura / Humedad)","Environmental Setup (Temperature / Humidity)",16050,true},
            {"Preferencias del Simulador","Simulator Preferences"}}}
    };
    return menus;
}
inline const std::vector<Menu>& forMode(int mode){return mode==1?dsp():analyzer();}
}
