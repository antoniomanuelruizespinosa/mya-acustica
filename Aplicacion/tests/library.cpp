#include "../src/CaptureLibrary.h"
#include "../src/CapturedTraceDisplay.h"
#include "../src/GraphViewState.h"
#include <cassert>
#include <iostream>
using namespace juce;
static var snapshot(){auto* o=new DynamicObject();o->setProperty("schema",1);o->setProperty("synthetic",true);o->setProperty("name","DEMO TF");o->setProperty("colour","ff54ee22");o->setProperty("sampleRateHz",var());o->setProperty("captureDSPState","unknown");Array<var> samples;
 for(int i=0;i<101;++i){auto* p=new DynamicObject();p->setProperty("frequencyHz",20.+199.8*i);p->setProperty("real",2.);p->setProperty("imaginary",0.);p->setProperty("coherence",.9);samples.add(var(p));}o->setProperty("samples",samples);return var(o);}
int main(int argc,char** argv){
 using K=CaptureLibrary::Kind;auto base=argc>1?(File::isAbsolutePath(String::fromUTF8(argv[1]))?File(String::fromUTF8(argv[1])):File::getCurrentWorkingDirectory().getChildFile(String::fromUTF8(argv[1]))):File::getCurrentWorkingDirectory().getChildFile("work/MYA-003/test-data");base.createDirectory();auto dir=base.getNonexistentChildFile("mya-library-test","",false);dir.createDirectory();
 CaptureLibrary lib(dir);if(!lib.ready())std::cerr<<"Library error: "<<lib.lastError()<<" at "<<dir.getFullPathName()<<std::endl;assert(lib.ready());auto session=lib.state().active;auto a=lib.createFolder("Sala A",session);auto b=lib.createFolder("Sala B",session);assert(a.isNotEmpty()&&b.isNotEmpty());assert(lib.setDestination(a));
 auto id=lib.capture(snapshot(),"Frente DEMO","ff54ee22");auto id2=lib.capture(snapshot(),"Lateral DEMO","ff4d88ff");assert(id.isNotEmpty()&&id2.isNotEmpty());
 auto average=snapshot();average.getDynamicObject()->setProperty("traceRole","average");assert(lib.capture(average,"AVG fake","ff54ee22").isEmpty());assert(lib.find(id)->role=="individual");
 auto original=lib.dataFile(id).loadFileAsString();auto original2=lib.dataFile(id2).loadFileAsString();auto oldPath=lib.dataFile(id);
 assert(lib.move({id,id2},b));assert(!oldPath.exists());assert(lib.dataFile(id).getParentDirectory()==lib.dataFile(b));assert(lib.dataFile(id).loadFileAsString()==original);assert(lib.dataFile(id2).loadFileAsString()==original2);
 assert(lib.setVisibility({id},false));assert(lib.setColour({id},"ffffe33b"));assert(lib.rename(id,"Principal DEMO"));assert(lib.rename(b,"Balcones"));assert(lib.dataFile(id).loadFileAsString()==original);
 assert(lib.setExpanded(b,false));assert(lib.setDestination(b));assert(lib.setView(K::ImpulseResponse));assert(lib.capture(snapshot(),"No IR","ff54ee22").isEmpty());
 CaptureLibrary reopened(dir);assert(reopened.ready());assert(reopened.state().active==b&&reopened.state().view==K::ImpulseResponse);assert(!reopened.find(id)->visible&&reopened.find(id)->colour=="ffffe33b");assert(reopened.find(id)->name=="Principal DEMO"&&!reopened.find(b)->expanded);assert(reopened.dataFile(id).loadFileAsString()==original);
 assert(!reopened.move({session},b));assert(!reopened.move({b},b));assert(reopened.find(b)->parent==session);
 // Stale writers are rejected without moving or overwriting either capture.
 assert(reopened.rename(id2,String::fromUTF8("Revisión nueva")));assert(!lib.rename(id,String::fromUTF8("Revisión obsoleta")));assert(reopened.dataFile(id).loadFileAsString()==original);
 assert(reopened.setView(K::SnapshotTF));auto trace=reopened.readTrace(id);assert(trace&&trace->samples.size()==101);assert(trace->metadata["measurementId"].toString()==id);auto point=CaptureLibrary::interpolate(*trace,1000);assert(point&&std::abs(point->value.real()-2)<1.e-12&&point->coherence&&*point->coherence==.9);
 assert(!CaptureLibrary::interpolate(*trace,19));assert(!CaptureLibrary::interpolate(*trace,20001));
 assert(reopened.setVisibility({id},true));CapturedTraceDisplay display;display.reload(reopened);auto db=display.value(id,1000,2,1,80);assert(db&&std::abs(*db*36-20*std::log10(2.))<1.e-10);assert(!display.value(id,1000,2,1,95));
 // Failed batch moves do not partially relocate data.
 auto before=reopened.dataFile(id);assert(!reopened.move({id,"missing-id"},a));assert(reopened.dataFile(id)==before&&before.loadFileAsString()==original);
 // Corrupt manifest is left untouched, never silently reset to an empty library.
 auto manifest=reopened.manifest();auto saved=manifest.loadFileAsString();assert(manifest.replaceWithText("{broken"));CaptureLibrary broken(dir);assert(!broken.ready());assert(manifest.loadFileAsString()=="{broken");assert(before.loadFileAsString()==original);assert(manifest.replaceWithText(saved));assert(reopened.reload());
 // Own-format adoption preserves the source and is idempotent; other formats are ignored.
 auto legacy=dir.getChildFile("Legacy");legacy.createDirectory();auto legacyFile=legacy.getChildFile("old.json");legacyFile.replaceWithText(JSON::toString(snapshot()));auto legacyOriginal=legacyFile.loadFileAsString();legacy.getChildFile("foreign.trf").replaceWithText("not imported");assert(reopened.adoptLegacy(legacy)==1);assert(reopened.adoptLegacy(legacy)==0);assert(legacyFile.loadFileAsString()==legacyOriginal);
 // Recover a crash between physical relocation and manifest commit, then a committed journal.
 auto simulate=[&](bool committed){
  auto beforeDoc=JSON::parse(reopened.manifest());auto afterDoc=beforeDoc.clone();afterDoc.getDynamicObject()->setProperty("revision",Uuid().toString());
  for(auto& item:*afterDoc["nodes"].getArray())if(item["id"].toString()==id)item.getDynamicObject()->setProperty("parentId",a);
  auto from=reopened.dataFile(id);auto to=reopened.dataFile(a).getChildFile(reopened.find(id)->diskName);
  auto* journal=new DynamicObject();journal->setProperty("before",beforeDoc);journal->setProperty("after",afterDoc);auto* relocation=new DynamicObject();relocation->setProperty("from",from.getRelativePathFrom(dir));relocation->setProperty("to",to.getRelativePathFrom(dir));Array<var> moves;moves.add(var(relocation));journal->setProperty("moves",moves);
  assert(dir.getChildFile("transaccion.json").replaceWithText(JSON::toString(var(journal))));assert(from.moveFileTo(to));
  if(committed)assert(reopened.manifest().replaceWithText(JSON::toString(afterDoc)));
  CaptureLibrary recovered(dir);assert(recovered.ready());assert(!dir.getChildFile("transaccion.json").exists());assert(recovered.dataFile(id).loadFileAsString()==original);assert(recovered.find(id)->parent==(committed?a:b));
  assert(reopened.reload());
 };
 simulate(false);simulate(true);
 // The same session remains active across data views; no IR/RTA record is fabricated.
 auto active=reopened.state().active;assert(reopened.setView(K::RTA));CaptureLibrary views(dir);assert(views.state().view==K::RTA&&views.state().active==active);for(const auto& n:views.state().nodes)assert(n.kind==K::Folder||n.kind==K::SnapshotTF);
 // Exact helpers used by the app: graph selectors survive file round-trips and reject unsupported views.
 auto prefs=dir.getChildFile("views.json");GraphViewState choices{{{2,2}},2};assert(choices.save(prefs));auto restored=GraphViewState::read(prefs);assert(restored&&restored->types[0]==2&&restored->types[1]==2&&restored->irScale==2);
 auto invalid=choices.encode();invalid.getDynamicObject()->setProperty("irScale",3);assert(!GraphViewState::decode(invalid));
 auto serialized=choices.encode();serialized["panelTypes"].getArray()->getReference(0)=3;assert(!GraphViewState::decode(serialized));
 // A missing index never silently resets a folder tree with existing payloads.
 auto manifestBefore=reopened.manifest().loadFileAsString();auto backup=dir.getChildFile("index-test-backup.json");assert(reopened.manifest().moveFileTo(backup));CaptureLibrary missingIndex(dir);assert(!missingIndex.ready());assert(!missingIndex.manifest().exists());assert(reopened.dataFile(id).loadFileAsString()==original);assert(backup.moveFileTo(reopened.manifest()));assert(reopened.manifest().loadFileAsString()==manifestBefore);
 std::cout<<"PASS: sessions, folders, rename, batch movement without payload loss, colour/visibility/destination/view persistence, cycle prevention, stale writer rejection, saved sample readout, invalid manifest preservation, own-demo adoption, journal rollback/finalization and empty IR/RTA classification\n";
 std::cout<<"Evidence directory: "<<dir.getFullPathName()<<"\n";
}
