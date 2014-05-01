#include "quit.hpp"

void wait_for_quit(bool display_quit_string, std::string quit)
{
    std::transform(quit.begin(), quit.end(), quit.begin(), tolower);
    
    if (display_quit_string)
    {
        std::cout << "Type: '" << quit << "' to quit" << std::endl;
    }

    std::string str;
    while (true)
    {
        std::getline(std::cin, str);
        std::transform(str.begin(), str.end(), str.begin(), tolower);
        if (str == quit)
        {
            break;
        }
    }
}

