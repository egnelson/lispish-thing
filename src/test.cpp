#include <parser.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>

using namespace lang::parser;

template <typename T>
bool test_parse(const std::string& name, const std::string& type, const std::string& input, const std::string& expected)
{
  State s{State::from_string(input)};
  s.filename = name;

  auto output{T::parse(s)};
  std::stringstream ss;
  ss << output.second;
  bool eq{ss.str().compare(expected) == 0};
  if (!eq)
  {
    std::cout << "Unequal; got '" << output.second << "', expected '" << expected << "'" << std::endl;
  }
  else
  {
    std::cout << "Equal; got '" << output.second << "'" << std::endl;
  }
  std::cout << "Test " << name << " of type " << type << ": " << (eq ? "pass" : "fail") << std::endl;
  return eq;
}

bool test_tokenize(const std::string& name, const std::string& input, const std::vector<std::string>& expected)
{
  State s{State::from_string(input)};
  s.filename = name;

  std::vector<std::pair<State::Token, std::string>> output;
  while (s)
  {
    output.push_back(s.token());
  }

  bool eq{true};
  for (size_t i = 0; i < output.size() || i < expected.size(); i++)
  {
    if (i < output.size() && i < expected.size())
    {
      if (expected[i].compare(output[i].second) == 0)
      {
        std::cout << i << ": Equal; " << State::token_to_string(output[i].first) << ":'" << output[i].second << "'." << std::endl;
      }
      else
      {
        eq = false;
        std::cout << i << ": Unequal; got " << State::token_to_string(output[i].first) << ":'" << output[i].second << "'"
         << "; expected '" << expected[i] << "'." << std::endl;
      }
    }
    else if (i < output.size())
    {
      eq = false;
      std::cout << i << ": Extra token in output: " << State::token_to_string(output[i].first) << ":'" << output[i].second << "'." << std::endl;
    }
    else
    {
      eq = false;
      std::cout << i << ": Missing token in output: '" << expected[i] << "'." << std::endl;
    }
  }

  std::cout << "Test " << name << ": " << (eq ? "pass" : "fail") << std::endl;

  return eq;
}

int main(int argc, char **argv)
{
  std::vector<std::vector<std::string>> tests;
  if (argc > 1)
  {
    std::ifstream f(argv[1]);
    if (f)
    {
      std::string data;
      f.seekg(0, std::ios::end);
      data.reserve(f.tellg());
      f.seekg(0, std::ios::beg);
      data.assign(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
      f.close();

      for (size_t line_last_i{0}, line_i{data.find('\n')};
           line_i != std::string::npos;
           line_last_i = line_i + 1, line_i = data.find('\n', line_last_i))
      {
        std::string line{data.substr(line_last_i, line_i - line_last_i)};

        std::vector<std::string> line_contents;
        char *item{strtok(line.data(), ",\n")};
        if (item)
        {
          do
          {
            line_contents.push_back(item);
          }
          while ((item = strtok(0, ",\n")));
        }

        tests.push_back(line_contents);
      }
    }
  }

  for (auto s : tests)
  {
    auto it = s.begin();
    if (it != s.end())
    {
      if (it->compare("t") == 0 && s.size() > 3)
      {
        it++;
        std::string name{*it++};
        std::string input{*it++};
        test_tokenize(name, input, std::vector<std::string>(it, s.end()));
      }
      else if (it->compare("p") == 0 && s.size() == 5)
      {
        it++;
        std::string name{*it++};
        std::string type{*it++};
        std::string input{*it++};
        std::string expected{*it++};
        if (it->compare("Ident") == 0)
        {
          test_parse<Ident>(name, type, input, expected);
        }
        else if (it->compare("Number") == 0)
        {
          test_parse<Number>(name, type, input, expected);
        }
        else if (it->compare("Char") == 0)
        {
          test_parse<Char>(name, type, input, expected);
        }
        else if (it->compare("Bool") == 0)
        {
          test_parse<Bool>(name, type, input, expected);
        }
        else if (it->compare("String") == 0)
        {
          test_parse<String>(name, type, input, expected);
        }
        else if (it->compare("Symbol") == 0)
        {
          test_parse<Symbol>(name, type, input, expected);
        }
        else if (it->compare("Atom") == 0)
        {
          test_parse<Atom>(name, type, input, expected);
        }
        else if (it->compare("Value") == 0)
        {
          test_parse<Value>(name, type, input, expected);
        }
        else if (it->compare("List") == 0)
        {
          test_parse<List>(name, type, input, expected);
        }
      }
    }
  }

  return 0;
}