/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ANNOTATE_EXCEPTION_HH_
#define ANNOTATE_EXCEPTION_HH_

/* Exception with line numbers */
static void annotate_helper( const char *exception_str, const char *file, int line, const char *function )
{
  fprintf( stderr, "Exception at function %s at %s:%d: %s\n",
           function, file, line, exception_str );
}

#define annotate_exception(exception_str) annotate_helper(exception_str, __FILE__, __LINE__, __func__ )

#endif
