#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <mutex>
#include <cstring>
#include <chrono>
using namespace std;
using namespace std::literals::string_literals;

unsigned ln = 1;
auto Line(int l) { int m=l-ln; ln=l; return "\r"+(m<0?"\33["+std::to_string(-m)+'A':std::string(m,'\n')); }
mutex print_lock;

size_t Download(const string& url, const string& filename, unsigned line)
{
    ofstream outfile(filename);
    size_t written = 0;

    cURLpp::Easy req;
    req.setOpt(cURLpp::Options::Url(url));
    req.setOpt(cURLpp::Options::NoProgress(false));
    req.setOpt(cURLpp::Options::FollowLocation(true));
    req.setOpt(cURLpp::Options::ProgressFunction([&](size_t total, size_t done, auto...)
    {
        lock_guard<mutex> l(print_lock);
        cout << Line(line) << filename << ": " << done << " / " << total << " (" << int(total ? done*100./total : 0) << "%)" << std::flush;
        return 0;
    }));
    req.setOpt(cURLpp::Options::WriteFunction([&](const char* p, size_t size, size_t nmemb)
    {
        outfile.write(p, size*nmemb);
        written += size*nmemb;
        return size*nmemb;
    }));
    req.perform();
    return written;
}

template<typename Out>
void split(const string &s, char delim, Out result)
{
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim))
    {
        *(result++) = item;
    }
}

vector<string> split(const string &s, char delim)
{
    vector<string> items;
    split(s, delim, back_inserter(items));
    return items;
}

#include <thread>
#include <atomic>

int main(int argc, char** argv)
{
    if (argc == 1)
    {
        cerr << "Error: No addresses provided." << endl;
        cout << "Usage: awg (<address>...|-f <file>)" << endl;
        return 1;
    }

    if (strcmp(argv[1], "-?") || strcmp(argv[1], "--help"))
    {
        cout << "Usage: awg (<address>...|-f <file>)" << endl;
        return 1;
    }

    int addrcount = 0;
    vector<string> addresses;
    vector<string> filenames;
    vector<thread> downloaders;
    atomic<size_t> total {0};


    if (strcmp(argv[1], "-f") == 0)
    {
        if (argc == 2)
        {
            cerr << "Error: No file specified." << endl;
            return 1;
        }
        ifstream addrfile(argv[2]);
        string aline;
        while (getline(addrfile, aline))
        {
            addrcount++;
            addresses.push_back(aline);
        }
        addrfile.close();
    }
    else
    {
        addrcount = argc - 1;
        for(int a = 1;a<argc;a++) addresses.push_back(argv[a]);
    }

    for (int addr = 0; addr < addrcount; addr++)
    {
        vector<string> chopped_url = split(addresses.at(addr), '/');
        filenames.push_back(chopped_url.back());
    }



    cURLpp::initialize();
    unsigned line = 1;

    cout << "Downloading " << addrcount << " files..." << endl << endl;

    auto start = chrono::steady_clock::now();
    for(int addr = 0; addr < addrcount; addr++)
    {

        downloaders.emplace_back([address=addresses.at(addr), l=line++, &total, &filenames, addr]
        {
            total += Download(address, filenames.at(addr), l);
        });
    }

    for (auto& dl: downloaders) dl.join();

    auto duration = chrono::steady_clock::now() - start;
    cout << Line(line) << to_string(total) << " bytes received, " << duration.count() << " seconds." << endl;
    cout << "Average: " << to_string(total / addrcount) << " bytes per file." << endl;
    cout << "Average speed: " << total / duration.count() << "bytes per second." << endl;
}
