#include <parser.h>

#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <cstring>

std::string help(R"%(astdump [-i] [-t] <filename>
  filename: name of the file to dump the AST of.
  -i: interactive mode. Overrides filename.
  -t: tokenize instead of parse.)%");

using namespace lang::parser;

void tokenize(State& s)
{
  std::pair<State::Token, std::string> tkn;
  while (s && (tkn = s.token()).first != State::UNKNOWN && tkn.first != State::EOI)
  {
    std::cout << State::token_to_string(tkn.first) << ":" << tkn.second << std::endl;
  }
}

void parse(State& s)
{
  try
  {
    File f;
    f.parse(s);
    std::cout << f.print() << std::endl;
  }
  catch (std::runtime_error& e)
  {
    std::cout << e.what() << std::endl;
  }
}

void interactive(bool tknize)
{
  std::string line;
  std::cout << "> ";
  while (std::cin >> line)
  {
    if (line.compare("quit") == 0)
    {
      break;
    }
    State s{State::from_string(line)};
    s.filename = "(console)";
    if (tknize)
    {
      tokenize(s);
    }
    else
    {
      parse(s);
    }
    
    std::cout << "> ";
  }
}

State from_file(const std::string& fname)
{
  State state(lang::parser::State::from_file(fname));

  if (!state)
  {
    std::cerr << "Empty or nonexistent file at " << fname << std::endl;
    exit(1);
  }

  return state;
}

State from_stdin()
{
  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string input(begin, end);

  State s{State::from_string(input)};
  s.filename = "(stdin)";

  return s;
}

int main(int argc, char **argv)
{
  if (argc > 3)
  {
    std::cerr << "Bad number of arguments." << std::endl
      << help << std::endl;
    return 1;
  }

  if (argc == 1)
  {
    return from_stdin();
  }
  else
  {
    bool interact{false};
    bool tknize{false};
    std::string fname;
    for (int i = 1; i < argc; i++)
    {
      size_t len = strlen(argv[i]);
      if (len == 2 && strncmp("-i", argv[i], 2) == 0)
      {
        interact = true;
      }
      else if (len == 2 && strncmp("-t", argv[i], 2) == 0)
      {
        tknize = true;
      }
      else
      {
        fname = argv[i];
      }
    }

    State s;
    if (!interact && fname.size() == 0)
    {
      s = from_stdin();
    }
    else if (fname.size() > 0)
    {
      s = from_file(fname);
    }

    if (interact)
    {
      interactive(tknize);
    }
    else if (tknize)
    {
      tokenize(s);
    }
    else
    {
      parse(s);
    }
  }

  return 0;
}