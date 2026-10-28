#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace MyaAppData {
inline juce::File directory(){return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Mya Acustica");}
inline juce::File projectFile(){return directory().getChildFile("proyecto-demo.json");}

enum class MigrationResult { Migrated, NoSourceData, DestinationExists, Failed };

inline bool safeTree(const juce::File& file,int depth=0){
 if(depth>64||file.isSymbolicLink())return false;
 if(file.existsAsFile())return true;
 if(!file.isDirectory())return false;
 for(const auto& child:file.findChildFiles(juce::File::findFilesAndDirectories,false))if(!safeTree(child,depth+1))return false;
 return true;
}

// Explicit source only: no build-machine path is embedded in the installed app.
inline MigrationResult migrateFrom(const juce::File& source,const juce::File& destination){
 juce::InterProcessLock migrationLock("MyaAppDataMigration-"+juce::String(destination.getFullPathName().hashCode64()));
 juce::InterProcessLock::ScopedLockType guard(migrationLock);
 if(!guard.isLocked())return MigrationResult::Failed;
 if(destination.exists()||destination.isSymbolicLink()||source==destination)return MigrationResult::DestinationExists;
 if(!source.isDirectory()||source.isSymbolicLink())return MigrationResult::NoSourceData;
 static constexpr std::array<const char*,10> files{{"proyecto-demo.json","entradas-niveles.json","calibraciones-spl.json","calibracion-loop.json","medidores.json","medidores-lateral.json","medidores-flotantes.json","vista-trabajo.json","vista-graficos.json","tf-graficos.json"}};
 static constexpr std::array<const char*,2> folders{{"Biblioteca","Capturas"}};
 std::vector<juce::File> entries;
 for(auto name:files){auto file=source.getChildFile(name);if(file.isSymbolicLink())return MigrationResult::Failed;if(file.exists()){if(!file.existsAsFile())return MigrationResult::Failed;entries.push_back(file);}}
 for(auto name:folders){auto folder=source.getChildFile(name);if(folder.isSymbolicLink())return MigrationResult::Failed;if(folder.exists()){if(!folder.isDirectory()||!safeTree(folder))return MigrationResult::Failed;entries.push_back(folder);}}
 if(entries.empty())return MigrationResult::NoSourceData;
 auto parent=destination.getParentDirectory();if(!parent.createDirectory().wasOk())return MigrationResult::Failed;
 auto stage=parent.getChildFile(destination.getFileName()+".migration-"+juce::Uuid().toString());
 if(!stage.createDirectory().wasOk())return MigrationResult::Failed;
 bool copied=true;
 for(const auto& entry:entries){auto target=stage.getChildFile(entry.getFileName());copied=entry.isDirectory()?entry.copyDirectoryTo(target):entry.copyFileTo(target);if(!copied)break;}
 if(!copied||destination.exists()||destination.isSymbolicLink()){stage.deleteRecursively(false);return copied?MigrationResult::DestinationExists:MigrationResult::Failed;}
 if(stage.moveFileTo(destination))return MigrationResult::Migrated;
 stage.deleteRecursively(false);return MigrationResult::Failed;
}
}
