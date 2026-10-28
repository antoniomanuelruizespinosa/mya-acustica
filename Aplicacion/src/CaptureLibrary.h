#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <complex>
#include <optional>
#include <set>
#include <vector>

// Own-format library. Original payloads are immutable; names/view/visibility live in the manifest.
// This class has no audio, GUI, Smaart parser, DSP or import engine.
class CaptureLibrary {
public:
 enum class Kind { Folder, SnapshotTF, ImpulseResponse, RTA };
 struct Node {
  juce::String id,parent,name,diskName,colour="ff54ee22",method="unknown",origin="original",derivedFrom,legacyKey,role="individual";
  Kind kind=Kind::Folder;bool visible=true,expanded=true;
 };
 struct State {
  juce::String id,revision,active;
  Kind view=Kind::SnapshotTF;
  std::vector<Node> nodes;
 };
 struct Sample {double frequency=0;std::complex<double> value;std::optional<double> coherence;};
 struct Trace {Node node;std::vector<Sample> samples;juce::var metadata;};
private:
 juce::File root;State data;juce::String error;bool usable=false;
 struct Move {juce::File from,to;};
 struct Lock {
  juce::InterProcessLock lock;bool entered;
  explicit Lock(const juce::String& path):lock("MyaCaptureLibrary-"+juce::String(path.hashCode64())),entered(lock.enter(250)){}
  ~Lock(){if(entered)lock.exit();}
 };
 static bool numeric(const juce::var& v){return v.isDouble()||v.isInt()||v.isInt64();}
 static bool relativePath(const juce::String& path){
  if(path.isEmpty()||juce::File::isAbsolutePath(path))return false;
  auto parts=juce::StringArray::fromTokens(path.replaceCharacter('\\','/'),"/","");
  for(const auto& part:parts)if(part=="."||part==".."||part.isEmpty())return false;return true;
 }
 static juce::String uid(){return juce::Uuid().toString();}
 static bool safeName(const juce::String& name){return name.isNotEmpty()&&name!="."&&name!=".."&&!name.containsAnyOf("/\\:");}
 static juce::String storageName(const juce::String& name,const juce::String& id){return juce::File::createLegalFileName(name).substring(0,50)+"__"+id;}
 const Node* node(const State& s,const juce::String& id) const {for(const auto& n:s.nodes)if(n.id==id)return &n;return nullptr;}
 static Node* mutableNode(State& s,const juce::String& id){for(auto& n:s.nodes)if(n.id==id)return &n;return nullptr;}
 juce::File file(const State& s,const Node& n) const {
  std::vector<juce::String> path{n.diskName};auto parent=n.parent;
  while(parent.isNotEmpty()){auto* p=node(s,parent);if(!p)return {};path.push_back(p->diskName);parent=p->parent;}
  auto f=root.getChildFile("Datos");for(auto it=path.rbegin();it!=path.rend();++it)f=f.getChildFile(*it);return f;
 }
 bool validate(const State& s) const {
  if(s.id.isEmpty()||s.revision.isEmpty()||s.nodes.size()>10000)return false;
  std::set<juce::String> ids,paths;
  for(const auto& n:s.nodes){
   if(n.id.isEmpty()||!ids.insert(n.id).second||n.name.trim().isEmpty()||!safeName(n.diskName))return false;
   if(n.role!="individual"&&n.role!="average")return false;
   if(n.role=="average"&&n.kind!=Kind::SnapshotTF)return false;
   if(n.kind!=Kind::Folder&&n.kind!=Kind::SnapshotTF&&n.kind!=Kind::ImpulseResponse&&n.kind!=Kind::RTA)return false;
   std::set<juce::String> ancestors{n.id};auto parent=n.parent;
   while(parent.isNotEmpty()){auto* p=node(s,parent);if(!p||p->kind!=Kind::Folder||ancestors.size()>64||!ancestors.insert(parent).second)return false;parent=p->parent;}
   if(!paths.insert(n.parent+"/"+n.diskName).second)return false;
   if(n.kind!=Kind::Folder&&(n.colour.length()!=8||!n.colour.containsOnly("0123456789abcdefABCDEF")))return false;
   if(n.derivedFrom.isNotEmpty()){
    std::set<juce::String> lineage{n.id};auto original=n.derivedFrom;
    while(original.isNotEmpty()){auto* source=node(s,original);if(!source||source->kind==Kind::Folder||!lineage.insert(original).second||lineage.size()>64)return false;original=source->derivedFrom;}
   }
  }
  auto* active=node(s,s.active);return active&&active->kind==Kind::Folder;
 }
 static juce::var encode(const State& s){
  auto* o=new juce::DynamicObject();o->setProperty("schema",1);o->setProperty("libraryId",s.id);o->setProperty("revision",s.revision);o->setProperty("activeFolderId",s.active);o->setProperty("view",(int)s.view);
  juce::Array<juce::var> nodes;
  for(const auto& n:s.nodes){auto* p=new juce::DynamicObject();p->setProperty("id",n.id);p->setProperty("parentId",n.parent);p->setProperty("name",n.name);p->setProperty("diskName",n.diskName);p->setProperty("kind",(int)n.kind);p->setProperty("colour",n.colour);p->setProperty("visible",n.visible);p->setProperty("expanded",n.expanded);p->setProperty("acquisitionMethod",n.method);p->setProperty("origin",n.origin);p->setProperty("derivedFromId",n.derivedFrom);p->setProperty("legacyKey",n.legacyKey);p->setProperty("traceRole",n.role);nodes.add(juce::var(p));}
  o->setProperty("nodes",nodes);return juce::var(o);
 }
 bool decode(const juce::var& v,State& s) const {
  if(!v.isObject()||int(v["schema"])!=1||!v["nodes"].isArray())return false;
  s={};s.id=v["libraryId"].toString();s.revision=v["revision"].toString();s.active=v["activeFolderId"].toString();s.view=(Kind)int(v["view"]);
  if(s.view!=Kind::SnapshotTF&&s.view!=Kind::ImpulseResponse&&s.view!=Kind::RTA)return false;
  for(const auto& x:*v["nodes"].getArray()){
   if(!x.isObject())return false;Node n;n.id=x["id"].toString();n.parent=x["parentId"].toString();n.name=x["name"].toString();n.diskName=x["diskName"].toString();n.kind=(Kind)int(x["kind"]);n.colour=x["colour"].toString();n.visible=bool(x["visible"]);n.expanded=bool(x["expanded"]);n.method=x["acquisitionMethod"].toString();n.origin=x["origin"].toString();n.derivedFrom=x["derivedFromId"].toString();n.legacyKey=x["legacyKey"].toString();if(x.hasProperty("traceRole"))n.role=x["traceRole"].toString();s.nodes.push_back(std::move(n));
  }return validate(s);
 }
 static bool atomicWrite(const juce::File& f,const juce::var& v){
  juce::TemporaryFile tmp(f);return tmp.getFile().replaceWithText(juce::JSON::toString(v))&&tmp.overwriteTargetFileWithTemporary();
 }
 juce::File journalPath() const{return root.getChildFile("transaccion.json");}
 bool safeStorageFile(const juce::File& f) const {
  if(!f.isAChildOf(root)||f.isSymbolicLink())return false;
  auto rel=f.getRelativePathFrom(root).replaceCharacter('\\','/');
  if(!(rel.startsWith("Datos/")||rel.startsWith("Pendientes/")))return false;
  for(auto p=f.getParentDirectory();p!=root&&p.isAChildOf(root);p=p.getParentDirectory())if(p.isSymbolicLink())return false;
  return true;
 }
 bool recover(){
  auto journal=journalPath();if(!journal.existsAsFile())return true;
  auto v=juce::JSON::parse(journal);State before,after;
  if(!decode(v["before"],before)||!decode(v["after"],after)||!v["moves"].isArray()){error=juce::String::fromUTF8("Diario de biblioteca inválido; se conservan archivos");return false;}
  auto stored=juce::JSON::parse(manifest());bool committed=stored["revision"].toString()==after.revision;
  std::vector<Move> moves;
  for(const auto& m:*v["moves"].getArray()){
   auto from=juce::File::createFileWithoutCheckingPath(root.getFullPathName()+"/"+m["from"].toString().replaceCharacter('\\','/'));auto to=juce::File::createFileWithoutCheckingPath(root.getFullPathName()+"/"+m["to"].toString().replaceCharacter('\\','/'));
   if(!safeStorageFile(from)||!safeStorageFile(to)||!relativePath(m["from"].toString())||!relativePath(m["to"].toString())){error=juce::String::fromUTF8("Rutas de recuperación inválidas");return false;}moves.push_back({from,to});
  }
  if(committed){for(const auto& m:moves)if(!m.to.exists()||m.from.exists()){error=juce::String::fromUTF8("Transacción incompleta; revisar archivos conservados");return false;}}
  else{
   if(stored.isObject()&&stored["revision"].toString()!=before.revision){error=juce::String::fromUTF8("Conflicto de revisión; no se sobrescribe la biblioteca");return false;}
   for(auto i=moves.rbegin();i!=moves.rend();++i){
    if(i->to.exists()){if(i->from.exists()||!i->from.getParentDirectory().createDirectory().wasOk()||!i->to.moveFileTo(i->from)){error=juce::String::fromUTF8("Recuperación pendiente; no se elimina ningún dato");return false;}}
    else if(!i->from.exists()){error=juce::String::fromUTF8("Archivo de transacción ausente; se conserva diario");return false;}
   }
   if(!atomicWrite(manifest(),encode(before))){error=juce::String::fromUTF8("No se pudo restaurar el índice");return false;}
  }
  if(!journal.deleteFile()){error=juce::String::fromUTF8("No se pudo cerrar el diario de recuperación");return false;}return true;
 }
 bool transaction(State next,const std::vector<Move>& moves={}){
  if(!usable){return false;}Lock lock(root.getFullPathName());if(!lock.entered){error="Biblioteca ocupada por otra instancia";return false;}error.clear();next.revision=uid();
  if(!validate(next)){error=juce::String::fromUTF8("Estructura de biblioteca inválida");return false;}
  // Refuse to overwrite another running instance's changes. No stale last-writer-wins.
  if(!manifest().existsAsFile()||juce::JSON::parse(manifest())["revision"].toString()!=data.revision){usable=false;error=juce::String::fromUTF8("La biblioteca cambió en otra instancia; recarga antes de editar");return false;}
  if(journalPath().existsAsFile()){error=juce::String::fromUTF8("Existe una transacción pendiente; recarga la biblioteca");return false;}
  for(const auto& m:moves)if(!safeStorageFile(m.from)||!safeStorageFile(m.to)||!m.from.exists()||m.to.exists()){error="Movimiento no seguro o destino ocupado";return false;}
  auto* j=new juce::DynamicObject();j->setProperty("before",encode(data));j->setProperty("after",encode(next));juce::Array<juce::var> list;
  for(const auto& m:moves){auto* o=new juce::DynamicObject();o->setProperty("from",m.from.getRelativePathFrom(root));o->setProperty("to",m.to.getRelativePathFrom(root));list.add(juce::var(o));}j->setProperty("moves",list);
  if(!atomicWrite(journalPath(),juce::var(j))){error="No se pudo guardar el diario; no se movieron datos";return false;}
  size_t moved=0;for(const auto& m:moves){if(!m.to.getParentDirectory().createDirectory().wasOk()||!m.from.moveFileTo(m.to))break;++moved;}
  bool saved=moved==moves.size()&&atomicWrite(manifest(),encode(next));
  if(!saved){
   bool rolledBack=true;while(moved>0){--moved;const auto& m=moves[moved];if(!m.to.moveFileTo(m.from))rolledBack=false;}
   if(rolledBack)journalPath().deleteFile();else usable=false;
   error=rolledBack?juce::String::fromUTF8("No se guardó la operación; originales conservados"):juce::String::fromUTF8("Recuperación pendiente; diario conservado");return false;
  }
  data=std::move(next);
  if(!journalPath().deleteFile()){usable=false;error="Datos guardados; recarga para cerrar el diario";}
  return true;
 }
public:
 explicit CaptureLibrary(juce::File directory):root(std::move(directory)){reload();}
 juce::File manifest() const{return root.getChildFile("biblioteca.json");}
 const State& state() const{return data;}
 const Node* find(const juce::String& id) const{return node(data,id);}
 bool ready() const{return usable;}
 const juce::String& lastError() const{return error;}
 juce::File dataFile(const juce::String& id) const{auto* n=find(id);return n?file(data,*n):juce::File{};}
 bool reload(){
  Lock lock(root.getFullPathName());if(!lock.entered){error="Biblioteca ocupada por otra instancia";return false;}
  usable=false;error.clear();if(!root.createDirectory().wasOk()||root.isSymbolicLink()){error="No se puede abrir la biblioteca";return false;}
  if(!recover())return false;
  if(manifest().existsAsFile()){
   State loaded;if(!decode(juce::JSON::parse(manifest()),loaded)){error=juce::String::fromUTF8("Índice de biblioteca inválido; no se sobrescribe");return false;}
   for(const auto& n:loaded.nodes)if(!safeStorageFile(file(loaded,n))||!file(loaded,n).exists()){error=juce::String::fromUTF8("Falta un archivo o carpeta; índice conservado");return false;}
   data=std::move(loaded);usable=true;return true;
  }
  if(root.getChildFile("Datos").isDirectory()&&!root.getChildFile("Datos").findChildFiles(juce::File::findFilesAndDirectories,false).isEmpty()){error=juce::String::fromUTF8("Falta el índice de biblioteca; se conservan los datos");return false;}
  data={};data.id=uid();data.revision=uid();Node session;session.id=uid();session.name=juce::String::fromUTF8("Sesión demo");session.diskName=storageName(session.name,session.id);data.active=session.id;data.nodes.push_back(session);
  if(!file(data,session).createDirectory().wasOk()||!atomicWrite(manifest(),encode(data))){error="No se pudo crear la biblioteca";return false;}usable=true;return true;
 }
 juce::String folderPath(const juce::String& id) const {
  auto* n=find(id);if(!n)return {};auto value=n->name;auto parent=n->parent;
  while(auto* p=find(parent)){value=p->name+" / "+value;parent=p->parent;}return value;
 }
 juce::String createFolder(juce::String name,juce::String parent,bool session=false){
  name=name.trim();if(!ready()||name.isEmpty()){error="Escribe un nombre";return {};}
  if(!session){auto* p=find(parent);if(!p||p->kind!=Kind::Folder){error=juce::String::fromUTF8("Selecciona una carpeta válida");return {};}}
  State next=data;Node n;n.id=uid();n.name=name;n.parent=session?juce::String{}:parent;n.diskName=storageName(name,n.id);next.nodes.push_back(n);if(session)next.active=n.id;
  if(!file(next,n).createDirectory().wasOk()){error="No se pudo crear la carpeta";return {};}
  return transaction(next)?n.id:juce::String{};
 }
 bool rename(const juce::String& id,juce::String name){
  name=name.trim();auto* old=find(id);if(!old||name.isEmpty()){error="Selecciona un elemento y un nombre";return false;}
  State next=data;auto* n=mutableNode(next,id);n->name=name;
  if(n->kind==Kind::Folder){n->diskName=storageName(name,id);if(n->diskName!=old->diskName)return transaction(next,{{file(data,*old),file(next,*n)}});}
  return transaction(next);
 }
 bool setDestination(const juce::String& id){auto* n=find(id);if(!n||n->kind!=Kind::Folder){error="El destino debe ser una carpeta";return false;}auto next=data;next.active=id;return transaction(next);}
 bool setView(Kind kind){if(kind!=Kind::SnapshotTF&&kind!=Kind::ImpulseResponse&&kind!=Kind::RTA)return false;auto next=data;next.view=kind;return transaction(next);}
 bool setExpanded(const juce::String& id,bool open){auto* n=find(id);if(!n||n->kind!=Kind::Folder)return false;if(n->expanded==open)return true;auto next=data;mutableNode(next,id)->expanded=open;return transaction(next);}
 bool setVisibility(const std::vector<juce::String>& ids,bool visible){auto next=data;for(const auto& id:ids){auto* n=mutableNode(next,id);if(n&&n->kind!=Kind::Folder)n->visible=visible;}return transaction(next);}
 bool setColour(const std::vector<juce::String>& ids,const juce::String& colour){
  if(colour.length()!=8||!colour.containsOnly("0123456789abcdefABCDEF")){error=juce::String::fromUTF8("Color inválido");return false;}auto next=data;for(const auto& id:ids){auto* n=mutableNode(next,id);if(n&&n->kind!=Kind::Folder)n->colour=colour;}return transaction(next);
 }
 bool move(const std::vector<juce::String>& ids,const juce::String& destination){
  auto* target=find(destination);if(!target||target->kind!=Kind::Folder){error=juce::String::fromUTF8("Destino no válido");return false;}
  std::set<juce::String> selected(ids.begin(),ids.end());std::vector<juce::String> roots;
  for(const auto& id:ids){auto* n=find(id);if(!n){error=juce::String::fromUTF8("Selección no válida");return false;}bool ancestor=false;auto parent=n->parent;
   while(auto* p=find(parent)){if(selected.count(p->id))ancestor=true;parent=p->parent;}if(!ancestor&&std::find(roots.begin(),roots.end(),id)==roots.end())roots.push_back(id);
  }
  auto next=data;std::vector<Move> moves;
  for(const auto& id:roots){auto* n=find(id);auto parent=destination;
   while(parent.isNotEmpty()){if(parent==id){error=juce::String::fromUTF8("No se puede mover una carpeta dentro de sí misma");return false;}parent=find(parent)->parent;}
   if(n->parent==destination)continue;auto* changed=mutableNode(next,id);changed->parent=destination;moves.push_back({file(data,*n),file(next,*changed)});
  }
  return moves.empty()?true:transaction(next,moves);
 }
 // Only this application's explicitly synthetic TF payload is currently capturable.
 juce::String capture(const juce::var& payload,juce::String name,juce::String colour,juce::String legacyKey={},bool adopting=false){
  if(!ready()||(!adopting&&data.view!=Kind::SnapshotTF)){error=juce::String::fromUTF8("Solo hay adquisición demo de snapshots TF");return {};}
  if(!payload.isObject()||!payload["synthetic"].isBool()||!bool(payload["synthetic"])||int(payload["schema"])!=1||!payload["samples"].isArray()||payload["samples"].getArray()->size()<2){error=juce::String::fromUTF8("Snapshot TF demo no válido");return {};}
  if(payload["traceRole"].toString()=="average"){error="AVG: estimador pendiente";return {};}
  double previous=0;
  for(const auto& point:*payload["samples"].getArray()){
   if(!point.isObject()||!numeric(point["frequencyHz"])||!numeric(point["real"])||!numeric(point["imaginary"])){error="Snapshot incompleto";return {};}
   double frequency=double(point["frequencyHz"]),real=double(point["real"]),imaginary=double(point["imaginary"]);
   if(!std::isfinite(frequency)||frequency<=previous||!std::isfinite(real)||!std::isfinite(imaginary)){error=juce::String::fromUTF8("Datos TF no válidos");return {};}previous=frequency;
  }
  auto next=data;Node n;n.id=uid();n.parent=data.active;n.kind=Kind::SnapshotTF;n.name=name.trim().isEmpty()?"Snapshot TF demo":name.trim();n.colour=colour;n.diskName=n.id+".json";n.method="synthetic-demo";n.origin="original-demo";n.legacyKey=legacyKey;next.nodes.push_back(n);
  auto stored=payload.clone();if(!adopting){stored.getDynamicObject()->setProperty("traceRole","individual");stored.getDynamicObject()->setProperty("measurementId",n.id);stored.getDynamicObject()->setProperty("libraryId",data.id);}
  auto staged=root.getChildFile("Pendientes").getChildFile(n.diskName);if(!staged.getParentDirectory().createDirectory().wasOk()||!staged.replaceWithText(adopting?juce::File(legacyKey).loadFileAsString():juce::JSON::toString(stored))){error="No se pudo escribir la captura";return {};}
  return transaction(next,{{staged,file(next,n)}})?n.id:juce::String{};
 }
 int adoptLegacy(const juce::File& folder){
  if(!ready())return 0;int count=0;
  for(const auto& f:folder.findChildFiles(juce::File::findFiles,false,"*.json")){
   auto key=f.getFullPathName();bool found=false;for(const auto& n:data.nodes)if(n.legacyKey==key)found=true;if(found)continue;
   auto v=juce::JSON::parse(f);if(!v.isObject()||!bool(v["synthetic"])||!v["samples"].isArray())continue;
   // Adoption is format-limited to our existing demo snapshots, never .trf/.srf/WAV.
   auto id=capture(v,v["name"].toString(),v["colour"].toString(),key,true);
   if(id.isNotEmpty())++count;
  }return count;
 }
 std::optional<Trace> readTrace(const juce::String& id) const {
  auto* n=find(id);if(!n||n->kind!=Kind::SnapshotTF)return std::nullopt;auto v=juce::JSON::parse(dataFile(id));if(!v.isObject()||!v["samples"].isArray())return std::nullopt;
  Trace trace;trace.node=*n;trace.metadata=v;double last=0;
  for(const auto& p:*v["samples"].getArray()){
   if(!p.isObject()||!numeric(p["frequencyHz"])||!numeric(p["real"])||!numeric(p["imaginary"]))return std::nullopt;
   Sample s;s.frequency=double(p["frequencyHz"]);s.value={double(p["real"]),double(p["imaginary"])};
   if(!std::isfinite(s.frequency)||s.frequency<=last)return std::nullopt;last=s.frequency;
   if(numeric(p["coherence"])&&std::isfinite(double(p["coherence"]))&&double(p["coherence"])>=0&&double(p["coherence"])<=1)s.coherence=double(p["coherence"]);
   trace.samples.push_back(s);
  }return trace.samples.size()>1?std::optional<Trace>(std::move(trace)):std::nullopt;
 }
 static std::optional<Sample> interpolate(const Trace& trace,double frequency){
  if(!std::isfinite(frequency)||trace.samples.empty()||frequency<trace.samples.front().frequency||frequency>trace.samples.back().frequency)return std::nullopt;
  auto next=std::lower_bound(trace.samples.begin(),trace.samples.end(),frequency,[](const Sample& p,double f){return p.frequency<f;});
  Sample out;
  if(next==trace.samples.begin()||next->frequency<=frequency)out=*next;
  else{auto before=next-1;double t=(frequency-before->frequency)/(next->frequency-before->frequency);out.frequency=frequency;out.value=before->value+(next->value-before->value)*t;if(before->coherence&&next->coherence)out.coherence=std::min(*before->coherence,*next->coherence);}
  if(!std::isfinite(out.value.real())||!std::isfinite(out.value.imag())||std::abs(out.value)<=1.e-15)return std::nullopt;return out;
 }
};
