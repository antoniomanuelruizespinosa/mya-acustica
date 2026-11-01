#include "../src/AppDataLocation.h"
#include "../src/ConfigurationBundle.h"
#include <cassert>
#include <iostream>

int main(int argc,char** argv){
 assert(argc==2);
 using namespace MyaAppData;
 assert(projectFile().getParentDirectory()==directory());
 assert(directory().getParentDirectory()==juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory));
 auto root=juce::File(juce::String::fromUTF8(argv[1])).getChildFile(juce::Uuid().toString());
 assert(root.createDirectory().wasOk());
 auto source=root.getChildFile("legacy"),destination=root.getChildFile("user-data");
 assert(source.createDirectory().wasOk());
 assert(source.getChildFile("proyecto-demo.json").replaceWithText("project-original"));
 assert(source.getChildFile("entradas-niveles.json").replaceWithText("routes-original"));
 assert(source.getChildFile("calibraciones-spl.json").replaceWithText("spl-original"));
 assert(source.getChildFile("calibracion-loop.json").replaceWithText("loop-original"));
 assert(source.getChildFile("vista-trabajo.json").replaceWithText("view-original"));
 assert(source.getChildFile("ignored.txt").replaceWithText("not app data"));
 auto library=source.getChildFile("Biblioteca");assert(library.getChildFile("Datos").createDirectory().wasOk());
 assert(library.getChildFile("biblioteca.json").replaceWithText("manifest-original"));
 assert(library.getChildFile("Datos").getChildFile("capture.json").replaceWithText("capture-original"));
 auto captures=source.getChildFile("Capturas");assert(captures.createDirectory().wasOk());
 assert(captures.getChildFile("legacy.json").replaceWithText("legacy-capture"));
 assert(migrateFrom(source,destination)==MigrationResult::Migrated);
 for(auto name:{"proyecto-demo.json","entradas-niveles.json","calibraciones-spl.json","calibracion-loop.json","vista-trabajo.json"})assert(destination.getChildFile(name).loadFileAsString()==source.getChildFile(name).loadFileAsString());
 assert(destination.getChildFile("Biblioteca/biblioteca.json").loadFileAsString()=="manifest-original");
 assert(destination.getChildFile("Biblioteca/Datos/capture.json").loadFileAsString()=="capture-original");
 assert(destination.getChildFile("Capturas/legacy.json").loadFileAsString()=="legacy-capture");
 assert(!destination.getChildFile("ignored.txt").exists()&&source.getChildFile("ignored.txt").existsAsFile());
 assert(migrateFrom(source,destination)==MigrationResult::DestinationExists);
 assert(destination.getChildFile("proyecto-demo.json").replaceWithText("newer-project"));
 assert(migrateFrom(source,destination)==MigrationResult::DestinationExists);
 assert(destination.getChildFile("proyecto-demo.json").loadFileAsString()=="newer-project");
 assert(source.getChildFile("proyecto-demo.json").loadFileAsString()=="project-original");
 assert(migrateFrom(root.getChildFile("missing"),root.getChildFile("missing-target"))==MigrationResult::NoSourceData);
 auto invalid=root.getChildFile("invalid");assert(invalid.createDirectory().wasOk());
 assert(invalid.getChildFile("Biblioteca").replaceWithText("wrong type"));
 assert(migrateFrom(invalid,root.getChildFile("invalid-target"))==MigrationResult::Failed);
 assert(!root.getChildFile("invalid-target").exists());
 auto outside=root.getChildFile("outside.json");assert(outside.replaceWithText("outside"));
 auto linked=root.getChildFile("linked");assert(linked.createDirectory().wasOk());
 if(outside.createSymbolicLink(linked.getChildFile("proyecto-demo.json"),false)){
  assert(migrateFrom(linked,root.getChildFile("linked-target"))==MigrationResult::Failed);
  assert(!root.getChildFile("linked-target").exists());
 }
 auto configRoot=root.getChildFile("config-test");assert(configRoot.createDirectory().wasOk());
 auto project=configRoot.getChildFile("proyecto-demo.json");assert(project.replaceWithText(R"({"schema":1,"micros":[{"name":"MIC 1"}]})"));
 assert(configRoot.getChildFile("entradas-niveles.json").replaceWithText(R"({"schema":3,"routes":[]})"));
 auto calibration=configRoot.getChildFile("calibraciones-spl.json");
 auto makeCalibration=[](int channel,double digital,double offset){auto* root=new juce::DynamicObject();root->setProperty("schema",1);root->setProperty("convention","rms-sample-power-v1");auto* record=new juce::DynamicObject();record->setProperty("deviceKey","Core / Card");record->setProperty("backend","Core");record->setProperty("device","Card");record->setProperty("channel",channel);record->setProperty("microphone","Mic");record->setProperty("gainDeclared","20 dB");record->setProperty("dateISO8601","2026-11-01T12:00:00+01:00");record->setProperty("referenceDb",94.);record->setProperty("digitalRmsDbFS",digital);record->setProperty("offsetDb",offset);record->setProperty("durationSeconds",5.);record->setProperty("rangeDb",0.);juce::Array<juce::var> rows;rows.add(juce::var(record));root->setProperty("records",rows);return juce::JSON::toString(juce::var(root));};
 assert(calibration.replaceWithText(makeCalibration(1,-43.,137.)));
 auto bundle=ConfigurationBundle::capture(project,"Prueba OCTA");assert(ConfigurationBundle::valid(bundle));auto exported=configRoot.getChildFile("export.json");assert(ConfigurationBundle::write(exported,bundle));
 assert(project.replaceWithText(R"({"schema":1,"micros":[]})"));assert(calibration.replaceWithText(makeCalibration(9,-40.,134.)));
 assert(ConfigurationBundle::restore(project,juce::JSON::parse(exported)));assert(project.loadFileAsString().contains("MIC 1"));
 auto restored=juce::JSON::parse(calibration);assert(restored["records"].getArray()->size()==2);bool one=false,nine=false;for(const auto& row:*restored["records"].getArray()){if(int(row["channel"])==1)one=double(row["offsetDb"])==137.;if(int(row["channel"])==9)nine=double(row["offsetDb"])==134.;}assert(one&&nine);

 std::cout<<"PASS stable per-user path, explicit migration of project/routes/calibrations/library/captures, source preserved, existing destination not overwritten, unrelated files excluded.\n";
}
