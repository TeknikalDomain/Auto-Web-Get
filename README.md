# Auto Web Get
Also known as massively parallelized curl
Because downloading files manually was too hard.

Usage: `awg <address list>` or `awg -f <file>`
where `file` is a list of URLs, one per line

awg will launch one thread per file to download as many as possible in parallel.
Actual download speeds will depend on your hardware, network connection, and the server.

command used to compile: `g++ main.cpp -Wall -Wextra -std=c++14 -lcurl -lcurlpp -pedantic -pthread -D_GLIBCXX_USE_CXX11_ABI=0 -lm -o awg -Ofast`
