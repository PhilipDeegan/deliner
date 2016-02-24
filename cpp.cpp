/**
Copyright (c) 2016, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "kul/io.hpp"
#include "kul/signal.hpp"

namespace deliner{
class Exception : public kul::Exception{
    public:
        Exception(const char*f, const uint16_t& l, const std::string& s) : kul::Exception(f, l, s){}
};

class Pattern{
    private:
        const std::string s;
        std::string st;
    public:
        Pattern(const std::string& s) : s(s){
            if(s.find("*") != std::string::npos) st = s.substr(0, s.find("*"));
        }
        const std::string& start() const { return st; }
        std::string end() const {
            if(s != st) return s.substr(st.size() + 1);
            return "";            
        }
        const std::string& str() const { return s; }
};

}

int main(int argc, char* argv[]) {
    kul::Signal sig;

    const char* DIR = "dir";
    const char* FILE = "file";
    const char* PATTERN = "patter";
    try{
        using namespace kul::cli;

        std::vector<Arg> argV { Arg('d', DIR,  ArgType::STRING),
                                Arg('f', FILE, ArgType::STRING),
                                Arg('p', PATTERN, ArgType::STRING),
                                Arg('r', "recurse")
                              };
        std::vector<Cmd> cmdV;

        Args args(cmdV, argV);
        try{
            args.process(argc, argv);
        }catch(const kul::cli::ArgNotFoundException& e){
            KERR << e.stack();
            KEXIT(0, "");
        }

        if(args.has(DIR)) {
            kul::Dir d(args.get(DIR));
            if(!d) KEXCEPT(deliner::Exception, "Directory does not exist: " + d.path());
            kul::env::CWD(d);
        }
        kul::File conf(".deliner");
        if(conf && (args.empty() || (args.size() == 1 && args.has(DIR)))){
            kul::io::Reader reader(conf);
            const std::string* s = reader.readLine();
            if(s && s->substr(0, 3) == "#! "){
                std::string line(s->substr(3));
                if(!line.empty()){
                    std::vector<std::string> lineArgs(kul::cli::asArgs(line));
                    std::vector<char*> lineV;
                    for(size_t i = 0; i < lineArgs.size(); i++) lineV.push_back(&lineArgs[i][0]);
                    return main(lineArgs.size(), &lineV[0]);
                }
            }
        }
        std::vector<std::string> types;
        if(!args.has(FILE)) KEXCEPT(deliner::Exception, "No file type specified e.g. -f cpp,hpp");
        kul::String::split(args.get(FILE), ',', types);
        kul::hash::set::String fts;
        for(const auto& s : types) fts.insert(s);

        std::vector<deliner::Pattern> pats;
        if(args.has(PATTERN)) pats.push_back(deliner::Pattern(args.get(PATTERN)));
        if(conf){
            kul::io::Reader r(conf);
            const std::string* l = 0;
            while((l = r.readLine())){
                if(l->size() == 0) continue;
                if((*l)[0] == '#') continue;
                pats.push_back(deliner::Pattern(*l));
            }
        }
        for(const auto& p : pats){ 
            if(p.str()[0] == '*') KEXCEPT(deliner::Exception, "Pattern may not start with *");
            if(p.str()[p.str().size() - 1] == '*') KEXCEPT(deliner::Exception, "Pattern may not end with *");
            if(std::count(p.str().begin(), p.str().end(), '*') > 1) KEXCEPT(deliner::Exception, "Pattern may not contains more than one *");
        }
        for(const auto file : conf.dir().files(1)){
            const std::string f = file.name();
            if(f.rfind('.') != std::string::npos){
                const std::string& ft(f.substr(f.rfind('.') + 1));
                if(fts.count(ft)){
                    kul::File tmp(file.real()+".tmp");
                    kul::io::Writer w(tmp);
                    kul::io::Reader r(file);
                    std::stringstream ss;
                    bool y = 1;
                    const std::string* l = r.readLine();
                    const deliner::Pattern* pat = 0;
                    if(l){
                        w << *l;
                        while((l = r.readLine())){
                            if(l->size() == 0){
                                w << kul::os::EOL();
                                continue;
                            } 
                            y = 1;
                            ss << *l;
                            std::string tr(*l);
                            kul::String::trim(tr);
                            if(pat){
                                ss.str(std::string());
                                w << kul::os::EOL();
                                const std::string& e(pat->end());
                                if(tr.rfind(e) == tr.size() - e.size()){
                                    pat = 0;
                                    y = 0;
                                    continue;
                                }
                            }
                            else
                                for(const auto& p : pats){
                                    if(p.start().size() > 0){
                                        if(tr.find(p.start()) == 0){
                                            w << kul::os::EOL();
                                            y = 0;
                                            ss.str(std::string());
                                            const std::string& e(p.end());
                                            if(tr.rfind(e) != tr.size() - e.size()) pat = &p;
                                            break;
                                        }
                                    }else
                                    if(tr == p.str()){
                                        w << kul::os::EOL();
                                        ss.str(std::string());
                                        y = 0;
                                        break;
                                    }
                                }
                            if(y){
                                w << kul::os::EOL() << ss.str();
                                ss.str(std::string());
                            }
                        }
                    }
                    file.rm();
                    tmp.mv(file);
                }
            }
        }
    }
    catch(const kul::Exception& e){ KERR << e.stack(); return 2;}
    catch(const std::exception& e){ KERR << e.what(); return 3;}
    return 0;
}
