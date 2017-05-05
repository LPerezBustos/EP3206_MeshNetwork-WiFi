from __future__ import print_function
import sys, datetime, threading
import struct, binascii
if sys.version_info[0] < 3:
    import SocketServer as socketserver
else:
    import socketserver

class MeshHandler(socketserver.BaseRequestHandler): # Clase del manejador

    def handle(self):
        self.buf = bytearray() # Convierte handle.buf en arreglo de bytes
        print('New node in mesh\n') # un nuevo nodo se ha agregado a la red
        try:
            while True:
                header = self.read_full(4) # ir a funcion read_full(self, 4)
                l, = struct.unpack_from('<H', header[2:4]) # longitud
                body = self.read_full(l-4) # ir a funcion read_full(self, l-4)
                req = bytearray()
                req.extend(header)
                req.extend(body)
                ######### construccion del mensaje de respuesta #########
                resp = bytearray()
                msg = "message received by server\r\n" # acuse de recibo
                req[2] = 16 + len(msg) # longitud total del paquete mesh
                resp.extend(req[0:4]) # parte del encabezado mesh
                resp.extend(req[10:16]) # direccion de destino
                resp.extend(req[4:10]) # direccion de origen mesh
                resp.extend(msg) # se agrega payload
                ############ fin del mensaje de respuesta ################
                src = binascii.b2a_hex(req[10:16]) # conversion hex --> ascii
                list_aux1 = list(src)
                i = 0 # contador
                j = 2 # pivote
                while i<5: # se construye mac address xx:xx:xx:xx:xx:xx
                    list_aux2 = list_aux1[j:]
                    list_aux1[j] = ":"
                    list_aux1[j+1:] = list_aux2
                    j = j+3
                    i += 1
                src = ''.join(list_aux1) # mac address en formato string
                ######### Informacion del mensaje recibido ############
                print("message received from {} at {}".format(src, datetime.datetime.now()))
                print(req[16:]) # "The temperature is ----"
                 ########### envia mensaje de respuesta ##############
                self.request.sendall(resp)
        except Exception as e:
            print(e)

    def read_full(self, n): # lee bytes del paquete mesh
        while len(self.buf) < n:
            try:
                req = self.request.recv(1024) # recibe datos
                if not req:
                    raise(Exception('recv error')) # error al recibir
                self.buf.extend(req) # | self. buf | <--- req
            except Exception as e:
                raise(e)
        read = self.buf[0:n] # n bytes leidos en variable "read"
        self.buf = self.buf[n:] # l-n bytes en self.buf
        return bytes(read) # retorna los bytes que han sido leidos

######################## configuracion del servidor #########################
class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 7000 # (ip + puerto) en el servidor local
    server = ThreadedTCPServer((HOST, PORT), MeshHandler)
    server.allow_reuse_address = True
    print('mesh server is working')
    server.serve_forever() # run server
