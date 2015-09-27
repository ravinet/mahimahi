/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#include "config.h"

#ifndef APACHE_CONFIGURATION_HH
#define APACHE_CONFIGURATION_HH

const std::string apache_main_config = "LoadModule dir_module " + std::string( MOD_DIR ) + "\nLoadModule mpm_prefork_module " + std::string( MOD_MPM_PREFORK ) + "\nLoadModule mime_module " + std::string( MOD_MIME ) + "\n<IfModule mod_mime.c>\nTypesConfig /etc/mime.types\nAddHandler cgi-script .cgi\n</IfModule>\nLoadModule authz_core_module " + std::string( MOD_AUTHZ_CORE ) + "\nLoadModule cgi_module " + std::string( MOD_CGI ) + "\nMutex pthread\n<Directory " + std::string( EXEC_DIR ) + ">\nAllowOverride None\nOptions +ExecCGI\nRequire all granted\n</Directory>\nLoadModule env_module " + std::string( MOD_ENV ) + "\nLoadFile " + std::string( MOD_STDOUTREADER ) + "\nLoadModule stdoutreader_module " + std::string( MOD_STDOUTREADER ) + "\nSetHandler stdoutreader-handler\n";

const std::string apache_ssl_config = "LoadModule ssl_module " + std::string( MOD_SSL ) + "\nSSLEngine on\nSSLCertificateFile      " + std::string( MOD_SSL_CERTIFICATE_FILE ) + "\nSSLCertificateKeyFile " + std::string( MOD_SSL_KEY ) + "\n";

#endif /* APACHE_CONFIGURATION_HH */
