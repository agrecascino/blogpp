#include <iostream>
#include <libnavajo/WebServer.hh>
#include <libnavajo/DynamicPage.hh>
#include <libnavajo/DynamicRepository.hh>
#include <vector>
#include <boost/spirit/include/qi.hpp>
#include <fstream>
#include <functional>
#include <boost/bind.hpp>

class BlogPage : public DynamicPage {
public:
    BlogPage(std::string t, std::string p, std::string fname) {
        page = p;
        title = t;
        filename = fname;
    }

    bool getPage(HttpRequest *request, HttpResponse *response) {
        std::string returnedpage;
        returnedpage += "<html>\n";
        returnedpage += "  <head>\n";
        returnedpage += "    <title>" + title + "</title>\n";
        returnedpage += "    <h1>" + title + "</h1>";
        returnedpage += "  </head>\n";
        returnedpage += "  <body>\n";
        returnedpage += page;
        returnedpage += "  </body>\n";
        returnedpage += "  </html>";
        return fromString(returnedpage, response);
    }

    std::string get_fname() {
        return filename;
    }

    std::string get_title() {
        return title;
    }

    private:
    std::string filename;
    std::string title;
    std::string page;
};
struct PageParser {
    void filename_semantic(std::vector<char> sv){
        std::string s(sv.begin(), sv.end());
        if(!page_status) {
            std::cout << "ERROR: Filename stated out of document! Exiting..." << std::endl;
            exit(-1);
        }
        if(!filename.empty()) {
            std::cout << "ERROR: Filename stated two times in same document! Exiting..." << std::endl;
            exit(-1);
        }

        if(s == "index.html") {
            std::cout << "ERROR: Would replace index! Exiting..." << std::endl;
            exit(-1);
        }
        filename = s;
    }

    void title_semantic(std::vector<char> sv){
        std::string s(sv.begin(), sv.end());
        if(!page_status) {
            std::cout << "ERROR: Title stated out of document! Exiting..." << std::endl;
            exit(-1);
        }
        if(!title.empty()) {
            std::cout << "ERROR: Title stated two times in same document! Exiting..." << std::endl;
            exit(-1);
        }
        if(s == "index") {
            std::cout << "ERROR: Would replace index! Exiting..." << std::endl;
            exit(-1);
        }
        title = s;
    }

    void document_semantic() {
        if(!page_status) {
            page_status = true;
            return;
        }
        if(filename.empty()) {
            filename = "/" + title + ".html";
            std::replace(filename.begin(), filename.end(), ' ', '_');
        }
        pages.push_back(BlogPage(title, page, filename));
        page_status = false;
    }

    void line_semantic(std::vector<char> sv){
        std::string s(sv.begin(), sv.end());
        page += s + "<br>";
    }

    void ParsePagesFromDisk(std::string filename) {
        using boost::spirit::qi::phrase_parse;
        using boost::spirit::qi::_1;
        using boost::spirit::qi::lexeme;
        using boost::spirit::qi::char_;
        using boost::spirit::qi::unused_type;
        using boost::spirit::qi::ascii::space;
        std::fstream f(filename);
        lines.push_back(std::string());
        while(getline(f, lines.back())) {
            bool r = phrase_parse(lines.back().begin(), lines.back().end(),
                                  (
                                      lexeme["+++"][boost::bind(&PageParser::document_semantic, this)]
                    | (lexeme["title: "] >> lexeme[+(char_)][boost::bind(&PageParser::title_semantic, this, boost::placeholders::_1)])
                    | (lexeme["filename: "] >> lexeme[+(char_)][boost::bind(&PageParser::filename_semantic, this, boost::placeholders::_1)])
                    |  (lexeme[+(char_)][boost::bind(&PageParser::line_semantic, this, boost::placeholders::_1)])
                    ), boost::spirit::qi::blank);
            if(!r) {
                std::cout << "Parsing failed! Exiting..." << std::endl;
            }
        }
    }
    std::vector<BlogPage> pages;
private:
    std::string filename;
    bool page_status = false;
    std::vector<std::string> lines;
    std::string page;
    std::string title;
};

class IndexPage : public DynamicPage {
public:
    IndexPage(std::vector<BlogPage> &pages) {
        index = "<html>\n"
                "  <head>\n"
                "    <title>Index</title>\n"
                "    <h1>Index</h1>"
                "  </head>\n"
                "  <body>\n";
        for(BlogPage &p : pages) {
            index += "  <a href=\"" + p.get_fname() + "\">"
                  + p.get_title() + "</a>";
        }
        index += "  </body>";
        index += "</html>";
    }

    bool getPage(HttpRequest *request, HttpResponse *response) {
        return fromString(index, response);
    }

public:
    std::string index;
};

int main(int argc, char *argv[])
{
    std::string bfile = "blog.dat";
    if(argc > 1) {
        bfile = argv[1];
    }
    PageParser par;
    par.ParsePagesFromDisk(bfile);
    WebServer w;
    w.setServerPort(8088);
    DynamicRepository r;
    for(BlogPage &page : par.pages) {
        r.add(page.get_fname(), &page);
    }
    IndexPage index(par.pages);
    r.add("/index.html", &index);
    w.addRepository(&r);
    w.startService();
    w.wait();
    return 0;
}
