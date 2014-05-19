/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#include "config.h"

#ifndef APACHE_CONFIGURATION_HH
#define APACHE_CONFIGURATION_HH

std::string apache_main_config =
"LoadModule dir_module " + std::string( MOD_DIR ) + "\n" +
"LoadModule mpm_prefork_module " + std::string( MOD_MPM_PREFORK ) + "\n" +
"LoadModule mime_module " + std::string( MOD_MIME ) + "\n" +

"<IfModule mod_mime.c>\n" +
"TypesConfig /etc/mime.types\n"+
"AddHandler cgi-script .cgi\n" +
"</IfModule>\n" +

"LoadModule authz_core_module " + std::string( MOD_AUTHZ_CORE ) + "\n" +
"LoadModule cgi_module " + std::string( MOD_CGI ) + "\n" +

"<Directory " + std::string( EXEC_DIR ) + ">\n" +
"AllowOverride None\n" +
"Options +ExecCGI\n" +
"Require all granted\n" +
"</Directory>\n" +

"LoadModule rewrite_module " + std::string( MOD_REWRITE ) + "\n" +
"RewriteEngine On\n" +
"RewriteRule ^(.*)$ " + std::string( REPLAYSERVER ) + "\n" +

"LoadModule env_module " + std::string( MOD_ENV ) + "\n" +
"SetEnv RECORD_FOLDER";

std::string apache_ssl_config =
"LoadModule ssl_module " + std::string( MOD_SSL ) + "\n" +
"SSLEngine on\n" +
"SSLCertificateFile      " + std::string( MOD_SSL_CERTIFICATE_FILE ) + "\n" +
"SSLCertificateKeyFile " + std::string( MOD_SSL_KEY ) +"\n";

#endif /* APACHE_CONFIGURATION_HH */
