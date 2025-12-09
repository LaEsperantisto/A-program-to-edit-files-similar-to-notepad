#include <ncurses.h>
#include <signal.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <array>
#include <algorithm>

#define CTRL(x) ((x) & 0x1f)


unsigned int spaces_in_tabs = 4;
unsigned int spaces_after_line_num = 5;
unsigned int min_spaces_after_line_num = 5;

unsigned int color_mode = 1;

std::vector<std::string> color_modes = {
    "plain text",
    "c++",
    "programming",
};

std::vector<std::string> keywords = {
    "int", "void", "if", "else", "return", "while", "for", "class", "struct",
    "do", "unsigned", "signed", "bool", "char", "continue", "break", "true",
    "false", "include", "define", "namespace", "using",
};


std::string paste_from_clipboard() {
    FILE *pipe = popen("xclip -selection clipboard -o", "r");
    if (!pipe) return "";

    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

void copy_to_clipboard(const std::string &text) {
    std::string cmd = "printf \"%s\" \"" + text + "\" | xclip -selection clipboard";
    system(cmd.c_str());
}

void backspace(int &x, int &y, std::vector<std::string> &document) {
    document[y].erase(x - 1, 1);
    x--;
}

void load_file(const std::string &filename, std::vector<std::string> &document) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        document.push_back(line);
    }
    if (document.empty()) document.push_back("");
}

void save_file(const std::string &filename, const std::vector<std::string> &document) {
    std::ofstream file(filename);
    for (const auto &line : document) {
        file << line << "\n";
    }
}

void option(const std::string &filename, const std::vector<std::string> &document) {

    while (true) {
        clear();
        mvprintw(0, 0, "Press Esc to exit options");
        
        mvprintw(2, 0, "Press + and - to increase and decrease the number of spaces in each tab");
        mvprintw(3, 0, "Current number of spaces per tab: %d", spaces_in_tabs);
        
        mvprintw(5, 0, "Press shift + and shift - to increase and decrease the number of spaces after each line number");
        mvprintw(6, 0, "Current number of spaces after line numbers: %d", spaces_after_line_num);

        printw("\n\nCurrent color mode is \"%s\"", color_modes.at(color_mode).c_str());
        printw("\nPress c to change color mode");

        if (has_colors() && COLORS < 256) printw("\n\nYour terminal does not support all 256 colors");
        else if (!has_colors()) printw("\n\nYour terminal does not support color");

        printw("\n\n");
        


        int c = getch();

        if (c == 27) { // Escape
            switch (color_mode) {
            case 1:
                keywords = {
                    "int", "void", "if", "else", "return", "while", "for", "class", "struct",
                    "do", "unsigned", "signed", "bool", "char", "continue", "break", "true",
                    "false", "include", "define", "namespace", "using",
                };
                break;
                
            default:
                keywords = {};
            }
            break;
        }

        else if (c == '=') {
            spaces_in_tabs++;
        }
        else if (c == '-') {
            if (spaces_in_tabs > 0) spaces_in_tabs--;
        }
        else if (c == '+') {
            spaces_after_line_num++;
        }
        else if (c == '_') {
            if (spaces_after_line_num > min_spaces_after_line_num) spaces_after_line_num--;
        }
        else if (c == 'c') {
            if (color_mode == color_modes.size()-1) color_mode = 0;
            else color_mode++;
        }
    }
}

void init() {
    
    initscr();
    
    if (has_colors() && COLORS >= 256) {
        start_color();
        use_default_colors();

        // Define color pairs using default background (-1)
        init_pair(1, COLOR_WHITE,  -1); // Normal text
        init_pair(2, COLOR_CYAN,   -1); // Line numbers
        init_pair(3, COLOR_YELLOW, -1); // Keywords
        init_pair(4, COLOR_GREEN,  -1); // Strings
        init_pair(5, COLOR_RED,    -1); // Errors/highlights
        init_pair(6, -1, COLOR_CYAN);   // Indent highlight block
        init_pair(7, COLOR_MAGENTA, -1);// Parenthesese, etc.
        init_pair(8, COLOR_BLUE,    -1);// Save
        init_pair(9, 230,           -1);// Comment
        init_pair(10, COLOR_BLUE,   -1);// Numbers
    }
    else if (has_colors()) {
        start_color();
        use_default_colors();

        // Define color pairs using default background (-1)
        init_pair(1, COLOR_WHITE,  -1); // Normal text
        init_pair(2, COLOR_CYAN,   -1); // Line numbers
        init_pair(3, COLOR_YELLOW, -1); // Keywords
        init_pair(4, COLOR_GREEN,  -1); // Strings
        init_pair(5, COLOR_RED,    -1); // Errors/highlights
        init_pair(6, -1, COLOR_CYAN);   // Indent highlight block
        init_pair(7, COLOR_MAGENTA, -1);// Parenthesese, etc.
        init_pair(8, COLOR_BLUE,    -1);// Save
        init_pair(9, COLOR_CYAN,    -1);// Comment
        init_pair(10, COLOR_BLUE,   -1);// Numbers
    }


    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);
    // mousemask(ALL_MOUSE_EVENTS, nullptr);
    
    // Unwanted Key-binds
    signal(SIGINT,  SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

void draw_line(int row, const std::string &line) {
    int x = spaces_after_line_num;

    std::string word;
    bool line_comment = false;
    char prev_c = '\0';
    char c = '\0';

    
    for (size_t i = 0; i < line.size(); i++) {
        prev_c = c;
        c = line[i];
        
        if (color_mode == 0) {
            attron(COLOR_PAIR(1));
            mvaddch(row, x++, c);
            attroff(COLOR_PAIR(1));
            continue;
        }
        
        if (line_comment) {
            attron(COLOR_PAIR(9));
            mvaddch(row, x++, c);
            attroff(COLOR_PAIR(9));
            continue;
        }

        if (color_mode != 0) {
            if (isalnum(c) || c == '_') {
                word += c;
                continue;
            }
            
            
            if (!word.empty() && word.at(0) >= '0' && word.at(0) <= '9') {
                attron(COLOR_PAIR(10));
                
                mvprintw(row, x, "%s", word.c_str());
                x += word.size();
                attroff(COLOR_PAIR(10));
                word.clear();
            }
        }
        
        if (color_mode != 2 && color_mode != 0) { // mode is a [programming] mode but not "programming" mode
            // If part of a word

        
            // If a word ended, print it:
            if (!word.empty()) {
                
                if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
                    attron(COLOR_PAIR(3)); // keyword color
                } else {
                    attron(COLOR_PAIR(1)); // normal text
                }
    
                mvprintw(row, x, "%s", word.c_str());
                x += word.size();
                attroff(COLOR_PAIR(3));
                attroff(COLOR_PAIR(1));
                word.clear();
            }
        }


        // Strings
        if (line[i] == '"') {
            // start string color
            attron(COLOR_PAIR(4));
            mvaddch(row, x++, '"');
            i++;
    
            // print until next quote
            while (i < line.size() && line[i] != '"') {
                mvaddch(row, x++, line[i]);
                i++;
            }
    
            // print closing quote if found
            if (i < line.size()) {
                mvaddch(row, x++, '"');
            }
    
            attroff(COLOR_PAIR(4));
            continue;
        }
        
        if (c == '/' && prev_c == '/') {
            attron(COLOR_PAIR(9));
            mvaddch(row, x-1, c);
            mvaddch(row, x++, c);
            attroff(COLOR_PAIR(9));
            line_comment = true;
            continue;
        }
        
        if (c == '(' || c == ')' || c == '{' ||
            c == '[' || c == ']' || c == '}') {
            attron(COLOR_PAIR(7));
            mvaddch(row, x++, c);
            continue;
        }


        // Normal character
        attron(COLOR_PAIR(1));
        mvaddch(row, x++, c);
        attroff(COLOR_PAIR(1));

        
    }

    if (!word.empty() && word.at(0) >= '0' && word.at(0) <= '9') {
        attron(COLOR_PAIR(10));

        mvprintw(row, x, "%s", word.c_str());
        x += word.size();
        attroff(COLOR_PAIR(10));
        word.clear();
    }
    else if (!word.empty()) {
                
        if (std::find(keywords.begin(), keywords.end(), word) != keywords.end()) {
            attron(COLOR_PAIR(3)); // keyword color
        } else {
            attron(COLOR_PAIR(1)); // normal text
        }

        mvprintw(row, x, "%s", word.c_str());
        x += word.size();
        attroff(COLOR_PAIR(3));
        word.clear();
    }
}



int main(int argc, char *argv[]) {

    copy_to_clipboard("\0");
    
    if (argc != 2) {
        std::cout << "There should be ONE argument (the name of the file to edit)\n";
        return 1;
    }

    system("stty -ixon"); // stops ctrl-s pause terminal

    std::string filename = argv[1];
    std::vector<std::string> document;
    load_file(filename, document);

    // Init ncurses

    init();

    int x = 0, y = 0;
    int scroll_offset = 0;
    bool saved = false;
    bool saved_currently = true;

    bool options = false;


    while (true) {

        if (!options) {
            clear();
    
            int screen_height = LINES - 1;
    				
    
            refresh();
    
            if (y < scroll_offset) {
                scroll_offset = y;
            }
            if (y >= scroll_offset + screen_height) {
                scroll_offset = y - screen_height + 1;
            }
    
            // Draw visible lines
            for (int row = 0; row < screen_height; row++) {
                int doc_index = row + scroll_offset;
                
                if (doc_index >= document.size()) break;

                
                attron(COLOR_PAIR(2));
                
                
                char ln_buf[16];
                snprintf(ln_buf, sizeof(ln_buf), "%4d", doc_index + 1);
                mvprintw(row, 0, "%s", ln_buf);
                
                attroff(COLOR_PAIR(2));

                // Print text after line number
                draw_line(row, document[doc_index]);
            }
    
    
            // Status bar
            if (saved) {
                attron(COLOR_PAIR(8));
                mvprintw(LINES - 1, 0, "Saved to '%s'", filename.c_str());
                saved = false;
                attroff(COLOR_PAIR(8));
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(LINES - 1, 0, "Ctrl+S = Save | Esc * 2 = Quit | Ctrl+O = Options | (%d,%d)", y, x);
                attroff(COLOR_PAIR(1));
            }
            
    
            // Place cursor
            move(y - scroll_offset, x + spaces_after_line_num);
            refresh();

            
            int c = getch();
            // mvprintw(LINES - 1, 0, "key=%d char=%c\n", c, c);
    
    		
            
            if (c == 27) { // detect ESC-based F2 in some terminals
                int c1 = getch();
                if (c1 == '[') {
                    int c2 = getch();
                    if (c2 == '1') {
                        int c3 = getch();   // ;
                        int c4 = getch();   // 5
                        int c5 = getch();   // A
                        if (c3 == ';' && c4 == '5' && c5 == 'A') {
                            // scroll_offset--;
                            mvprintw(LINES-1, 0, "CTRL+UP detected!");
                            continue;
                        }
                    }
                }

                if (c1 == 27) break;
                continue;
            }
    
            // ----- MOVEMENT -----
            if (c == KEY_UP) {
                if (y > 0) y--;
                if (x > document[y].size()) x = document[y].size();
            }
            else if (c == KEY_DOWN) {
                if (y < document.size() - 1) y++;
                if (x > document[y].size()) x = document[y].size();
            }
            else if (c == KEY_LEFT) {
                if (x > 0) x--;
                else if (y > 0) {
                    y--;
                    x = document[y].size();
                }
            }
            else if (c == KEY_RIGHT) {
                if (x < document[y].size()) x++;
                else if (y < document.size()-1) {
                    y++;
                    x = 0;
                }
            }
    
            // ----- EDITING -----
            else if (c == '\n') {
                saved_currently = false;
                std::string rest = document[y].substr(x);
                document[y] = document[y].substr(0, x);
                
                document.insert(document.begin() + y + 1, rest);
                y++;
                x = 0;
            }
    
            else if (c == CTRL('s')) {
                save_file(filename, document);
                saved = true;
                saved_currently = true;
            }
    
            else if (c == CTRL('o')) {
                options = true;
            }
            
            else if (c == KEY_BACKSPACE || c == 8) {
                saved_currently = false;
                if (x > 0) {
                    if (document[y][x - 1] == ' ') {
                        
                        for (int i = 0; i < spaces_in_tabs; i++) {
                            if (document[y][x - 1] != ' ') break;
                            backspace(x, y, document);
                        }
                        
                    } else {
                        backspace(x, y, document);
                    }
                    
                } else if (y > 0) {
                    int prev_len = document[y - 1].size();
                    document[y - 1] += document[y];
                    document.erase(document.begin() + y);
                    y--;
                    x = prev_len;
                }
            }
    
            else if (c == '\t') {
                saved_currently = false;
                for (int i = 0; i < spaces_in_tabs; i++) {
                    document[y].insert(document[y].begin() + x, ' ');
                    x++;
                }
            }

            else if (c == CTRL('/')) {
                document[y].insert(document[y].begin(), '/');
                document[y].insert(document[y].begin(), '/');
                document[y].insert(document[y].begin(), ' ');
            }
            
            else if (c >= 32 && c <= 126) {
                saved_currently = false;
                document[y].insert(document[y].begin() + x, (char)c);
                x++;
            }

            else if (c == 127) {
                
                while (isalnum(c)) {
                    backspace(x, y, document);
                }
                
            }


        } else {
            option(filename, document);
            options = false;
        }
    }
    
    clear();
    endwin();

    if (!saved_currently) {
            
        std::string response;
        do {
        	std::cout << "Do you want to save this unsaved file (Y/n)\n";
        	std::cin >> response;
        
        } while (response != "n" && response != "y");
        
        if (response == "y") save_file(filename, document);

    }
    
	
    return 0;
}
