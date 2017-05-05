from __future__ import print_function
import sys, datetime, threading
import struct, binascii
import paho.mqtt.publish as publish
if sys.version_info[0] < 3:
    import SocketServer as socketserver
else:
    import socketserver

######### SOME USER DECLARATIONS FOR MQTT CONFIGURATIONS #######
field_list = []
value_list = []
channelID = "226554"
apiKey = "8QOW4YE49TXLJBEB"
# This type of unsecured MQTT connection
# uses the least amount of system resources (port 1883)
useUnsecuredTCP = False
# Set useUnsecuredWebSockets to True to use MQTT
# over an unsecured websocket (port 80).
useUnsecuredWebsockets = False
# This type of connection will use slightly more system resources,
# but the connection will be secured by SSL (port 443)
useSSLWebsockets = True
# Hostname ThingSpeak MQTT service
mqttHost = "mqtt.thingspeak.com"
if useUnsecuredTCP:
    tTransport = "tcp" # transport protocol
    tPort = 1883 # port
    tTLS = None # encrypted link
elif useUnsecuredWebsockets:
    tTransport = "websockets"
    tPort = 80
    tTLS = None
elif useSSLWebsockets:
    import ssl
    tTransport = "websockets"
    tTLS = {'ca_certs':"/etc/ssl/certs/ca-certificates.crt",'tls_version':ssl.PROTOCOL_TLSv1}
    tPort = 443
# Create the topic string
topic = "channels/" + channelID + "/publish/" + apiKey
################# END OF USER DECLARATIONS ###################

class MeshHandler(socketserver.BaseRequestHandler): # request handler class of server

    def handle(self):
        self.buf = bytearray() # transform handle.buf in array of bytes, not string
        print('New node in mesh\n')
        try:
            while True:
                header = self.read_full(4)
                l, = struct.unpack_from('<H', header[2:4])
                body = self.read_full(l-4)
                req = bytearray()
                req.extend(header)
                req.extend(body)
                resp = bytearray()
                msg = "message received by server\r\n" # acknowledge from server to node
                req[2] = 16 + len(msg) # total length of mesh packet
                resp.extend(req[0:4]) # part of mesh header
                resp.extend(req[10:16]) # dst address in mesh header
                resp.extend(req[4:10]) # src address in mesh header
                resp.extend(msg)
                src = binascii.b2a_hex(req[10:16])
                value_list.append(req[-7:-5]) # int value of temperature
                if src == "18fe34df0f4c":
                    field_list.append("field1")
                    to_thingspeak = True
                elif src == "5ccf7f86935f":
                    field_list.append("field2")
                    to_thingspeak = True
                elif src == "5ccf7fc11ade":
                    field_list.append("field3")
                    to_thingspeak = True
                elif src == "5ccf7f8691f1":
                    field_list.append("field4")
                    to_thingspeak = True
                else:
                    print("Mesh node doesn't have a channel-field in ThingSpeak")
                    to_thingspeak = False
                if to_thingspeak:
                    print("\nsending to channel ... {} in ThingSpeak" .format(channelID))
                    print("Waiting for transmission time")
                list_aux1 = list(src)
                i = 0 # counter
                j = 2 # pivot
                while i<5: # building MAC ADDRESS XX:XX:XX:XX:XX:XX
                    list_aux2 = list_aux1[j:]
                    list_aux1[j] = ":"
                    list_aux1[j+1:] = list_aux2
                    j = j+3
                    i += 1
                src = ''.join(list_aux1) # MAC ADDRESS in string format
                print("message received from {} at {}".format(src, datetime.datetime.now())) # from ... XX:XX:XX:XX:XX:XX
                print(req[16:]) # print node message
                self.request.sendall(resp) # send mesh packet through TCP socket
        except Exception as e:
            print(e)

    def read_full(self, n): # read mesh packet data --> read_full(handler,number_of_bytes)
        while len(self.buf) < n:
            try:
                req = self.request.recv(1024) # collects 1kByte
                if not req:
                    raise(Exception('recv error'))
                self.buf.extend(req)
            except Exception as e:
                raise(e)
        read = self.buf[0:n] # passe values to "read" variable
        self.buf = self.buf[n:]
        return bytes(read) # return bytes that have been read

def publish_mqtt():
    if field_list:
        tPayload = field_list[0] + "=" + str(value_list[0])
        try:
            publish.single(topic, payload=tPayload, hostname=mqttHost, port=tPort, tls=tTLS, transport=tTransport)
            print("the data have been published in channel {} and {}".format(channelID, field_list[0]))
            field_list[0:] = field_list[1:]
            value_list[0:] = value_list[1:]
        except:
            print ("There was an error while publishing the data\n")
    else:
        print("There's not a message to publish in ThingSpeak")
    threading.Timer(15, publish_mqtt).start()
    return

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 7000 # server port
    server = ThreadedTCPServer((HOST, PORT), MeshHandler)
    server.allow_reuse_address = True
    print('mesh server is working')
    publish_mqtt()
    server.serve_forever() # run server
