#include <libxml++/parsers/textreader.h>
#include <fmt/format.h>
#include <iostream>
#include <string>
int main(int argc,char**argv) {
 if(argc<2)return 2;
 long strings=0;
 for(int iteration=0;iteration<80;++iteration) {
  for(int f=1;f<argc;++f) {
   xmlpp::TextReader reader{std::string(argv[f])};
   while(reader.read()) {
    std::string name=reader.get_name();
    if(reader.has_attributes()) {
     while(reader.move_to_next_attribute()) {
      std::string key=reader.get_name(),value=reader.get_value();
      std::string formatted=fmt::format("{}:{}={}",name,key,value);
      if(formatted.empty())return 3;
      strings+=3;
     }
     reader.move_to_element();
    }
    ++strings;
   }
  }
 }
 std::cout<<"PASS: "<<strings<<" XML/string operations with rebuilt C++ wrapper and header-only fmt under AddressSanitizer\n";
}
