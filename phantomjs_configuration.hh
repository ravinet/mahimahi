/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#ifndef PHANTOMJS_CONFIGURATION_HH
#define PHANTOMJS_CONFIGURATION_HH

std::string phantomjs_setup = "\";\nvar system = require('system');\nvar page = require('webpage').create();\npage.settings.userAgent = '";

std::string phantomjs_load = "\npage.open(url, function (status) {\nif (status != 'success') {\nconsole.log('Unable to load address: ' + url);\nphantom.exit(0);\n} else {\nwindow.setTimeout(function () {\nphantom.exit(0);\n}, 200);\n}\n});";

#endif /* PHANTOMJS_CONFIGURATION_HH */
