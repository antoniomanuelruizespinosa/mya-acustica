#include "../src/AppDataLocation.h"
#include <iostream>

int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Uso: mya_migrate_data <carpeta-antigua>\n";return 2;}
 const auto source=juce::File(juce::String::fromUTF8(argv[1]));
 const auto destination=MyaAppData::directory();
 switch(MyaAppData::migrateFrom(source,destination)){
  case MyaAppData::MigrationResult::Migrated:std::cout<<"Datos copiados a la carpeta de usuario. El origen se conserva.\n";return 0;
  case MyaAppData::MigrationResult::DestinationExists:std::cerr<<"El destino ya existe; no se ha sobrescrito ni copiado nada.\n";return 3;
  case MyaAppData::MigrationResult::NoSourceData:std::cerr<<"No hay datos conocidos en la carpeta antigua.\n";return 4;
  case MyaAppData::MigrationResult::Failed:std::cerr<<"La migración falló; el origen se conserva.\n";return 1;
 }
 return 1;
}
