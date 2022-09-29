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

#include <base64.h>

#include <png++/png.hpp>

using namespace std;
namespace fs = std::filesystem;

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


string convert_b64 (string& b64, png_uint_32 size, std::unique_ptr<ofstream>& olog) {

    std::istringstream mb;
    std::stringstream thumb;

    png::image<png::rgb_pixel>image;
    png_uint_32 pixel;

    mb.str(base64_decode(b64));
    image.read_stream(mb);
    if ((image.get_height()!=size)||(image.get_width() !=size)) {
        log(olog,to_string(size)+"x"+to_string(size)+ "thumbnail size is "+to_string(image.get_width())+"x"+to_string(image.get_height()));
        return "";
    }
    for (png_uint_32 y=0; y<size; y++) {
        for (png_uint_32 x=0; x<size; x++) {
//        ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
            pixel  =  image[y][x].blue >> 3;
            pixel |= (image[y][x].green & 0xfc)<<3;
            pixel |= (image[y][x].red   & 0xf8)<<8;
            thumb << std::setfill ('0') << std::setw(2) << std::hex << int(pixel & 0xff);
            thumb << std::setfill ('0') << std::setw(2) << std::hex << int((pixel>>8) & 0xff);

        }
        thumb << "\rM10086 ;";
    }
    return thumb.str();
}

void convert(string fname, string bname, int flog) {

    png::image<png::rgb_pixel>image;


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
