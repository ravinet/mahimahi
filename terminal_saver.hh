/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TERMINAL_SAVER_HH
#define TERMINAL_SAVER_HH

#include <termios.h>
#include <unistd.h>

/* helper class to save and restore the termios state */
class TerminalSaver
{
private:
  struct termios saved_termios_;

public:
  TerminalSaver()
    : saved_termios_()
  {
    if ( tcgetattr( STDIN_FILENO, &saved_termios_ ) < 0 ) {
      throw Exception( "tcgetattr" );
    }
  }

  ~TerminalSaver()
  {
    if ( tcsetattr( STDIN_FILENO, TCSANOW, &saved_termios_ ) < 0 ) {
      throw Exception( "tcsetattr" );
    }
  }

};

#endif /* TERMINAL_SAVER_HH */
