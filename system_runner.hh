#ifndef SYSTEM_RUNNER_HH
#define SYSTEM_RUNNER_HH

#include <vector>
#include <string>
#include <functional>

int ezexec( const std::vector< std::string > & command );
void run( const std::vector< std::string > & command );
void in_network_namespace( pid_t pid, std::function<void(void)> && procedure );

#endif /* SYSTEM_RUNNER_HH */
