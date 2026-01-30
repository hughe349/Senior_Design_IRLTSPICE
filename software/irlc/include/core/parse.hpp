#pragma once

#include "core/netlist.hpp"
#include <istream>

// An interface for a parser that can take an input circuit into a raw netlist
class INetlistParser {
  protected:
    INetlistParser() {}

  public:
    virtual RawNetlist parse(std::istream &in) = 0;
};
