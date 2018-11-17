#ifndef PARSED_ARGUMENTS_HH
#define PARSED_ARGUMENTS_HH

#include <map>
#include <string>
#include "util.hh"
#include "ezio.hh"

using namespace std;

class ParsedArguments
{
private:
  map<string, string> argMap_;
public:
  ParsedArguments(map<string, string> argMap ): argMap_(argMap)
  {
  }

  double get_float_arg(const string & argName)
  {
    if (argMap_.count(argName) > 0) {
      return myatof(argMap_.at(argName));
    } else {
      throw runtime_error("Missing argument " + argName);
    }
  }

  double get_float_arg(const string & argName, double defaultArg)
  {
    if (argMap_.count(argName) > 0) {
      return myatof(argMap_.at(argName));
    } else {
      return defaultArg;
    }
  }

  int get_int_arg(const string & argName)
  {

    if (argMap_.count(argName) > 0) {
      return myatoi(argMap_.at(argName));
    } else {
      throw runtime_error("Missing argument " + argName);
    }
  }

  int get_int_arg(const string & argName, int defaultArg)
  {
    if (argMap_.count(argName) > 0) {
      return myatoi(argMap_.at(argName));
    } else {
      return defaultArg;
    }

  }

  bool empty()
  {
    return argMap_.empty();
  }

};

#endif /* PARSED_ARGUMENTS_HH */
