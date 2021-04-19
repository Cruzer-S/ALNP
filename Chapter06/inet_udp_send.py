import socket
import sys
import string
import time

HOST = sys.argv[1]
PORT = int(sys.argv[2])

print("Destination Address: %s %d" % (HOST, PORT))

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while (True):
    sbuf = "Time: " + str(time.time())
    s.sendto(str.encode(sbuf), 0, (HOST, PORT))
    print("[Send] (%d) %s" % (len(sbuf), sbuf) )
    rbuf = s.recvfrom(2048)
    print("[Recv] Addr %s Len(%s)" % (str(rbuf[1]), len(rbuf[0])) )
    print("\tBuff(%s)" % (rbuf[0]).decode('UTF-8'))

    time.sleep(2)

s.close()
