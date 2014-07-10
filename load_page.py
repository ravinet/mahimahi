import sys
from selenium import webdriver
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.keys import Keys
from pyvirtualdisplay import Display
import os
from time import sleep
#TO RUN: download https://pypi.python.org/packages/source/s/selenium/selenium-2.39.0.tar.gz
#run sudo apt-get install python-setuptools
#after untar, run sudo python setup.py install
#follow directions here: https://pypi.python.org/pypi/PyVirtualDisplay to install pyvirtualdisplay

#For chrome, need chrome driver: https://code.google.com/p/selenium/wiki/ChromeDriver
#chromedriver variable should be path to the chromedriver
#the default location for firefox is /usr/bin/firefox and chrome binary is /usr/bin/google-chrome
#if they are at those locations, don't need to specify

site = sys.argv[1]
#delay = int(sys.argv[2])*2
#link_speed = sys.argv[3]
#if sys.argv[4] == "replay":
#    #results = open("replay_summary_" + str(delay) + ".txt", 'a')
#    file_name = "replay_summary_" + str(link_speed) + "Mbps_" + str(delay) + "ms.txt"
#    results = open( file_name, 'a' )

### to not display the page in browser ###
#display = Display(visible=0, size=(800,600))
#display.start()

### to run chrome ###
options=Options()
options.add_argument("--incognito")
options.add_argument("--ignore-certificate-errors")
chromedriver = "/home/ravi/chromedriver"
driver=webdriver.Chrome(chromedriver, chrome_options=options)

# to run firefox ###
#driver = webdriver.Firefox()

#profile = webdriver.FirefoxProfile()
#profile.set_preference("webdriver_assume_untrusted_issuer", "false") 

driver.set_page_load_timeout(500)
driver.get(site)
#sleep(10)


#notes: http://www.sitepoint.com/profiling-page-loads-with-the-navigation-timing-api/

#beginning of page load as perceived by user (same as fetchstart if no previous document)
navigationStart = driver.execute_script("return window.performance.timing.navigationStart")

#time just before browser starts searching for URL
fetchStart = driver.execute_script("return window.performance.timing.fetchStart")

#time just before browser sends request for URL (first request)...dns and connection already done
requestStart = driver.execute_script("return window.performance.timing.requestStart")

#time just after browser receives first byte of response
responseStart = driver.execute_script("return window.performance.timing.responseStart")

#time just after browser receives last byte of response
responseEnd = driver.execute_script("return window.performance.timing.responseEnd") 

#time just before dom is set to complete
domComplete = driver.execute_script("return window.performance.timing.domComplete")

#end of page load
loadEventEnd = driver.execute_script("return window.performance.timing.loadEventEnd")

backendPerformance = responseStart - navigationStart
frontendPerformance = domComplete - responseStart
print "Back End: %s" % backendPerformance
print "Front End: %s" % frontendPerformance

#Network latency (total time to complete document fetch): responseEnd-fetchStart
#The time taken for page load once the page is received from the server: loadEventEnd-responseEnd
#The whole process of navigation and page load: loadEventEnd-navigationStart (this is total page load delay experienced by user)
#total time taken to send a request to the server and receive the response: responseEnd - requestStart

network_latency = responseEnd - fetchStart
load_after_server_rcv = loadEventEnd - responseEnd
while loadEventEnd == 0:
    loadEventEnd = driver.execute_script("return window.performance.timing.loadEventEnd")
complete_process = loadEventEnd - navigationStart
reqres = responseEnd - requestStart
print "Network Latency (just before request to end of response): %s" % network_latency
print "Page load after page is received from server: %s" % load_after_server_rcv
print "Send a request and receive response: %s" % reqres
print "Entire process (navigation and page load): %s" % complete_process
#if sys.argv[4] == "replay":
#    results.write( site + " "  + str(complete_process) + "\n")
#    sleep(1)
    #os.system(  "sudo grep -r \"Can't find:\" /tmp/ > " + site + ".txt" ) 

#if sys.argv[2] == "replay":
#    driver.get_screenshot_as_file('/Screenshots/' + site)
sleep(4)
driver.quit()
