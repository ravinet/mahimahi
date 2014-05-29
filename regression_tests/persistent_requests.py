#must run "sudo pip install requests"
#if that doesnt work, first do "sudo pip install --upgrade setuptools" 
import requests
from time import time

start = time()
s = requests.Session()
r = s.get('http://nms.csail.mit.edu/')
#r = s.get('http://www.mit.edu/')
print r.status_code
r = s.get('http://nms.csail.mit.edu/nms_logo.jpg')
end = time()
#r = s.get('http://www.mit.edu/favicon.ico')
print r.status_code
total = end - start
print total
