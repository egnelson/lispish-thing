#include <parser.h>

#include <regex>
#include <cstring>
#include <utility>
#include <fstream>
#include <iostream>
#include <sstream>

namespace lang::parser {
  using namespace lang::parser;

  State State::from_file(const std::string& filename)
  {
    State s;
    std::ifstream f;
    f.open(filename);

    if (f.is_open())
    {
      std::string data;
      f.seekg(0, std::ios::end);
      data.reserve(f.tellg());
      f.seekg(0, std::ios::beg);
      data.assign(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
      f.close();
      std::cout << data << std::endl;
      s.len = data.size();
      char *tmp = new char[s.len + 1];
      strncpy(tmp, data.c_str(), s.len);
      tmp[s.len] = 0;
      s.filename = filename;
      s.buffer = tmp;
      s.index = 0;
      s.lineno = 0;
      s.column = 0;
    }

    return s;
  }
  State State::from_string(const std::string& data)
  {
    State s;

    if (data.size() > 0)
    {
      s.len = data.size();
      char *tmp = new char[s.len + 1];
      strncpy(tmp, data.c_str(), s.len);
      tmp[s.len] = 0;
      s.buffer = tmp;
      s.index = 0;
      s.lineno = 0;
      s.column = 0;
    }

    return s;
  }

  State::State()
    : filename()
    , buffer(0)
    , len(0)
    , index(0)
    , lineno(0)
    , column(0)
    , quiet(true)
  {}

  State::State(const State& s)
  {
    *this = s;
  }

  State::State(State&& s)
  {
    *this = s;
  }

  State::~State()
  {
    if (buffer && len > 0)
    {
      delete buffer;
      buffer = 0;
      len = 0;
      index = 0;
      lineno = 0;
      column = 0;
    }
  }

  State& State::operator=(const State& s)
  {
    filename = s.filename;
    len = s.len;
    index = s.index;
    lineno = s.lineno;
    column = s.column;
    char *tmp = new char[len + 1];
    strncpy(tmp, s.buffer, len);
    tmp[len] = 0;
    buffer = tmp;

    return *this;
  }

  State& State::operator=(State&& s)
  {
    filename = s.filename;
    buffer = s.buffer;
    len = s.len;
    index = s.index;
    lineno = s.lineno;
    column = s.column;
    s.buffer = 0;
    s.len = 0;
    s.index = 0;
    s.lineno = 0;
    s.column = 0;

    return *this;
  }

  size_t State::remaining_len() const
  {
    return (len > index) ? (len - index) : 0;
  }

  State::operator bool() const
  {
    return (bool)buffer && remaining_len() > 0;
  }

  void State::fail(const std::string& msg) const
  {
    const size_t len{512};
    char buf[len];
    int end = snprintf(buf, len, "%s at %s:%zu:%zu char '%c'", msg.c_str(), filename.c_str(), lineno + 1, column + 1, buffer[index]);
    buf[end] = 0;
    throw std::runtime_error(buf);
  }

  const std::vector<std::pair<State::Token, std::regex>> regexes({
    std::pair<State::Token, std::regex>{
      State::CHAR,       std::regex{R"%(^'([^\\]|\\([abftvrn'\\]|(x([0-9a-fA-F]{2}))))')%"}},
    std::pair<State::Token, std::regex>{
      State::SYMBOL,     std::regex{R"%(^'[a-zA-Z~!@$%^&*_+=|:<>?/]+[a-zA-Z0-9~!@$%^&*_+=|:<>.?/-]*)%"}},
    std::pair<State::Token, std::regex>{
      State::RATIONAL,   std::regex{R"%(^-?[0-9]+/-?[0-9]+)%"}},
    std::pair<State::Token, std::regex>{
      State::BIN,        std::regex{R"%(^-?0b[01]+)%"}},
    std::pair<State::Token, std::regex>{
      State::OCT,        std::regex{R"%(^-?0o[0-7]+)%"}},
    std::pair<State::Token, std::regex>{
      State::HEX,        std::regex{R"%(^-?0x[0-9a-fA-F]+)%"}},
    std::pair<State::Token, std::regex>{
      State::FLT,        std::regex{R"%(^-?[0-9]+\.[0-9]*[eE]-?[0-9]+)%"}},
    std::pair<State::Token, std::regex>{
      State::FLT,        std::regex{R"%(^-?[0-9]+\.[0-9]+)%"}},
    std::pair<State::Token, std::regex>{
      State::FLT,        std::regex{R"%(^-?[0-9]+\.)%"}},
    std::pair<State::Token, std::regex>{
      State::FLT,        std::regex{R"%(^-?\.[0-9]+[eE]-?[0-9]+)%"}},
    std::pair<State::Token, std::regex>{
      State::FLT,        std::regex{R"%(^-?\.[0-9]+)%"}},
    std::pair<State::Token, std::regex>{
      State::DEC,        std::regex{R"%(^-?[0-9]+)%"}},
    std::pair<State::Token, std::regex>{
      State::IDENT,      std::regex{R"%(^[a-zA-Z~!@$%^&*_+=|:<>?/]+[a-zA-Z0-9~!@$%^&*_+=|:<>.?/-]*)%"}},
    std::pair<State::Token, std::regex>{
      State::STRING,     std::regex{R"%(^"([^\\"]|\\([abftvrn"\\]|x([0-9a-fA-F][0-9a-fA-F])))*")%"}},
  });

  static const std::vector<std::pair<State::Token, std::string>> strings({
    std::pair<State::Token, std::string>{
      State::LIST_START, std::string{"("}},
    std::pair<State::Token, std::string>{
      State::LIST_END,   std::string{")"}},
    std::pair<State::Token, std::string>{
      State::CONS_START, std::string{"'("}},
    std::pair<State::Token, std::string>{
      State::BOOL,       std::string{"true"}},
    std::pair<State::Token, std::string>{
      State::BOOL,       std::string{"false"}},
  });

  std::pair<State::Token, std::string> State::token()
  {
    std::pair<Token, std::string> out{EOI, ""};

    if (*this)
    {
      out.first = UNKNOWN;

      for (char c{buffer[index]};
           *this && (c == ' ' || c == '\t' || c == '\n' || c == '\r');
           bump(), c = buffer[index])
      {}

      const char *str{&buffer[index]};
      for (auto& p : strings)
      {
        if (strncmp(p.second.c_str(), str, p.second.size()) == 0)
        {
          out.first = p.first;
          out.second = p.second;
          bump(out.second.size());
          break;
        }
      }

      if (out.first == UNKNOWN)
      {
        for (auto& p : regexes)
        {
          std::cmatch m;
          if (std::regex_search(str, m, p.second))
          {
            out.first = p.first;
            out.second = m.str(0);
            bump(out.second.size());

            break;
          }
        }
      }

      for (char c{buffer[index]};
           *this && (c == ' ' || c == '\t' || c == '\n' || c == '\r');
           bump(), c = buffer[index])
      {}
    }

    return out;
  }


  void State::bump(size_t len_)
  {
    if (*this && len_ > 0)
    {
      const size_t new_idx{index + len_};

      for (index++; *this && index < new_idx; index++)
      {
        char c = buffer[index];
        if (c == '\n')
        {
          column = 0;
          lineno++;
        }
        else if (c != '\r')
        {
          column++;
        }
      }
    }
  }

  std::string State::location()
  {
    const size_t len{512};
    char buf[len];
    int end = snprintf(buf, len, "%s:%zu:%zu", filename.c_str(), lineno + 1, column + 1);
    return std::string(buf, end);
  }

  std::string State::token_to_string(Token t)
  {
    std::stringstream ss;
    switch (t)
    {
    case IDENT:
      ss << "IDENT";
      break;
    case BIN:
      ss << "BIN";
      break;
    case OCT:
      ss << "OCT";
      break;
    case DEC:
      ss << "DEC";
      break;
    case HEX:
      ss << "HEX";
      break;
    case FLT:
      ss << "FLT";
      break;
    case RATIONAL:
      ss << "RATIONAL";
      break;
    case CHAR:
      ss << "CHAR";
      break;
    case BOOL:
      ss << "BOOL";
      break;
    case STRING:
      ss << "STRING";
      break;
    case SYMBOL:
      ss << "SYMBOL";
      break;
    case CONS_START:
      ss << "CONS_START";
      break;
    case LIST_START:
      ss << "LIST_START";
      break;
    case LIST_END:
      ss << "LIST_END";
      break;
    case COMMENT:
      ss << "COMMENT";
      break;
    case EOI:
      ss << "EOI";
      break;
    case UNKNOWN:
      ss << "UNKNOWN";
      break;
    }
    return ss.str();
  }

  std::pair<State, Ident> Ident::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Ident> out;

    if (!ok)
    {
      state.fail("End of input");
    }

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::IDENT;
    }

    if (ok)
    {
      out.second.val = tkn.second;
      out.first = st;
    }

    if (!ok)
    {
      state.fail("Expected ident token");
    }
    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Ident& item)
  {
    os << "Ident(" << item.val << ")";
    return os;
  }

  std::pair<State, Number> Number::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Number> out;

    if (!ok)
    {
      state.fail("End of input");
    }

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::BIN || tkn.first == State::OCT ||
        tkn.first == State::DEC || tkn.first == State::HEX ||
        tkn.first == State::FLT || tkn.first == State::RATIONAL;
    }

    if (ok)
    {
      out.first = st;
      size_t erase_idx{(tkn.second[0] == '-') ? static_cast<size_t>(1) : static_cast<size_t>(0)};
      int base(0);

      switch (tkn.first)
      {
      case State::BIN:
        base = 2;
        break;
      case State::OCT:
        base = 8;
        break;
      case State::HEX:
        base = 16;
        break;
      default:
        base = 10;
      }

      switch (tkn.first)
      {
      case State::BIN:
        [[fallthrough]];
      case State::OCT:
        [[fallthrough]];
      case State::DEC:
        [[fallthrough]];
      case State::HEX:
      {
        tkn.second.erase(erase_idx, 2);
        char *end{reinterpret_cast<char*>(1)};
        out.second.kind = N;
        out.second.i = std::strtoll(tkn.second.c_str(), &end, base);
        if (errno == ERANGE)
        {
          state.fail("Integer literal too large");
        }
        else if (end != &tkn.second.c_str()[tkn.second.size()])
        {
          state.fail("Invalid integer literal");
        }
      } break;
      case State::FLT:
      {
        tkn.second.erase(erase_idx, 2);
        char *end{reinterpret_cast<char*>(1)};
        out.second.kind = F;
        out.second.i = std::strtod(tkn.second.c_str(), &end);
        if (errno == ERANGE)
        {
          state.fail("Floating-point number literal too large");
        }
        else if (end != &tkn.second.c_str()[tkn.second.size()])
        {
          state.fail("Invalid floating-point number literal");
        }
      } break;
      case State::RATIONAL:
      {
        out.second.kind = R;

        size_t slash_idx{tkn.second.find('/')};
        std::string numerator{tkn.second.substr(0, slash_idx)};
        slash_idx++;
        std::string denomenator{tkn.second.substr(slash_idx, tkn.second.size() - slash_idx)};

        char *end{reinterpret_cast<char*>(1)};
        out.second.r.first = std::strtoll(numerator.c_str(), &end, base);
        if (errno == ERANGE)
        {
          state.fail("Integer literal too large");
        }
        else if (end != &tkn.second.c_str()[tkn.second.size()])
        {
          state.fail("Invalid integer literal");
        }
        out.second.r.second = std::strtoll(denomenator.c_str(), &end, base);
        if (errno == ERANGE)
        {
          state.fail("Integer literal too large");
        }
        else if (end != &tkn.second.c_str()[tkn.second.size()])
        {
          state.fail("Invalid integer literal");
        }
      } break;
      default:
        state.fail("Bad token type");
      }
    }

    if (!ok)
    {
      state.fail("Expected number token");
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Number& item)
  {
    switch (item.kind)
    {
    case Number::F:
      os << "Float(" << item.d << ")";
      break;
    case Number::N:
      os << "Integer(" << item.i << ")";
      break;
    case Number::R:
      os << "Rational(" << item.r.first << "/" << item.r.second << ")";
      break;
    default:
      os << "Number(Uninitialized)";
    }
    return os;
  }

  std::pair<State, Char> Char::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Char> out;

    if (!ok)
    {
      state.fail("End of input");
    }

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::CHAR;
    }

    if (ok)
    {
      out.first = st;
      std::string chr{tkn.second.substr(1, tkn.second.size() - 2)};

      if (chr[0] == '\\' && chr.size() > 1)
      {
        switch (chr[1])
        {
          case 'a': {
            out.second.val = '\a';
          } break;
          case 'b': {
            out.second.val = '\b';
          } break;
          case 'f': {
            out.second.val = '\f';
          } break;
          case 'n': {
            out.second.val = '\n';
          } break;
          case 't': {
            out.second.val = '\t';
          } break;
          case 'v': {
            out.second.val = '\v';
          } break;
          case 'r': {
            out.second.val = '\r';
          } break;
          case '\'': {
            out.second.val = '\'';
          } break;
          case '"': {
            out.second.val = '"';
          } break;
          case 'x': {
            if (chr.size() == 4)
            {
              char c1 = chr[2];
              char c2 = chr[3];
              out.second.val = ((c1 & 0xF) << 4) | (c2 & 0xF);
            }
            else
            {
              state.fail("\\x escape sequences must have two hex chars");
            }
          } break;
          default: {
            state.fail("Invalid escape sequence");
          }
        }
      }
      else
      {
        out.second.val = chr[0];
      }
    }

    if (!ok)
    {
      state.fail("Expected char token");
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Char& item)
  {
    os << "Char(" << item.val << ")";
    return os;
  }

  std::pair<State, Bool> Bool::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Bool> out;

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::BOOL;
    }

    if (ok)
    {
      out.first = st;
      out.second.val = tkn.second.compare("true") == 0;
    }

    if (!ok)
    {
      state.fail("Expected bool token");
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Bool& item)
  {
    os << "Bool(" << (item.val ? "true" : "false") << ")";
    return os;
  }

  std::pair<State, String> String::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, String> out;

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::STRING;
    }

    if (ok)
    {
      out.first = st;
      out.second.val = tkn.second.substr(1, tkn.second.size() - 2);
    }

    if (!ok)
    {
      state.fail("Expected string token");
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const String& item)
  {
    os << "String(" << item.val << ")"; // TODO re-escape
    return os;
  }

  std::pair<State, Symbol> Symbol::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Symbol> out;

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::SYMBOL;
    }

    if (ok)
    {
      out.first = st;
      out.second.val = tkn.second.substr(1, tkn.second.size() - 1);
    }

    if (!ok)
    {
      state.fail("Expected string token");
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Symbol& item)
  {
    os << "Symbol('" << item.val << ")";
    return os;
  }

  std::pair<State, Atom> Atom::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Atom> out;

    if (ok)
    {
      ok = false;
      try
      {
        std::pair<State, Number> n{Number::parse(st)};
        ok = true;
        out.second.kind = NU;
        out.second.n = n.second;
        out.first = n.first;
      }
      catch (std::runtime_error&)
      {}

      if (!ok)
      {
        try
        {
          std::pair<State, Bool> i{Bool::parse(state)};
          ok = true;
          out.second.kind = BL;
          out.second.b = i.second;
          out.first = i.first;
        }
        catch (std::runtime_error&)
        {}
      }

      if (!ok)
      {
        try
        {
          std::pair<State, Ident> i{Ident::parse(state)};
          ok = true;
          out.second.kind = ID;
          out.second.i = new Ident;
          out.first = i.first;
          *out.second.i = i.second;
        }
        catch (std::runtime_error&)
        {}
      }

      if (!ok)
      {
        try
        {
          std::pair<State, Char> i{Char::parse(state)};
          ok = true;
          out.second.kind = CH;
          out.second.c = i.second;
          out.first = i.first;
        }
        catch (std::runtime_error&)
        {}
      }

      if (!ok)
      {
        try
        {
          std::pair<State, String> i{String::parse(state)};
          ok = true;
          out.second.kind = ST;
          out.second.s = new String;
          *out.second.s = i.second;
          out.first = i.first;
        }
        catch (std::runtime_error&)
        {}
      }

      if (!ok)
      {
        try
        {
          std::pair<State, Symbol> i{Symbol::parse(state)};
          ok = true;
          out.second.kind = SY;
          out.second.sy = new Symbol;
          *out.second.sy = i.second;
          out.first = i.first;
        }
        catch (std::runtime_error&)
        {}
      }

      if (!ok)
      {
        state.fail("Expected ident, number, bool, char, string, or symbol token");
      }
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Atom& item)
  {
    os << "A(";
    switch (item.kind)
    {
      case Atom::NU:
        os << item.n;
      break;
      case Atom::CH:
        os << item.c;
      break;
      case Atom::BL:
        os << item.b;
      break;
      case Atom::ST:
        if (item.s)
        {
          os << *item.s;
        }
        else
        {
          os << "Uninitialized String";
        }
      break;
      case Atom::ID:
        if (item.i)
        {
          os << *item.i;
        }
        else
        {
          os << "Uninitialized Ident";
        }
      break;
      case Atom::SY:
        if (item.sy)
        {
          os << *item.sy;
        }
        else
        {
          os << "Uninitialized Symbol";
        }
      break;
    }
    os << ")";
    return os;
  }

  Atom::Atom()
    : sy(0)
    , kind(SY)
  {}

  Atom::Atom(const Atom& a)
  {
    *this = a;
  }

  Atom::~Atom()
  {
    if (kind == ST && s)
    {
      delete s;
      s = 0;
    }
    else if (kind == ID && i)
    {
      delete i;
      i = 0;
    }
    else if (kind == SY && sy)
    {
      delete sy;
      sy = 0;
    }
  }

  Atom& Atom::operator=(const Atom& a)
  {
    kind = a.kind;
    switch (kind)
    {
      case NU:
        n = a.n;
      break;
      case CH:
        c = a.c;
      break;
      case BL:
        b = a.b;
      break;
      case ST:
        if (a.s)
        {
          s = new String;
          *s = *a.s;
        }
      break;
      case ID:
        if (a.i)
        {
          i = new Ident;
          *i = *a.i;
        }
      break;
      case SY:
        if (a.sy)
        {
          sy = new Symbol;
          *sy = *a.sy;
        }
      break;
    }
    return *this;
  }

  std::pair<State, List> List::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, List> out;

    if (ok)
    {
      tkn = st.token();
      ok = tkn.first == State::LIST_START || tkn.first == State::CONS_START;
    }

    if (ok)
    {
      while (st)
      {
        try
        {
          auto p{Value::parse(st)};
          out.second.val.push_back(p.second);
          st = std::move(p.first);
        }
        catch (std::runtime_error&)
        {
          break;
        }
      }

      tkn = st.token();
      ok = tkn.first == State::LIST_END;
    }

    if (ok)
    {
      out.first = std::move(st);
    }
    else
    {
      state.fail("Expected list or cons start token");
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const List& item)
  {
    os << "L( ";
    for (auto& i : item.val)
    {
      os << i << " ";
    }
    os << ")";
    return os;
  }

  std::pair<State, Value> Value::parse(const State& state)
  {
    bool ok = (bool)state;
    State st{state};
    std::pair<State::Token, std::string> tkn;
    std::pair<State, Value> out;

    if (ok)
    {
      ok = false;
      try
      {
        std::pair<State, List> n{List::parse(st)};
        ok = true;
        out.second.kind = L;
        out.second.l = new List;
        out.second.l->val = n.second.val;
        out.first = n.first;
      }
      catch (std::runtime_error&)
      {}

      if (!ok)
      {
        try
        {
          std::pair<State, Atom> i{Atom::parse(state)};
          ok = true;
          out.second.kind = A;
          out.second.a = new Atom;
          *out.second.a = i.second;
          out.first = i.first;
        }
        catch (std::runtime_error&)
        {}
      }

      if (!ok)
      {
        state.fail("Expected list or atom");
      }
    }

    return out;
  }

  std::ostream& operator<<(std::ostream& os, const Value& item)
  {
    os << "V(";
    switch (item.kind)
    {
      case Value::A:
        if (item.a)
        {
          os << *item.a;
        }
        else
        {
          os << "A(uninit)";
        }
        break;
      case Value::L:
        if (item.l)
        {
          os << *item.l;
        }
        else
        {
          os << "L(uninit)";
        }
        break;
    }
    os << ")";
    return os;
  }

  std::ostream& operator<<(std::ostream& s, const File& item)
  {
    s << "File(";
    for (auto& i : item.exprs)
    {
      s << i;
    }
    s << ")";
    return s;
  }

  void File::parse(State& state)
  {
    if (!state.quiet)
    {
      std::cout << "File::parse at " << state.location() << std::endl;
    }
    exprs.clear();

    while (state)
    {
      auto p{Value::parse(state)};
      exprs.push_back(p.second);
      state = std::move(p.first);
    }
  }

  std::string File::print()
  {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

  bool Ident::operator==(const Ident& item) const
  {
    return val.compare(item.val) == 0;
  }
  bool Number::operator==(const Number& item) const
  {
    bool eq{kind == item.kind};
    if (eq)
    {
      switch (kind)
      {
      case N:
        eq = i == item.i;
      break;
      case F:
        eq = d == item.d;
        break;
      case R:
        eq = r == r;
        break;
      }
    }
    return eq;
  }
  bool Char::operator==(const Char& item) const
  {
    return val == item.val;
  }
  bool Bool::operator==(const Bool& item) const
  {
    return val == item.val;
  }
  bool String::operator==(const String& item) const
  {
    return val.compare(item.val) == 0;
  }
  bool Atom::operator==(const Atom& item) const
  {
    bool eq{kind == item.kind};
    if (eq)
    {
      switch (kind)
      {
      case NU:
        eq = n == item.n;
      break;
      case CH:
        eq = c == item.c;
      break;
      case BL:
        eq = b == item.b;
      break;
      case ST:
        if (s && item.s)
        {
          eq = *s == *item.s;
        }
        else
        {
          eq = !s && !item.s;
        }
      break;
      case ID:
        if (i && item.i)
        {
          eq = *i == *item.i;
        }
        else
        {
          eq = !i && !item.i;
        }
      break;
      case SY:
        if (sy && item.sy)
        {
          eq = *sy == *item.sy;
        }
        else
        {
          eq = !sy && !item.sy;
        }
      break;
      }
    }
    return eq;
  }
  bool Symbol::operator==(const Symbol& item) const
  {
    return val.compare(item.val) == 0;
  }
  bool Value::operator==(const Value& item) const
  {
    bool eq{kind == item.kind};
    if (eq)
    {
      switch (kind)
      {
      case A:
        if (a && item.a)
        {
          eq = *a == *item.a;
        }
        else
        {
          eq = !a && !item.a;
        }
      break;
      case L:
        if (l && item.l)
        {
          eq = *l == *item.l;
        }
        else
        {
          eq = !l && !item.l;
        }
      break;
      }
    }
    return eq;
  }
  bool List::operator==(const List& item) const
  {
    bool eq{val.size() == item.val.size()};

    if (eq)
    {
      for (size_t i = 0; eq && i < val.size(); i++)
      {
        eq = val[i] == item.val[i];
      }
    }

    return eq;
  }
}
