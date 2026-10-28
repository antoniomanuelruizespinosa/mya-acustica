// Included inside Workstation: JUCE host actions for the independent command catalog.
CommandUI commands;
MyaCommands::MeterSequence meterSequence;
bool fineCommandStep=false;
MyaCommands::Context commandContext() const {
 MyaCommands::Context x;x.mode=mode;x.measurementView=measurementView;x.selectedTF=source.getSelectedId();
 x.drawer=settingsDrawer&&settingsDrawer->isVisible();x.left=leftVisible;x.right=rightVisible;
 x.ir=plotChoice.getSelectedId()==1;x.coherence=showCoherence.getToggleState();
 x.linked=linkSmooth.getToggleState();x.polarity=polarity;x.bypass=disabled;
 auto lib=libraryPanel.context();x.libraryReady=lib.libraryReady;x.libraryTF=lib.libraryTF;x.librarySelection=lib.librarySelection;x.libraryFolders=lib.libraryFolders;x.libraryCaptures=lib.libraryCaptures;x.libraryView=(int)captureLibrary.state().view;x.sidebarMeters=dockMeterState.state().sidebarMeters;x.metersVisible=meters&&meters->isVisible();x.allowNoSmoothing=tfSettings.allowNone;
 x.mainMeterVisible=meterState.state().mainVisible;x.meterClock=meterState.state().clock;if(auto* c=meterState.primaryCard()){x.meterMax=meterState.runtime(c->id).maximum.has_value();x.meterLeq=meterState.runtime(c->id).leqFill.has_value();}
 x.graphTypes=plotTypes;x.irScale=irScale;
 x.smoothing={{plotSmooth[0].getSelectedId(),plotSmooth[1].getSelectedId()}};return x;
}
void restoreView(){
 measurementView=0;
 irFraction=.14;phaseFraction=.5;leftWidth=WorkspaceViewState::leftMin;rightWidth=WorkspaceViewState::rightMin;plotPanX.fill(0.);plotPanY.fill(0.);
 plotZoomX.fill(1.);plotZoomY.fill(1.);activePlot=1;plotTypes={{1,2}};irScale=0;dockMeterState.sidebar(false);impulseScale.setSelectedId(1,dontSendNotification);for(size_t i=0;i<2;++i)plotTypeChoice[i].setSelectedId(plotTypes[i],dontSendNotification);
 plotChoice.setSelectedId(1,dontSendNotification);leftVisible=rightVisible=true;
 if(settingsDrawer)settingsDrawer->setVisible(false);
}
void executeCommand(int id){
 if(id>=930&&id<=932){irScale=id-930;impulseScale.setSelectedId(irScale+1,dontSendNotification);plotPanY[0]=0;plotZoomY[0]=1;}
 else if(id==940||id==941||id==950||id==951){size_t panel=id>=950?1:0;plotTypes[panel]=(id==940||id==950)?2:1;plotTypeChoice[panel].setSelectedId(plotTypes[panel],dontSendNotification);plotPanY[panel+1]=0;plotZoomY[panel+1]=1;activePlot=(int)panel+1;}
 else if(id>=900&&id<=914){if(id>=912)dockMeterState.sidebar(false);libraryPanel.command(id);}
 else if(id>=310&&id<=316){plotSmooth[0].setSelectedId(id-309,sendNotificationSync);}
 else if(id>=320&&id<=326){plotSmooth[1].setSelectedId(id-319,sendNotificationSync);}
 else if(id>=850&&id<=855){
  auto n=(size_t)activePlot;bool increase=id%2==0;double factor=increase?1.25:.8;
  if(id>=852)PlotPresentation::horizontalZoom(plotZoomX[n],plotPanX[n],!increase,activePlot==0);
  if(id<854){if(activePlot==0)plotZoomY[n]=jlimit(.25,64.,plotZoomY[n]*factor);else{int kind=mode==0?displayType(activePlot):activePlot;double span=tfSettings.span(kind),step=kind==1?tfSettings.phaseZoom:tfSettings.magZoom;double visible=span/plotZoomY[n];plotZoomY[n]=jlimit(.25,64.,span/std::max(span/64.,visible+(increase?-step:step)));}}
  if(tfSettings.linkFrequencyZoom&&activePlot>0&&id>=852){PlotPresentation::linkFrequency(plotZoomX,plotPanX,activePlot);}
 }
 else if(id>=856&&id<=859){
  auto n=(size_t)activePlot;
  if(id==856||id==857){PlotPresentation::horizontalZoom(plotZoomX[n],plotPanX[n],id==857,activePlot==0);if(tfSettings.linkFrequencyZoom&&activePlot>0){PlotPresentation::linkFrequency(plotZoomX,plotPanX,activePlot);}}
  else{double step=activePlot==0?.05:(mode==0?displayType(activePlot):activePlot)==1?tfSettings.phaseScroll/tfSettings.span(1):tfSettings.magScroll/tfSettings.span(2);if(fineCommandStep)step*=.2;plotPanY[n]=jlimit(-10.,10.,plotPanY[n]+(id==858?step:-step));}
 }
 else switch(id){
 case 506:beginInputs(15);break;
 case 515:{int n=source.getSelectedId()-1;if(n>=0&&n<4){beginInputs(1u<<n);}break;}
 case 507:stopInputs();break;
 case 508:stopInputs("Entradas en pausa. Pulsa Play para reabrir.");break;
 case 101:load();break;case 102:save();break;case 103:captureTrace();break;
 case 201:audioSettings();if(auto* window=getTopLevelComponent())window->toFront(true);break;case 202:engineSettings(source.getSelectedId()-1);break;
 case 971:meterState.mainVisibility(!meterState.state().mainVisible);break;case 972:meterState.clockMode(!meterState.state().clock);break;case 973:principalMeterOptions();break;case 974:if(auto* c=meterState.primaryCard())meterState.resetMax(c->id);break;case 975:if(auto* c=meterState.primaryCard())meterState.resetLeq(c->id);break;
 case 517:signalOptions("pink");break;case 518:signalOptions("sweep");break;case 970:signalOptions("loop");break;
 case 203:showMeters();break;case 303:tfGraphSettings(1);break;case 963:tfGraphSettings(2);break;case 964:tfGraphSettings(0);break;case 960:case 961:dockMeterState.sidebar(id==961?!dockMeterState.state().sidebarMeters:false);leftVisible=true;break;
 case 301:{
  linkSmooth.setToggleState(!linkSmooth.getToggleState(),dontSendNotification);
  if(linkSmooth.getToggleState()){plotSmooth[1].setSelectedId(plotSmooth[0].getSelectedId(),dontSendNotification);}
  break;
 }
 case 380:case 381:commands.language=id==380?MyaCommands::Language::ES:MyaCommands::Language::EN;if(graphWindow&&graphWindow->isVisible()){int tab=graphWindow->selectedTab();tfGraphSettings(tab,false);}menuItemsChanged();break;
 case 980:select(0);measurementView=0;plotChoice.setSelectedId(1,dontSendNotification);tfSettings.showIR=true;cursorPlot=-1;activePlot=1;status="Vista TF DEMO / impulso, fase y magnitud; dBFS de entrada independiente";break;
 case 981:measurementView=1;cursorPlot=-1;activePlot=0;status="IR DEMO sintetica / no medicion real; motor IR pendiente";break;
 case 401:case 402:case 403:case 404:select(id-401);break;
 case 405:leftVisible=!leftVisible;break;case 406:rightVisible=!rightVisible;break;
 case 407:plotChoice.setSelectedId(plotChoice.getSelectedId()==1?2:1,sendNotificationSync);break;
 case 408:showCoherence.setToggleState(!showCoherence.getToggleState(),dontSendNotification);break;
 case 409:restoreView();break;
 case 520:{auto n=(size_t)(source.getSelectedId()-1);micDelay[n].setValue(0);selectedDelay.setValue(0);break;}
 case 290:showCalibration();break;
 case 602:{closeCalibration();if(settingsDrawer)settingsDrawer->setVisible(false);break;}
 case 701:info(commands.language==MyaCommands::Language::ES?"Atajos / perfil Mya":"Shortcuts / Mya profile",commands.shortcutsHelp());break;
 case 702:info(commands.language==MyaCommands::Language::ES?"Estado real":"Actual status",commands.language==MyaCommands::Language::ES?
  juce::String::fromUTF8("Datos sintéticos. Audio: entradas dBFS con Play explicito; graficas sinteticas, TF pendiente.\nDSP: ganancia, delay, polaridad y bypass visual demo. EQ/FIR y propuestas pendientes.\nDiseño: geometría demo, sin predicción SPL validada. Medidores: FS Peak de entradas abiertas; SPL/Leq sin calibracion ni estimador.\nProcesadores: API, archivo y ficha manual pendientes por modelo/firmware.\nES/EN cubre menús y comandos; el resto de la interfaz aún es parcial."):
  "Synthetic data. Audio: explicit Play opens dBFS inputs; synthetic plots, TF pending.\nDSP: demo gain, delay, polarity and visual bypass. EQ/FIR and proposals pending.\nDesign: demo geometry, no validated SPL prediction. Meters: FS Peak from opened inputs; SPL RMS unweighted with confirmed amplitude calibration; A/C, Fast/Slow and Leq pending.\nProcessors: API, file and manual sheet pending per model/firmware.\nES/EN covers menus and commands; other interface text is still partial.");break;
 case 703:showMyaAbout();break;
 case 790:checkForUpdates(true);break;
 case 801:polarity=!polarity;break;case 802:disabled=!disabled;break;
 default:break;
 }
 if((id>=930&&id<=951)||id==409||id==301)persistGraphChoices();
 if(id==301||id==407||id==408||id==409)persistTF();
 if(id==405||id==406||id==409||id==960||id==961)persistWorkspace();
 resized();if(graphWindow)graphWindow->repaint();repaint();menuItemsChanged();
}
void setupCommands(){
 commands.context=[this]{return commandContext();};commands.action=[this](int id){executeCommand(id);};
 const auto& top=MyaCommands::topBindings;
 jassert(buttons.size()==top.size());
 for(size_t i=0;i<buttons.size();++i)commands.bind(*buttons[i],top[i],top[i]==703||top[i]==815||top[i]==832||top[i]==817||top[i]==818);
 const auto& measurement=MyaCommands::measurementBindings;
 jassert(measurementButtons.size()==measurement.size());
 for(size_t i=0;i<measurementButtons.size();++i)commands.bind(*measurementButtons[i],measurement[i],true);
 for(size_t k=0;k<tfActions.size();++k)for(size_t j=0;j<tfActions[k].size();++j){auto& button=tfActions[k][j];commands.bind(button,MyaCommands::tfBindings[j],true,(int)k+1);button.onClick=[this,k,j]{source.setSelectedId((int)k+1,sendNotificationSync);commands.invoke(MyaCommands::tfBindings[j]);};}
 for(size_t i=0;i<8;++i){int id=MyaCommands::topBindings[i];buttons[i]->onClick=[this,id]{commands.invoke(id);navigationOverlay.setVisible(false);navigationTab.setButtonText("v");if(getPeer()!=nullptr)grabKeyboardFocus();};}
 // Checkboxes share the exact command action; values remain selected from current state.
 commands.bind(showCoherence,408,true);
 averageChoice.setEnabled(false);averageChoice.setTooltip("Motor de promedio pendiente / Averaging engine pending");
 commands.refresh();
}
