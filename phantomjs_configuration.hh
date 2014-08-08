/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>

#ifndef PHANTOMJS_CONFIGURATION_HH
#define PHANTOMJS_CONFIGURATION_HH

std::string phantomjs_config = "\"\nvar system = require('system')\nvar page = require('webpage').create();\npage.settings.userAgent = 'Chrome/31.0.1650.63 Safari/537.36';\npage.customHeaders = {\"accept\": \"text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\"};\npage.open(url, function (status) {\nif (status != 'success') {\nconsole.log('Unable to load address')\n} else {\nwindow.setTimeout(function () {\nphantom.exit(0);\n}, 200);\n}\n});";

#endif /* PHANTOMJS_CONFIGURATION_HH */
