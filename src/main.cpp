/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <iostream>
#include <bits/stdc++.h>
#include <fstream>
#include <istream>
#include <ostream>
#include <cstring>
#include <string>
#include <filesystem>
#include <memory>
#include <regex>

int decodePNG(std::vector<unsigned char>& , unsigned long& , unsigned long& , const unsigned char* , size_t , bool );

using namespace std;
namespace fs = std::filesystem;

unsigned static int b64val( char c) {
    switch(c) {
	case 'A' ... 'Z'	: return c - 'A';
        case 'a' ... 'z'	: return c - 'a' + 0b011010;
        case '0' ... '9'	: return c - '0' + 0b110100;
	case '+'		: return 0b111110;
        case '/'		: return 0b111111;
	default			: return 0b000000;
    }
}

//  000000.11 1111.2222 22.333333

string b64decode(const string& b64 ) {
    
    stringstream bin;
    unsigned int tmp[4];
    char dec;
    auto ptr=b64.begin();
	
	bin.clear();
    while(ptr!=b64.end()) {
		tmp[0]=b64val(*ptr); ptr++;
		tmp[1]=b64val(*ptr); ptr++;
			dec=(tmp[0]<<2 | tmp[1]>>4)&0xff;
			bin << dec;
		if (*ptr != '=') {
			tmp[2]=b64val(*ptr); ptr++;
			dec = (tmp[1]<<4 | tmp[2]>>2)&0xff;
			bin << dec;
		} else break;
		if (*ptr != '=') {
			tmp[3]=b64val(*ptr); ptr++;
			dec = (tmp[2]<<6 | tmp[3])&0xff;
			bin << dec;
		} else break;
    }
	return bin.str();
}

void inline log(std::unique_ptr<ofstream>& olog, string msg) {
    if (olog->is_open()) {
        *olog << msg << endl;
    }
}

void scan (std::unique_ptr<fstream>& gc, string size, string& thumbnail ) {
    string line;
    string tag = ";[ \\t]*thumbnail[ \\t]+begin[ \\t]+"+size+"[ x]"+size+"[ \\t]+";
    std::regex threg(tag);

    gc->seekg(0);
    while ( getline (*gc,line) && (!regex_search(line,threg)));

    if (string::npos == line.find("thumbnail")) { thumbnail=""; return; }
    while ( getline (*gc,line) && (string::npos == line.find("thumbnail"))){
        thumbnail+=line.substr(2,line.length()-2);  // hardcoded... no good...
    }
}

////int decodePNG(std::vector<unsigned char>& , unsigned long& , unsigned long& , const unsigned char* , size_t , bool );

typedef struct rgba {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} rgba;


string convert_b64 (string& b64,unsigned int size, std::unique_ptr<ofstream>& olog) {

    string mb;
    std::stringstream thumb;
	vector<unsigned char> image;
	unsigned long w,h;
	rgba * pixels;
	int pixel;

    mb = b64decode(b64);
	decodePNG(image,w,h,reinterpret_cast<const unsigned char*>(mb.c_str()),mb.length(),true);

	pixels = reinterpret_cast<rgba*>(image.data());
	
    if ((h!=size)||(w!=size)) {
        log(olog,to_string(size)+"x"+to_string(size)+ "thumbnail size is "+to_string(w)+"x"+to_string(h));
        return "";
    }
    for (unsigned int y=0; y<size; y++) {
        for (unsigned int x=0; x<size; x++) {
            pixel  =  pixels[x+y*size].b >> 3;
            pixel |= (pixels[x+y*size].g & 0xfc)<<3;
            pixel |= (pixels[x+y*size].r & 0xf8)<<8;
            thumb << std::setfill ('0') << std::setw(2) << std::hex << int( pixel     & 0xff);
            thumb << std::setfill ('0') << std::setw(2) << std::hex << int((pixel>>8) & 0xff);

        }
        thumb << "\rM10086 ;";
    }
    return thumb.str();
}

void convert(string fname, string bname, int flog) {

    std::unique_ptr<fstream>  gcode(new fstream);
    std::unique_ptr<fstream>  gout (new fstream);
    std::unique_ptr<ofstream> olog (new ofstream);

    string th100 = "";
    string th200 = "";
    string thout;
    stringstream hack;

    if (flog) { olog->open(fname+".log",ios::out); } // file doesn't exists? so what. maybe log will ;)

    if (! fname.length())    { log(olog,"No filename given"); return;}
    if (! fs::exists(fname)) { log(olog,"File <"+fname+"> doesn't exists."); return; }

    fs::copy(fname,bname,fs::copy_options::overwrite_existing);
    gcode->open(bname,ios::in);
    if (! gcode->is_open()) { log(olog,"Cannot open file <"+bname+">."); return; }

    scan(gcode, "100", th100);
    scan(gcode, "200", th200);

    if (! th100.length()) { log(olog,"100x100 thumbnail not found"); return; }
    if (! th200.length()) { log(olog,"200x200 thumbnail not found"); return; }


    th100=convert_b64(th100,100,olog);
    if (!th100.length()) { return; }
    th200=convert_b64(th200,200,olog);
    if (!th200.length()) { return; }

    hack << endl; // ugly. works.
    
    thout  = "; Postprocessed by png2mks thumbnail converter"+hack.str()+"; https://github.com/slawkis/png2mks" +hack.str()+"; "+hack.str();
    thout += "; simage=100\n; gimage=200\n";
    thout += ";simage:"+th100+"\r;;gimage:"+th200+"\r\r;"+hack.str();

    gcode->seekg(0);
    gout->open(fname,ios::out);
    if (! gout->is_open()) { log(olog,"Cannot open file <"+fname+">."); return; }

    string tag = ";[ \\t]*thumbnail[ \\t]+begin[ \\t]\\d+[ x]\\d+[ \\t]+";
    std::regex threg(tag);

    *gout << thout;

    while ( getline (*gcode,thout) ) {
        if (regex_search(thout,threg)) {
            while (getline (*gcode,thout) && string::npos == thout.find("thumbnail"));
        }
        else { *gout << thout << endl; }
    }

    gcode->close();  // unnecessary, but...
    gout->close();
    olog->close();

}


int main(int argc, char** argv)
{
    int keep=0;
    int log=0;
    string fname="";
    string bname="";
    string parm;

     if (argc<2) {
         cout << endl << "\t" << argv[0] << "[--keep] [--log] filename.gcode" << endl;
         cout << "\t\t keep - keep tmp file as backup" << endl;
         cout << "\t\t log  - create log file" << endl;
         return 1;
     }

     for (auto i = 1; i < argc; i++) {
         parm=string(argv[i]);
         if (!parm.compare("--keep")) { keep=1; continue; }
         if (!parm.compare("--log" )) { log=1;  continue; }
         fname = parm;
         bname = fname+".bak";
     }
    convert(fname, bname, log);
    if (! keep) { fs::remove(bname); }

    return 0;
}
