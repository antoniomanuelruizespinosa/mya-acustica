#pragma once
#include <array>
#include <functional>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
namespace MyaCommands {
enum class Language { ES, EN };
enum class Capability { Ready, Demo, Pending };
struct Text {std::string es,en;const std::string& get(Language l) const{return l==Language::ES?es:en;}};
struct Command {int id;std::string key;Text name;std::string shortcut;int menu;Text path;Capability capability;Text reason;};
inline const std::vector<Command>& catalog(){
 static const std::vector<Command> commands{
  {101,"project.open",{"Abrir demo","Open demo"},"",0,{"",""},Capability::Ready,{"",""}},
  {102,"project.save",{"Guardar demo","Save demo"},"",0,{"",""},Capability::Ready,{"",""}},
  {103,"trace.capture",{"Capturar TF","Capture TF"},"Space",0,{"",""},Capability::Demo,{"Guarda datos sintéticos; no una medida","Saves synthetic data; not a measurement"}},
  {190,"data.import",{"Importar medidas / IR / carpetas","Import measurements / IR / folders"},"",0,{"",""},Capability::Pending,{"Importador y procedencia pendientes","Importer and provenance pending"}},
  {192,"report.export",{"Exportar informe","Export report"},"",0,{"",""},Capability::Pending,{"Generación de informes pendiente","Report generation pending"}},
  {201,"config.audio",{"Audio / E-S","Audio / I-O"},"",1,{"",""},Capability::Ready,{"Dispositivos y rutas de entrada; Play mide una TF seleccionada, Stop/Pause cierra","Input devices and routes; Play measures one selected TF, Stop/Pause closes"}},
  {202,"config.tf",{"Ajustes TF seleccionada","Selected TF settings"},"",1,{"",""},Capability::Demo,{"Edita parámetros de la TF demo; no inicia audio","Edits demo TF parameters; does not start audio"}},
  {290,"calibration.spl",{"Calibración SPL / micrófono","SPL / microphone calibration"},"",1,{"",""},Capability::Ready,{"Calibracion RMS por backend/dispositivo/canal","RMS calibration per backend/device/channel"}},
  {292,"calibration.loop",{"Calibración de loop","Loop calibration"},"",1,{"",""},Capability::Ready,{"Abre los controles del loop físico; el audio solo arranca al activarlo","Opens physical loop controls; audio starts only when enabled"}},
  {293,"config.generator",{"Ajustes de generador","Generator settings"},"",1,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {291,"config.averaging",{"Promedios en directo","Live averaging"},"",1,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {301,"plot.smoothing.link",{"Vincular suavizado de fase y magnitud","Link phase and magnitude smoothing"},"",2,{"Ajustes de gráficos/Función de transferencia","Graph settings/Transfer function"},Capability::Demo,{}},
  {310,"plot.smoothing.phase.0",{"Sin suavizado","No smoothing"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {311,"plot.smoothing.phase.1",{"1/48 oct","1/48 oct"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {312,"plot.smoothing.phase.2",{"1/24 oct","1/24 oct"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {313,"plot.smoothing.phase.3",{"1/12 oct","1/12 oct"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {314,"plot.smoothing.phase.4",{"1/6 oct","1/6 oct"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {315,"plot.smoothing.phase.5",{"1/3 oct","1/3 oct"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {316,"plot.smoothing.phase.6",{"1 octava","1 octave"},"",2,{"Suavizado / fase","Smoothing / phase"},Capability::Demo,{"",""}},
  {320,"plot.smoothing.magnitude.0",{"Sin suavizado","No smoothing"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {321,"plot.smoothing.magnitude.1",{"1/48 oct","1/48 oct"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {322,"plot.smoothing.magnitude.2",{"1/24 oct","1/24 oct"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {323,"plot.smoothing.magnitude.3",{"1/12 oct","1/12 oct"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {324,"plot.smoothing.magnitude.4",{"1/6 oct","1/6 oct"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {325,"plot.smoothing.magnitude.5",{"1/3 oct","1/3 oct"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {326,"plot.smoothing.magnitude.6",{"1 octava","1 octave"},"",2,{"Suavizado / magnitud","Smoothing / magnitude"},Capability::Demo,{"",""}},
  {380,"language.es",{"Español","Spanish"},"",2,{"Idioma de menús y comandos","Menu and command language"},Capability::Ready,{"",""}},
  {381,"language.en",{"Inglés","English"},"",2,{"Idioma de menús y comandos","Menu and command language"},Capability::Ready,{"",""}},
  {391,"plot.fft",{"Ventanas FFT / FTW","FFT windows / FTW"},"",2,{"",""},Capability::Pending,{"Motor y datos complejos compatibles pendientes","Engine and compatible complex data pending"}},
  {392,"plot.cursor.lock",{"Cursor bloqueado / deltas","Locked cursor / deltas"},"",2,{"",""},Capability::Pending,{"Interacción y deltas pendientes","Interaction and deltas pending"}},
  {393,"plot.theme",{"Tema / grosor de trazos","Theme / trace width"},"",2,{"",""},Capability::Pending,{"Preferencias de apariencia pendientes","Appearance preferences pending"}},
  {401,"view.measurement",{"Medición","Measurement"},"",3,{"",""},Capability::Demo,{"Espacio de demostración; sin motor validado","Demo workspace; no validated engine"}},
  {402,"view.dsp",{"DSP / Ajuste","DSP / Adjust"},"",3,{"",""},Capability::Demo,{"Espacio de demostración; sin motor validado","Demo workspace; no validated engine"}},
  {403,"view.design",{"Mya Mapp Vision 3D","Mya Mapp Vision 3D"},"",3,{"",""},Capability::Demo,{"Espacio de demostración; sin motor validado","Demo workspace; no validated engine"}},
  {404,"view.lab",{"Laboratorio","Laboratory"},"",3,{"",""},Capability::Demo,{"Espacio de demostración; sin motor validado","Demo workspace; no validated engine"}},
  {405,"view.left",{"Barra izquierda","Left bar"},"B",3,{"",""},Capability::Ready,{"",""}},
  {406,"view.right",{"Barra derecha","Right bar"},"O",3,{"",""},Capability::Ready,{"",""}},
  {407,"view.ir",{"Impulso superior en TF","Top impulse in TF"},"",3,{"",""},Capability::Demo,{"",""}},
  {408,"view.coherence",{"Coherencia","Coherence"},"C",3,{"",""},Capability::Ready,{"TF seleccionada: real; otras curvas: demo","Selected TF: live; other curves: demo"}},
  {409,"view.reset",{"Restaurar vista","Restore view"},"Option/Alt+5",3,{"",""},Capability::Ready,{"",""}},
  {980,"measurement.view.tf",{"Vista TF","TF view"},"T",3,{"",""},Capability::Ready,{"Una TF seleccionada puede mostrar magnitud, fase y coherencia en directo; impulso demo","One selected TF can show live magnitude, phase and coherence; demo impulse"}},
  {981,"measurement.view.ir",{"Vista IR demo","Demo IR view"},"",3,{"",""},Capability::Demo,{"Vista propia de impulso sintetico; adquisicion IR pendiente","Standalone synthetic impulse; measured IR pending"}},
  {420,"view.rta",{"Espectro RTA","RTA spectrum"},"",3,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {421,"view.spectrogram",{"Espectrograma","Spectrogram"},"",3,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {422,"view.rt60",{"RT60 por bandas","RT60 by band"},"",3,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {502,"measurement.find",{"Buscar retardo TF","Find TF delay"},"L / F",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {503,"measurement.track.all",{"Track general / todas las TF","Track all project TFs"},"",4,{"",""},Capability::Pending,{"Tiempo de todas las TF del proyecto; motor pendiente, sin incluir AVG","Timing for every project TF; engine pending, excludes AVG"}},
  {504,"generator.pink",{"Ruido rosa","Pink noise"},"G",4,{"",""},Capability::Ready,{"Emite por la salida configurada al nivel ajustado; con Int.Loop usa la misma señal como referencia virtual","Outputs on the configured channel at the selected level; with Int.Loop the same signal is the virtual reference"}},
  {505,"generator.sweep",{"Sweep","Sweep"},"S",4,{"",""},Capability::Ready,{"Emite un barrido logarítmico por la salida configurada","Outputs a logarithmic sweep on the configured channel"}},
  {526,"measurement.averager.reset",{"Refrescar TF en directo","Refresh live TF"},"V",4,{"",""},Capability::Ready,{"Vacía muestras pendientes y reinicia el promedio M/R; conserva capturas guardadas","Discards pending samples and resets M/R averaging; keeps saved captures"}},
  {506,"start",{"Iniciar todos","Start all"},"",4,{"",""},Capability::Ready,{"Abre entradas y mide una TF seleccionada","Opens inputs and measures one selected TF"}},
  {507,"stop",{"Detener todos","Stop all"},"",4,{"",""},Capability::Ready,{"Cierra entradas y borra TF en directo","Closes inputs and clears live TF"}},
  {508,"pause",{"Pausar todos","Pause all"},"",4,{"",""},Capability::Ready,{"Cierra entradas y borra TF en directo","Closes inputs and clears live TF"}},
  {509,"tf.create",{"Crear TF","Create TF"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {510,"average.create",{"Crear promedio","Create average"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {511,"timer",{"Temporizador","Timer"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {512,"internal.loop",{"Loop interno","Internal loop"},"",4,{"",""},Capability::Ready,{"Usa el generador rosa como referencia virtual al iniciar Pink; no emite audio al activar este botón","Uses pink generator as virtual reference when Pink starts; this button alone emits no audio"}},
  {513,"gain.track",{"Seguimiento de ganancia","Gain tracking"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {514,"track.tf",{"Track individual","Track individual TF"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {515,"start.tf",{"Iniciar TF","Start TF"},"",4,{"",""},Capability::Ready,{"Mide magnitud, fase y coherencia M/R de esta TF","Measures this TF's live M/R magnitude, phase and coherence"}},
  {516,"find.advanced",{"Buscador avanzado","Advanced delay finder"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {517,"pink.settings",{"Opciones de ruido rosa","Pink noise options"},"",4,{"",""},Capability::Ready,{"Panel sin generacion; parametros sin motor deshabilitados","Panel only; engine parameters disabled"}},
  {518,"sweep.settings",{"Opciones de sweep","Sweep options"},"",4,{"",""},Capability::Ready,{"Panel sin generacion; parametros sin motor deshabilitados","Panel only; engine parameters disabled"}},
  {519,"phase.results",{"Resultados de fase automática","Auto phase results"},"",4,{"",""},Capability::Pending,{"Motor de audio pendiente","Audio engine pending"}},
  {520,"measurement.delay.reset",{"Restaurar delay manual TF","Reset manual TF delay"},"",4,{"",""},Capability::Demo,{"",""}},
  {521,"measurement.polarity",{"Polaridad de medición","Measurement polarity"},"",4,{"",""},Capability::Pending,{"Inversión de adquisición pendiente; distinta de DSP","Acquisition inversion pending; separate from DSP"}},
  {203,"window.meters",{"Medidores","Meters"},"E, E",5,{"",""},Capability::Demo,{"Medidores independientes por vista; Play abre entradas dBFS, SPL pendiente","Independent meters per view; Play opens dBFS inputs, SPL pending"}},
  {602,"window.drawer.close",{"Cerrar panel de ajustes","Close settings drawer"},"Esc",5,{"",""},Capability::Ready,{"",""}},
  {701,"help.shortcuts",{"Atajos y manejo","Shortcuts and operation"},"",6,{"",""},Capability::Ready,{"",""}},
  {702,"help.status",{"Estado del prototipo","Prototype status"},"",6,{"",""},Capability::Ready,{"",""}},
  {703,"help.about",{"Acerca de Mya Acústica","About Mya Acústica"},"",6,{"",""},Capability::Ready,{"",""}},
  {790,"help.update",{"Buscar actualizaciones","Check for updates"},"",6,{"",""},Capability::Ready,{"Consulta la última versión publicada en GitHub","Check the latest published GitHub release"}},
  {801,"dsp.polarity",{"Polaridad DSP","DSP polarity"},"",3,{"DSP","DSP"},Capability::Demo,{"",""}},
  {802,"dsp.bypass",{"Bypass visual DSP","Visual DSP bypass"},"",3,{"DSP","DSP"},Capability::Demo,{"Solo comparación de la respuesta sintética","Synthetic response comparison only"}},
  {810,"dsp.auto.phase",{"Proponer alineación de fase","Propose phase alignment"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {811,"dsp.auto.xover",{"Proponer cruce","Propose crossover"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {812,"dsp.auto.eq",{"Auto EQ","Auto EQ"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {813,"dsp.auto.fir",{"Auto FIR","Auto FIR"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {814,"dsp.target",{"Targets","Targets"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {815,"dsp.filters",{"Filtros / tabla DSP","Filters / DSP table"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {816,"dsp.variant",{"Crear variante / deshacer","Create variant / undo"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {817,"dsp.design.bridge",{"Diseño → DSP","Design → DSP"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {818,"dsp.dsp.bridge",{"DSP → Diseño","DSP → Design"},"",3,{"DSP","DSP"},Capability::Pending,{"Modelo, cálculo o historial DSP pendiente","DSP model, computation or history pending"}},
  {830,"processor.profiles",{"Modelo / firmware y capacidades","Model / firmware and capabilities"},"",1,{"Procesadores","Processors"},Capability::Pending,{"Perfiles y compatibilidad sin validar; Prodigy / Galaxy / Marani","Profiles and compatibility unvalidated; Prodigy / Galaxy / Marani"}},
  {831,"processor.api",{"Conexión API por modelo","API connection by model"},"",1,{"Procesadores","Processors"},Capability::Pending,{"Perfiles y compatibilidad sin validar; Prodigy / Galaxy / Marani","Profiles and compatibility unvalidated; Prodigy / Galaxy / Marani"}},
  {832,"processor.export",{"Exportar archivo por modelo","Export file by model"},"",1,{"Procesadores","Processors"},Capability::Pending,{"Perfiles y compatibilidad sin validar; Prodigy / Galaxy / Marani","Profiles and compatibility unvalidated; Prodigy / Galaxy / Marani"}},
  {833,"processor.manual",{"Ficha de introducción manual","Manual entry sheet"},"",1,{"Procesadores","Processors"},Capability::Pending,{"Perfiles y compatibilidad sin validar; Prodigy / Galaxy / Marani","Profiles and compatibility unvalidated; Prodigy / Galaxy / Marani"}},
  {840,"remote.lan",{"Control remoto LAN / API","LAN / API remote control"},"",1,{"",""},Capability::Pending,{"Servidor no implementado; sin servicio activo","Server not implemented; no active service"}},
  {850,"plot.zoom.in.y",{"Ampliar Y","Zoom Y in"},"+",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {851,"plot.zoom.out.y",{"Reducir Y","Zoom Y out"},"-",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {852,"plot.zoom.in.xy",{"Ampliar XY","Zoom XY in"},"Cmd/Ctrl + +",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {853,"plot.zoom.out.xy",{"Reducir XY","Zoom XY out"},"Cmd/Ctrl + -",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {854,"plot.zoom.in.x",{"Ampliar X","Zoom X in"},"Cmd/Ctrl + Option/Alt + +",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {855,"plot.zoom.out.x",{"Reducir X","Zoom X out"},"Cmd/Ctrl + Option/Alt + -",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {856,"plot.zoom.arrow.in",{"Reducir intervalo X","Narrow X interval"},"Left",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {857,"plot.zoom.arrow.out",{"Ampliar intervalo X","Widen X interval"},"Right",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {858,"plot.pan.up",{"Desplazar arriba","Pan up"},"Up",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {859,"plot.pan.down",{"Desplazar abajo","Pan down"},"Down",4,{"Gráfica activa","Active plot"},Capability::Demo,{"",""}},
  {900,"library.session.new",{"Nueva sesión","New session"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {901,"library.folder.new",{"Nueva subcarpeta","New subfolder"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {902,"library.rename",{"Renombrar selección","Rename selection"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {903,"library.destination",{"Fijar destino de captura","Pin capture destination"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {904,"library.move",{"Mover selección a…","Move selection to…"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {905,"library.show",{"Mostrar capturas","Show captures"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {906,"library.hide",{"Ocultar capturas","Hide captures"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {907,"library.info",{"Información de captura / carpeta","Capture / folder information"},"Enter (library)",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {908,"library.colour",{"Color de capturas","Capture colour"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {909,"library.delete",{"Eliminar selección","Delete selection"},"",0,{"Biblioteca","Library"},Capability::Pending,{"Borrado recuperable pendiente","Recoverable deletion pending"}},
  {910,"library.load.dsp",{"Cargar en canal DSP…","Load into DSP channel…"},"",0,{"Biblioteca","Library"},Capability::Pending,{"Motor y selector de canales DSP pendientes; original inmutable","DSP engine and channel selector pending; immutable original"}},
  {911,"library.reload",{"Recargar biblioteca","Reload library"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},

  {930,"plot.type.ir.lin",{"Lin","Lin"},"",2,{"Escala de amplitud IR","IR amplitude scale"},Capability::Demo,{}},
  {931,"plot.type.ir.log",{"Log","Log"},"",2,{"Escala de amplitud IR","IR amplitude scale"},Capability::Demo,{}},
  {932,"plot.type.ir.etc",{"ETC","ETC"},"",2,{"Escala de amplitud IR","IR amplitude scale"},Capability::Demo,{}},
  {940,"plot.type.panel1.mag",{"Magnitud","Magnitude"},"",2,{"Gráfico 1","Plot 1"},Capability::Demo,{}},
  {941,"plot.type.panel1.phase",{"Fase","Phase"},"",2,{"Gráfico 1","Plot 1"},Capability::Demo,{}},
  {950,"plot.type.panel2.mag",{"Magnitud","Magnitude"},"",2,{"Gráfico 2","Plot 2"},Capability::Demo,{}},
  {951,"plot.type.panel2.phase",{"Fase","Phase"},"",2,{"Gráfico 2","Plot 2"},Capability::Demo,{}},
  {302,"plot.smoothing.global",{"Vincular suavizado global del mismo tipo","Link global smoothing of the same type"},"",2,{"Ajustes de gráficos/Función de transferencia","Graph settings/Transfer function"},Capability::Pending,{"Pendiente: enlace entre ventanas del mismo tipo","Pending: same-type link across windows"}},
  {303,"plot.settings.tf",{"Función de transferencia…","Transfer function…"},"Option/Alt+T",2,{"Ajustes de gráficos","Graph settings"},Capability::Ready,{}},
  {963,"plot.settings.ir",{"Respuesta al impulso…","Impulse response…"},"",2,{"Ajustes de gráficos","Graph settings"},Capability::Ready,{}},
  {964,"plot.settings.spectrum",{"Espectro…","Spectrum…"},"",2,{"Ajustes de gráficos","Graph settings"},Capability::Ready,{}},
  {960,"sidebar.traces",{"Trazos","Traces"},"",3,{"Barra izquierda","Left sidebar"},Capability::Ready,{}},
  {961,"sidebar.meters",{"dB Meter","dB Meter"},"",3,{"Barra izquierda","Left sidebar"},Capability::Demo,{}},
  {912,"library.view.rta",{"RTA (biblioteca contextual)","RTA (contextual library)"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {913,"library.view.traces",{"Trazos: individuales y AVG","Traces: individual and AVG"},"",0,{"Biblioteca","Library"},Capability::Ready,{}},
  {971,"meter.main.visible",{"Medidor principal","Main meter"},"Option/Alt+K",3,{"",""},Capability::Ready,{}},
  {972,"meter.main.clock",{"Medidor / reloj","Meter / clock"},"K",3,{"",""},Capability::Ready,{}},
  {973,"meter.main.config",{"SPL Config / opciones de medidor","SPL Config / meter options"},"",1,{"",""},Capability::Ready,{"Configuracion del principal; estimadores pendientes","Principal configuration; estimators pending"}},
  {974,"meter.max.reset",{"Reset Max","Reset Max"},"",4,{"",""},Capability::Ready,{}},
  {975,"meter.leq.reset",{"Reset Leq","Reset Leq"},"",4,{"",""},Capability::Ready,{}},
  {969,"average.start",{"Iniciar AVG","Start AVG"},"",4,{"",""},Capability::Pending,{"Estimador AVG pendiente","AVG estimator pending"}},
  {970,"loop.settings",{"Opciones de loop interno","Internal loop options"},"",1,{"",""},Capability::Ready,{"Panel sin audio; calibracion y parametros de motor deshabilitados","Panel only; calibration and engine parameters disabled"}},
  {965,"dsp.auto.delay",{"Auto Delay","Auto Delay"},"",3,{"DSP","DSP"},Capability::Pending,{"Estimador temporal pendiente","Time estimator pending"}},
  {966,"dsp.compare",{"Comparar","Compare"},"",3,{"DSP","DSP"},Capability::Pending,{"Variantes y comparación verificable pendientes","Verifiable comparison and variants pending"}},
  {967,"dsp.proposals",{"Propuestas","Proposals"},"",3,{"DSP","DSP"},Capability::Pending,{"Propuestas verificables y decisión del ingeniero pendientes","Verifiable proposals and engineer decision pending"}},
  {968,"plot.sum",{"Suma","Sum"},"",2,{"",""},Capability::Pending,{"Suma compleja y referencias temporales compatibles pendientes; AVG no es suma","Complex sum and compatible time references pending; AVG is not a sum"}},
  {914,"library.view.ir",{"Sweep / IR","Sweep / IR"},"",0,{"Biblioteca","Library"},Capability::Ready,{}}

 };return commands;
}
// Compact captions preserve the existing narrow controls; full names/reasons are in tooltips.
inline std::string compactName(const Command& c,Language language){
 switch(c.id){
 case 703:return language==Language::ES?"Ayuda":"Help";
 case 815:return language==Language::ES?"Filtros":"Filters";
 case 832:return language==Language::ES?"Exportar DSP":"Export DSP";
 case 817:return language==Language::ES?"Diseño → DSP":"Design → DSP";
 case 818:return "DSP → 3D";
 case 502:return "Find L/F";case 503:return "Track";case 514:return "Track";case 513:return "AG";
 case 511:case 516:case 517:case 518:case 519:return "...";
 case 504:return "Pink [G]";case 505:return "Sweep [S]";
 case 509:return "+ TF";case 510:return "+ AVG";case 401:return "TF";
 case 407:return language==Language::ES?"IR demo":"Demo IR";
 case 814:return "Target";case 810:return language==Language::ES?"Auto Fase":"Auto Phase";case 521:return "Ø";
 case 103:return language==Language::ES?"Captura":"Capture";case 408:return "Coh";case 960:return language==Language::ES?"Trazos":"Traces";case 961:return "dB Meter";
 case 293:return language==Language::ES?"Ajustes":"Settings";
 default:return c.name.get(language);
 }
}
inline const Command* find(int id){for(const auto& c:catalog())if(c.id==id)return &c;return nullptr;}
inline const std::array<Text,7>& menus(){static const std::array<Text,7> names{{{"Archivo","File"},{"Configuración","Configuration"},{"Opciones","Options"},{"Vista","View"},{"Comandos","Commands"},{"Ventanas","Windows"},{"Ayuda","Help"}}};return names;}
inline constexpr std::array<int,19> topBindings{{401,402,403,404,102,101,201,703,812,813,814,815,801,802,818,817,290,502,832}};
inline constexpr std::array<int,30> measurementBindings{{420,401,203,422,512,511,511,502,516,504,517,505,518,509,510,810,519,520,521,503,980,981,203,506,507,508,201,407,293,103}};
inline constexpr std::array<int,4> tfBindings{{502,514,513,515}};
// Secondary menu placements: command IDs are reused, never copied or aliased.
inline constexpr std::array<std::array<int,2>,3> extraMenus{{{{4,103}},{{1,203}},{{5,409}}}};
struct Context {
 bool mainMeterVisible=true,meterClock=false,meterMax=false,meterLeq=false;bool libraryReady=true,libraryTF=true;int librarySelection=0,libraryFolders=0,libraryCaptures=0,libraryView=1;bool sidebarMeters=false,metersVisible=false,allowNoSmoothing=true;
 int mode=0,selectedTF=1,measurementView=0;bool drawer=false,left=true,right=true,ir=true,coherence=true,linked=true,polarity=false,bypass=false,internalLoop=false,pinkActive=false,sweepActive=false;
 std::array<int,2> graphTypes{{1,2}};int irScale=0;
 std::array<int,2> smoothing{{1,1}};Language language=Language::ES;
};
struct State {bool enabled=true,checked=false;Text reason;};
inline State state(const Command& c,const Context& x){
 State s{c.capability!=Capability::Pending,false,c.reason};
 if(c.capability==Capability::Pending)return s;
 if((c.id>=900&&c.id<=908)&&!x.libraryReady){s.enabled=false;s.reason={"Biblioteca no disponible; revisa el error","Library unavailable; check the error"};}
 if((c.id==292||c.id==517||c.id==518||c.id==970)&&x.mode!=0){s.enabled=false;s.reason={"Disponible en Medicion","Available in Measurement"};}
 if(c.id==971)s.checked=x.mainMeterVisible;
 if(c.id==972){s.checked=x.meterClock;s.enabled=x.mode==0&&x.mainMeterVisible&&x.right;if(!s.enabled)s.reason={"Mostrar principal en Medicion","Show main meter in Measurement"};}
 if(c.id==974){s.enabled=x.meterMax;s.reason={"Sin maximo valido; no borra Leq","No valid maximum; does not reset Leq"};}
 if(c.id==975){s.enabled=x.meterLeq;s.reason={"Sin buffer Leq; no borra Max","No Leq buffer; does not reset Max"};}
 if(c.id==980||c.id==981){s.enabled=c.id==980||x.mode==0;s.checked=x.mode==0&&x.measurementView==(c.id==981?1:0);}
 if(c.id==407&&x.measurementView!=0)s.enabled=false;
 if(c.id==512){s.enabled=x.mode==0;s.checked=x.internalLoop;if(!s.enabled)s.reason={"Disponible en Medición","Available in Measurement"};}
 if(c.id==504){s.enabled=x.mode==0;s.checked=x.pinkActive;if(!s.enabled)s.reason={"Disponible en Medición","Available in Measurement"};}
 if(c.id==505){s.enabled=x.mode==0;s.checked=x.sweepActive;if(!s.enabled)s.reason={"Disponible en Medición","Available in Measurement"};}
 if(c.id==526&&x.mode!=0){s.enabled=false;s.reason={"Disponible en Medición","Available in Measurement"};}
 if(c.id==203)s.checked=x.metersVisible;
 if(c.id==960)s.checked=!x.sidebarMeters;if(c.id==961)s.checked=x.sidebarMeters;
 if(c.id>=912&&c.id<=914){s.enabled=x.libraryReady;s.checked=x.libraryView==(c.id==912?3:c.id==913?1:2);}
 if((c.id==310||c.id==320)&&!x.allowNoSmoothing){s.enabled=false;s.reason={"Sin suavizado desactivado en ajustes TF","No smoothing disabled in TF settings"};}
 if(c.id==103&&(!x.libraryReady||!x.libraryTF)){s.enabled=false;s.reason={"Captura demo disponible en Capturas TF; IR/RTA pendientes","Demo capture available in TF snapshots; IR/RTA pending"};}
 if((c.id==902||c.id==907)&&x.librarySelection!=1){s.enabled=false;s.reason={"Selecciona un elemento","Select one item"};}
 if(c.id==903&&(x.librarySelection!=1||x.libraryFolders!=1)){s.enabled=false;s.reason={"Selecciona una carpeta destino","Select a destination folder"};}
 if(c.id==904&&x.librarySelection<1){s.enabled=false;s.reason={"Selecciona capturas o carpetas","Select captures or folders"};}
 if((c.id==905||c.id==906||c.id==908)&&x.libraryCaptures<1){s.enabled=false;s.reason={"Selecciona capturas","Select captures"};}
 if((c.id==103||c.id==202||c.id==515||c.id==520)&&(x.selectedTF<1||x.selectedTF>4)){
  s.enabled=false;s.reason={"Selecciona una TF individual; AVG no implementado","Select an individual TF; AVG not implemented"};
 }
 if((c.id==103||c.id==515||c.id==520||(c.id>=310&&c.id<=326))&&x.mode!=0){
  s.enabled=false;s.reason={"Disponible en Medición","Available in Measurement"};
 }
 if(c.id>=850&&c.id<=859&&x.mode==2){s.enabled=false;s.reason={"Selecciona un gráfico","Select a plot"};}
 if(c.id>=930&&c.id<=951){s.enabled=c.id<=932||x.mode==0;if(!s.enabled)s.reason={"Disponible en Medición","Available in Measurement"};
  if(c.id<=932)s.checked=x.irScale==c.id-930;
  if(c.id==940||c.id==941)s.checked=x.graphTypes[0]==(c.id==940?2:1);
  if(c.id==950||c.id==951)s.checked=x.graphTypes[1]==(c.id==950?2:1);
 }
 if(c.id==602){s.enabled=x.drawer;if(!s.enabled)s.reason={"No hay panel abierto","No drawer is open"};}
 if(c.id==801||c.id==802){s.enabled=x.mode==1||x.mode==3;if(!s.enabled)s.reason={"Disponible en DSP / Laboratorio","Available in DSP / Laboratory"};}
 if(c.id>=401&&c.id<=404)s.checked=x.mode==c.id-401;
 switch(c.id){case 301:s.checked=x.linked;break;case 380:s.checked=x.language==Language::ES;break;case 381:s.checked=x.language==Language::EN;break;case 405:s.checked=x.left;break;case 406:s.checked=x.right;break;case 407:s.checked=x.ir;break;case 408:s.checked=x.coherence;break;case 801:s.checked=x.polarity;break;case 802:s.checked=x.bypass;break;default:break;}
 if(c.id>=310&&c.id<=316)s.checked=x.smoothing[0]==c.id-309;
 if(c.id>=320&&c.id<=326)s.checked=x.smoothing[1]==c.id-319;
 return s;
}
inline bool dispatch(int id,const Context& context,const std::function<void(int)>& action){
 auto* c=find(id);if(!c||!state(*c,context).enabled||!action)return false;action(id);return true;
}
// Shared keyboard policy. Widget focus is decided by the JUCE adapter.
inline int shortcut(char key,bool command,bool alt,bool shift,bool plotFocus,bool editing){
 if(editing)return 0;
 if(alt&&!command&&!shift&&(key=='k'||key=='K'))return 971;
 if(alt&&!command&&!shift&&key=='5')return 409;
 if(alt&&!command&&!shift&&(key=='t'||key=='T'))return 303;
 if(plotFocus&&(key=='+'||key=='-')){if(alt&&!command)return 0;return (command?(alt?854:852):850)+(key=='-'?1:0);}
 if(command||alt||shift)return 0;
 key=static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
 switch(key){case 'k':return 972;case 'b':return 405;case 'o':return 406;case 'c':return 408;case ' ':return 103;case 'l':case 'f':return 502;case 'g':return 504;case 's':return 505;case 't':return 980;case 'v':return 526;default:break;}
 return 0;
}
inline bool isPlainMeterKey(int code,bool command,bool control,bool alt,bool shift){return (code=='e'||code=='E')&&!command&&!control&&!alt&&!shift;}
class MeterSequence {
 double last=-1;
public:
 int press(double now,bool valid){if(!valid){last=-1;return 0;}if(last>=0&&now-last>=0&&now-last<650){last=-1;return 203;}last=now;return 0;}
 void reset(){last=-1;}
};
}
