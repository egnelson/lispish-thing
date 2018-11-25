#pragma once

#include <parser.h>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>
#include <ostream>

namespace lang::parser {

  struct State {
    static State from_file(const std::string& file);
    static State from_string(const std::string& data);

    State();
    State(const State&);
    State(State&&);
    ~State();

    State& operator=(const State&);
    State& operator=(State&&);

    enum Token
    {
      IDENT,
      BIN,
      OCT,
      DEC,
      HEX,
      FLT,
      RATIONAL,
      CHAR,
      BOOL,
      STRING,
      SYMBOL,
      CONS_START,
      LIST_START,
      LIST_END,
      COMMENT,
      EOI,
      UNKNOWN
    };

    std::string filename;
    const char *buffer;
    size_t len;
    size_t index;
    size_t lineno;
    size_t column;
    size_t remaining_len() const;
    operator bool() const;
    void fail(const std::string& msg) const;
    std::pair<Token, std::string> token();
    void bump(size_t len = 1);
    std::string location();
    static std::string token_to_string(Token t);
    bool quiet;
  };

  struct Ident
  {
    bool operator==(const Ident& item) const;
    static std::pair<State, Ident> parse(const State& state);
    std::string val;
    friend std::ostream& operator<<(std::ostream& os, const Ident& item);
  };
  std::ostream& operator<<(std::ostream& os, const Ident& item);

  struct Number
  {
    bool operator==(const Number& item) const;
    static std::pair<State, Number> parse(const State& state);

    union {
      int64_t i;
      double d;
    };
    std::pair<int64_t, int64_t> r;

    enum { F, N, R } kind;
    friend std::ostream& operator<<(std::ostream& os, const Number& item);
  };
  std::ostream& operator<<(std::ostream& os, const Number& item);

  struct Char
  {
    bool operator==(const Char& item) const;
    static std::pair<State, Char> parse(const State& state);
    char val;
    friend std::ostream& operator<<(std::ostream& os, const Char& item);
  };
  std::ostream& operator<<(std::ostream& os, const Char& item);

  struct Bool
  {
    bool operator==(const Bool& item) const;
    static std::pair<State, Bool> parse(const State& state);
    bool val;
    friend std::ostream& operator<<(std::ostream& os, const Bool& item);
  };
  std::ostream& operator<<(std::ostream& os, const Bool& item);

  struct String
  {
    bool operator==(const String& item) const;
    static std::pair<State, String> parse(const State& state);
    std::string val;
    friend std::ostream& operator<<(std::ostream& os, const String& item);
  };
  std::ostream& operator<<(std::ostream& os, const String& item);

  struct Symbol
  {
    bool operator==(const Symbol& item) const;
    static std::pair<State, Symbol> parse(const State& state);
    std::string val;
    friend std::ostream& operator<<(std::ostream& os, const Symbol& item);
  };
  std::ostream& operator<<(std::ostream& os, const Symbol& item);

  struct Atom
  {
    bool operator==(const Atom& item) const;
    static std::pair<State, Atom> parse(const State& state);
    Atom();
    Atom(const Atom& a);
    ~Atom();

    Atom& operator=(const Atom& a);

    union {
      Number n;
      Char c;
      Bool b;
      String *s;
      Ident *i;
      Symbol *sy;
    };

    enum { NU, CH, BL, ST, ID, SY } kind;
    friend std::ostream& operator<<(std::ostream& os, const Atom& item);
  };
  std::ostream& operator<<(std::ostream& os, const Atom& item);

  struct List;

  struct Value
  {
    bool operator==(const Value& item) const;
    static std::pair<State, Value> parse(const State& state);

    union
    {
      Atom *a;
      List *l;
    };

    enum { A, L } kind;
    friend std::ostream& operator<<(std::ostream& os, const Value& item);
  };
  std::ostream& operator<<(std::ostream& os, const Value& item);

  struct List
  {
    bool operator==(const List& item) const;
    static std::pair<State, List> parse(const State& state);
    std::vector<Value> val;
    bool is_cons{false};
    friend std::ostream& operator<<(std::ostream& os, const List& item);
  };
  std::ostream& operator<<(std::ostream& os, const List& item);

  struct File
  {
    void parse(State& state);
    std::string print();

    std::vector<Value> exprs;
  };
}
