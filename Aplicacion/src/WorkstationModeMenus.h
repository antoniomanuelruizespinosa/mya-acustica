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
            {"Acerca de...","About...",703,true},
            {"Importar","Import",0,true,{
                {"Archivo de traza","Trace File",16013,true},
                {"Archivo de proyecto","Project File"}}},
            {"Buscar actualizaciones...","Check For Updates...",790,true},
            {"Salir","Quit",16011,true}}},
        {"Configuración","Config",{
            {"Administrar configuraciones...","Manage Configurations...",200,true},
            {"Configuración de Audio I-O...","Audio I-O Configuration...",201,true},
            {"Medición y Micrófonos...","Measurement and Microphones...",202,true},
            {"Configuración SPL...","SPL Config...",290,true},
            {"Configuración de barra de comandos...","Command Bar Config..."},
            {"Nuevo espectro...","New Spectrum...",420,true},
            {"Nueva TF...","New TF...",509,true},
            {"Calibración de amplitud...","Amplitude Calibration...",290,true}}},
        {"Opciones","Options",{
            {"Preferencias...","Preferences...",0,true,{{"Configuración Ambiental (Temperatura / Humedad)","Environmental Setup (Temperature / Humidity)",16050,true}}},
            {"Ajustes de gráfica","Graph Settings",303,true},
            {"Ajustes de medición","Measurement Settings",202,true},
            {"Generador de señal...","Signal Generator...",293,true},
            {"Curvas objetivo...","Target Curves..."},
            {"Curvas de ponderación...","Weighting Curves..."},
            {"Curvas de corrección de micrófono...","Mic Correction Curves..."}}},
        {"Vista","View",{
            {"Modo en tiempo real","Real Time Mode",980,true},
            {"Modo respuesta al impulso","Impulse Response Mode",981,true},
            {"Modo SPL","SPL Mode"},
            {"Preajustes de vista","View Presets"},
            {"Ventana de cliente...","Client Window..."},
            {"Medidores de entrada","Input Meters",203,true},
            {"Medidores SPL","SPL Meters"},
            {"Mostrar IR en vivo","Toggle Live IR"},
            {"Barra de medidor Data/SPL","Data/SPL Meter Bar",971,true},
            {"Barra de control","Control Bar"},
            {"Barra de comandos","Command Bar"},
            {"Barra de pestañas","Tab Bar"},
            {"Medidor SPL","SPL Meter"},
            {"Generador compacto","Compact Signal Generator"}}},
        {"Comandos","Command",{
            {"Zoom","Zoom",0,true,{{"Ampliar","Zoom In",850,true},{"Reducir","Zoom Out",851,true}}},
            {"Cursor","Cursor"},
            {"Conmutar generador de señal","Toggle Signal Generator",504,true},
            {"Reiniciar promedio","Reset Averager",526,true},
            {"Conmutar seguimiento de retardo","Toggle Delay Tracking",503,true},
            {"Seguir todos","Track All",503,true},
            {"Buscar retardo...","Activate Delay Finder...",502,true},
            {"Trazas guardadas","Stored Data Traces"},
            {"Desplazar","Scroll",0,true,{{"Arriba","Up",858,true},{"Abajo","Down",859,true}}},
            {"Avanzar orden Z","Cycle Z Order Forward"},
            {"Retroceder orden Z","Cycle Z Order Reverse"},
            {"Traza frontal arriba","Front Trace Up"},
            {"Traza frontal abajo","Front Trace Down"},
            {"Limpiar offset dB","Clear dB Offset"},
            {"Limpiar todos los offsets dB","Clear All dB Offsets"},
            {"Capturar comparación rápida","Capture Quick Compare"}}}
    };
    return menus;
}

inline const std::vector<Menu>& dsp(){
    static const std::vector<Menu> menus{
        {"RiTA","RiTA",{
            {"Acerca de","About",16012,true},
            {"Buscar actualizaciones...","Check for Updates...",790,true},
            {"Preferencias","Preferences",0,true,{{"Configuración Ambiental (Temperatura / Humedad)","Environmental Setup (Temperature / Humidity)",16050,true}}},
            {"Desactivar licencia","License Deactivation"},
            {"Idioma","Language",0,true,{{"Español","Spanish",380,true},{"Inglés","English",381,true}}},
            {"Tema","Theme"},
            {"Salir","Quit",16011,true},
            {"Cerrar ventana","Close Window"}}},
        {"Archivo","File",{
            {"Abrir proyecto...","Open Project..."},
            {"Guardar proyecto","Save Project"},
            {"Guardar proyecto como...","Save Project As..."},
            {"Importar carpetas","Import Folders"},
            {"Exportar informe PDF","Export PDF Report"},
            {"Preferencias...","Preferences...",0,true,{{"Configuración Ambiental (Temperatura / Humedad)","Environmental Setup (Temperature / Humidity)",16050,true}}}}},
        {"Configuración","Config",{{"Matriz de ruteo DSP","DSP Routing Matrix"},{"Ajustes ambientales","Environmental Setup",16050,true},{"Preferencias del simulador","Simulator Preferences"}}},
        {"Dispositivos","Devices",{{"Configuración de dispositivo de audio...","Audio Device Setup..."}}},
        {"Medición","Measurement",{
            {"Generador","Generator"},
            {"Sincronizar todos","Sync All"},
            {"Exportar todos","Export All"},
            {"Capturar todos","Snap All"},
            {"Suma","Sum",16060,true},
            {"Promedio","AVG",16061,true}}},
        {"DSP","DSP",{
            {"Silenciar todos","Mute All"},{"Activar todos","Unmute All"},
            {"Omitir todos los EQ","Bypass All EQs"},
            {"Omitir todos los cruces","Bypass All Crossovers"},
            {"Limpiar todos los retardos","Clear All Delays"},
            {"Invertir todas las polaridades","Invert All Polarities"}}},
        {"Herramientas","Tools",{
            {"Alineación automática...","Auto Align..."},
            {"Diseñador de curva objetivo...","Target Curve Designer..."},
            {"Asistente de alineación de subs...","Sub Alignment Wizard..."}}},
        {"Ventana","Window",{
            {"Expandir/contraer gráfica superior","Expand/Collapse Upper Plot",16040,true},
            {"Expandir/contraer gráfica inferior","Expand/Collapse Lower Plot",16041,true},
            {"Expandir/contraer banco de memorias","Expand/Collapse Memory Bank",405,true},
            {"Expandir/contraer banco de motores","Expand/Collapse Engines Bank",406,true},
            {"Expandir/contraer DSP","Expand/Collapse DSP"}}}
    };
    return menus;
}
inline const std::vector<Menu>& forMode(int mode){return mode==1?dsp():analyzer();}
}
