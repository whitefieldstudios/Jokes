// MIT License
//
// Copyright (c) 2026 Code Life
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Jokes
//     by White Field Studios
//     command line version using modern C++
//     version 1.1 - 2/06/2026
//     version 2.0 - 3/23/2026 - no --color is necessary since color is now on by default; also using jokes_data.h for jokes
// Description: A collection of jokes via command line.

#include <string>
#include <iostream>
#include <random>
#include <algorithm>
#include <numeric> // Required for std::iota
#include "jokes_data.h" // Contains the vector<Joke> jokes with all the joke data

using namespace std;

const string VERSION = "2.0";

void outputJoke(const Joke& joke, int jokeNumber, bool separateLines = false, bool colored = false, bool showNumber = false, bool showType = false) {
    // ANSI color codes
    string cyanColor = "\033[36m";
    string greenColor = "\033[32m";
    string resetColor = "\033[0m";     // Reset to default
    
    if (showNumber) {
        cout << "[" << jokeNumber << "]";
        if (!showType) {
            cout << " ";
        }
    }
    if (showType) {
        cout << "[" << joke.type << "] ";
    }
    if (!colored) { // simple way to disable colors if not wanted
        cyanColor = "";
        greenColor = "";
        resetColor = "";
    }
    if (separateLines) {
        cout << cyanColor << (joke.type == "knock-knock" ? "Knock knock!\nWho's there?\n" : "")
                << joke.setup << (joke.type == "knock-knock" ? "\n" + joke.setup + " who?" : "") << resetColor << "\n"
                << greenColor << "\n" << joke.punchline << resetColor << "\n\n";
    } else {
        cout << cyanColor << (joke.type == "knock-knock" ? "Knock knock! Who's there? " : "")
                << joke.setup << (joke.type == "knock-knock" ? " " + joke.setup + " who?" : "") << resetColor
                << greenColor << ": " << joke.punchline << resetColor << "\n";
    }
}

void displayHelp() {
    cout << "Jokes - A collection of jokes\n\n";
    cout << "Usage: ./jokes [OPTIONS]\n\n";
    cout << "Options:\n";
    cout << "  (none)                Display a single random joke (default)\n\n";
    cout << "  -a, --all             Display all jokes in shuffled order\n";
    cout << "  -1, --oneline         Display joke setup and punchline on one line with a colon separator\n";
    cout << "  -nc, --nocolor        Disable colored output (color is on by default)\n";
    cout << "  -sn, --shownumber     Display the number of the joke\n";
    cout << "  -st, --showtype       Display the type of the joke\n";
    cout << "  -t, --type xxx        Filter by joke type: dad, knock-knock, pun, math, tech, work, aging, all\n";
    cout << "  -p, --picknumber xxx  Display joke by number, supply number after\n";
    cout << "  -v, --version         Display the version number\n";
    cout << "  -h, --help            Display this help message\n";
    cout << "\nExamples:\n";
    cout << "  ./jokes                       - Display one random joke\n";
    cout << "  ./jokes -a                    - Display all jokes shuffled\n";
    cout << "  ./jokes --nocolor             - Display one random joke without color\n";
    cout << "  ./jokes -a -1                 - Display all jokes on one line (color is default)\n";
    cout << "  ./jokes -p 2                  - Display joke #2\n";
    cout << "  ./jokes -t dad                - Display a random dad joke\n";
    cout << "  ./jokes -a -t tech            - Display all tech jokes shuffled\n";
}

int main(int argc, char* argv[]) {
    // Parse command line options
    bool together = false;      // -1 flag (default is separate lines)
    bool showAll = false;       // -a flag (default is single random)
    bool colored = true;
    bool filterType = false;
    bool showNumber = false;    // -sn flag (default is false)
    bool showType = false;      // -st flag (default is false)
    bool pickJokeByNumber = false;
    int jokeNumber = 0;
    string type = "all"; // default type filter is "all" (no filtering)

    try {
        for (int i = 1; i < argc; i++) {
            string arg = argv[i];

            if (arg == "-1" || arg == "--oneline") {
                together = true;
            } else if (arg == "-a" || arg == "--all") {
                showAll = true;
            } else if (arg == "-nc" || arg == "--nocolor") {
                colored = false;
            } else if (arg == "-sn" || arg == "--shownumber") {
                showNumber = true;
            } else if (arg == "-st" || arg == "--showtype") {
                showType = true;
            } else if (arg == "-t" || arg == "--type") {
                if (i + 1 < argc) {
                    filterType = true;
                    type = argv[++i]; // Skip to next arg
                    if (type != "dad" && type != "knock-knock" && type != "pun" && type != "math" && type != "tech" && type != "work" && type != "aging" && type != "all") {
                        cerr << "Invalid joke type specified. Valid types are: dad, knock-knock, tech, pun, math, work, aging, all.\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --type option requires an argument.\n";
                    return 1;
                }
            } else if (arg == "-p" || arg == "--pick") {
                if (i + 1 < argc) {
                    pickJokeByNumber = true;
                    try {
                        jokeNumber = stoi(argv[++i]); // Skip to next arg
                    } catch (...) {
                        cerr << "Error: --pick requires a valid number.\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --pick option requires a number.\n";
                    return 1;
                }
            } else if (arg == "-v" || arg == "--version") {
                cout << "Jokes version " << VERSION << "\n";
                return 0;
            } else if (arg == "-h" || arg == "--help") {
                displayHelp();
                return 0;
            }
        }
    } catch (...) {
        cerr << "Error parsing command line arguments.\n";
        return 1;
    }   

    bool separateLines = !together;
    
    if (showAll) {
        // Display all jokes (shuffled)
        cout << "Jokes - A collection of jokes\n";
        cout << "Brought to you by Code Life\n\n";
        
        // 1. Create a list of indices: 0, 1, 2, ... N
        vector<int> indices(jokes.size());
        iota(indices.begin(), indices.end(), 0); 
        
        // 2. Shuffle the indices
        random_device rd;
        mt19937 gen(rd());
        shuffle(indices.begin(), indices.end(), gen);
        
        int jokeCount = 0;
        for (int i : indices) {
            // Apply Type Filter
            if (filterType && type != "all" && jokes[i].type != type) {
                continue; 
            }
            // Pass (i + 1) so it displays as "Joke [1]" not "Joke [0]"
            outputJoke(jokes[i], i + 1, separateLines, colored, showNumber, showType);
            jokeCount++;
        }
        
        cout << "\nTotal jokes: " << jokeCount << "\n";
    } else {
        // Logic for displaying a SINGLE joke (Random or Specific)
        int selectedIndex = -1;

        if (pickJokeByNumber) {
            // 1. User specified a specific number
            // Vector is 0-indexed, so we subtract 1 from user input
            if (jokeNumber >= 1 && jokeNumber <= (int)jokes.size()) {
                selectedIndex = jokeNumber - 1;
            } else {
                cerr << "Error: Joke #" << jokeNumber << " does not exist. (Range: 1-" << jokes.size() << ")\n";
                return 1;
            }
        } else {
            // 2. Random selection (must respect filters!)
            vector<int> validIndices;
            
            for (size_t i = 0; i < jokes.size(); ++i) {
                // Check if this joke matches the requested type
                if (!filterType || type == "all" || jokes[i].type == type) {
                    validIndices.push_back(i);
                }
            }

            if (validIndices.empty()) {
                cerr << "No jokes found matching type: " << type << "\n";
                return 1;
            }

            // Securely pick a random index from the VALID indices only
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(0, validIndices.size() - 1);
            
            selectedIndex = validIndices[dis(gen)];
        }

        // Output the final selected joke
        outputJoke(jokes[selectedIndex], selectedIndex + 1, separateLines, colored, showNumber, showType);
    }
    return 0;
}
