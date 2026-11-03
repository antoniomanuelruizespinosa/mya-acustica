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
        {"Archivo","File",{{"About Mya Analyzer...","About Mya Analyzer...",703,true},
            {"Importar Traza","Import Trace",16013,true},{"Buscar Actualizaciones","Check for Updates",790,true},
            {"Salir","Quit",16011,true}}},
        {"Dispositivos","Devices",{{"Configuración I-O (Octa-Capture/Dante)","I-O Configuration (Octa-Capture/Dante)",201,true},
            {"Ajustes de Hardware por Defecto","Default Hardware Settings"}}},
        {"Medición","Measurement",{{"Calibración SPL","SPL Calibration",290,true},
            {"Curvas de Corrección de Micrófonos","Microphone Correction Curves"}}},
        {"Opciones","Options",{{"Preferencias Generales","General Preferences"},
            {"Generador de Señal (Pink/Sweep)","Signal Generator (Pink/Sweep)",0,true,
                {{"Pink","Pink",293,true},{"Sweep","Sweep",518,true}}},
            {"Umbral de Coherencia","Coherence Threshold"}}},
        {"Vista","View",{{"Ventana Única","Single Window",16020,true},
            {"Doble Ventana (RTA + Espectrograma)","Dual Window (RTA + Spectrogram)",16021,true},
            {"Selector de Grosor de Traza","Trace Thickness",0,true,
                {{"1 px","1 px",15001,true},{"2 px","2 px",15002,true},
                 {"3 px","3 px",15003,true},{"4 px","4 px",15004,true}}}}},
        {"Comandos","Commands",{{"Conmutar Ruido Rosa","Toggle Pink Noise",504,true},
            {"Reiniciar Promedios (Reset Avg)","Reset Averages",526,true},
            {"Capturar Traza (Snapshot)","Capture Trace (Snapshot)",103,true}}}
    };
    return menus;
}

inline const std::vector<Menu>& dsp(){
    static const std::vector<Menu> menus{
        {"Archivo","File",{{"About Mya DSP...","About Mya DSP...",16012,true},
            {"Cargar Proyecto DSP","Load DSP Project"},{"Guardar Simulación","Save Simulation"},
            {"Exportar Respuesta al Impulso (.txt/.wav)","Export Impulse Response (.txt/.wav)"}}},
        {"Memorias","Memories",{{"Banco de Impulsos Locales","Local Impulse Bank",16030,true},
            {"Limpiar Historial de Predicción","Clear Prediction History"},
            {"Comparar Trazas Estáticas","Compare Static Traces"}}},
        {"Procesamiento DSP","DSP Processing",{{"Copiar DSP entre Canales","Copy DSP Between Channels"},
            {"Enlazar Canales (Link L/R)","Link Channels (L/R)"},
            {"Bypass Global de Matriz","Global Matrix Bypass"}}},
        {"Herramientas","Tools",{{"Diseñador FIR Automático (Auto EQ)","Automatic FIR Designer (Auto EQ)"},
            {"Asistente de Alineación de Fase (Auto Align)","Phase Alignment Assistant (Auto Align)"}}},
        {"Vista Gráfica","Graph View",{{"Ver Parcela IR","Show IR Plot",16040,true},
            {"Ver Parcela Magnitud/Fase Logarítmica","Show Log Magnitude/Phase Plots",16041,true},
            {"Mostrar Nodos Interactivos (Graphic EQ On/Off)","Show Interactive Nodes (Graphic EQ On/Off)",16042,true}}}
    };
    return menus;
}
inline const std::vector<Menu>& forMode(int mode){return mode==1?dsp():analyzer();}
}
