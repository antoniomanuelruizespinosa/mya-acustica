#include "../src/AppDataLocation.h"
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
 std::cout<<"PASS stable per-user path, explicit migration of project/routes/calibrations/library/captures, source preserved, existing destination not overwritten, unrelated files excluded.\n";
}
