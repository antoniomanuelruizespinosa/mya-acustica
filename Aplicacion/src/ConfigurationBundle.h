#pragma once
#include <juce_core/juce_core.h>
#include <map>

// A portable snapshot of configuration files. Measurement captures and SPL
// calibration records are deliberately outside the configuration bundle.
struct ConfigurationBundle {
 static juce::StringArray names(){return {"project", "entradas-niveles.json", "vista-trabajo.json", "tf-graficos.json", "vista-graficos.json", "medidores.json", "medidores-lateral.json", "medidores-flotantes.json"};}
 static juce::File target(const juce::File& project,const juce::String& name){return name=="project"?project:project.getParentDirectory().getChildFile(name);}
 static juce::var capture(const juce::File& project,const juce::String& title){auto* root=new juce::DynamicObject();root->setProperty("schema",1);root->setProperty("name",title);auto* files=new juce::DynamicObject();for(const auto& name:names()){auto file=target(project,name);if(file.existsAsFile())files->setProperty(name,file.loadFileAsString());}root->setProperty("files",juce::var(files));return juce::var(root);}
 static bool valid(const juce::var& bundle){if(!bundle.isObject()||int(bundle["schema"])!=1||!bundle["files"].isObject())return false;for(const auto& name:names()){auto value=bundle["files"].getDynamicObject()->getProperty(juce::Identifier(name));if(value.isVoid())continue;if(!value.isString()||!juce::JSON::parse(value.toString()).isObject())return false;}return bundle["files"]["project"].isString();}
 static bool write(const juce::File& file,const juce::var& bundle){if(!valid(bundle))return false;juce::TemporaryFile temp(file);return temp.getFile().replaceWithText(juce::JSON::toString(bundle))&&temp.overwriteTargetFileWithTemporary();}
 static bool restore(const juce::File& project,const juce::var& bundle){if(!valid(bundle))return false;juce::StringArray written;std::map<juce::String,juce::String> originals;for(const auto& name:names()){auto value=bundle["files"].getDynamicObject()->getProperty(juce::Identifier(name));if(value.isVoid())continue;auto file=target(project,name);originals[name]=file.existsAsFile()?file.loadFileAsString():juce::String{};juce::TemporaryFile temp(file);if(!temp.getFile().replaceWithText(value.toString())||!temp.overwriteTargetFileWithTemporary()){for(const auto& done:written){auto previous=originals[done];auto destination=target(project,done);if(previous.isEmpty())destination.deleteFile();else destination.replaceWithText(previous);}return false;}written.add(name);}return true;}
};
