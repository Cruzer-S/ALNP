import socket
import sys
import string
import time

def usages():
    print ("Usage: ", sys.argv[0], "<path>")
    sys.exit()

if (len(sys.argv) != 2):
    usages()

PATH = sys.argv[1]
print ("UNXI Domain Socket: ", PATH)

s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect((PATH))

while (True):
    sbuf = "Time:" + str(time.time())
    s.send(str.encode(sbuf), 0)

    print("[Send:" + str(len(sbuf)) + "]", sbuf)
    rbuf = s.recv(2048);
    print("[Recv:" + str(len(rbuf)), rbuf, "[Echo Msg]");

    time.sleep(1)
