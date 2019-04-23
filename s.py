import serial


class app:
    def __init__(self):
        self.Status = True
        self.Ser = serial.Serial("/dev/tty.SLAB_USBtoUART",115200,timeout=None)
        self.Ser.flush()

    def send(self):
        index = 0
        print("Send data")
        while self.Status:
            self.Ser.write(b'a'*64)
            index += 64
            recv = self.Ser.readline()
            while bytes.decode(recv) != "data partial received\n":
                if bytes.decode(recv) == "data received completely\n":
                    self.Status = False
                    print(index)
                    break

    def run(self):
        while self.Status:
            recv = self.Ser.readline()
            if bytes.decode(recv) == "ready to receive\n":
                self.send()
                print("exit send")
            elif bytes.decode(recv) == "ready to send\n":
                self.Ser.write(b"please send\n")
                self.receive()
                self.Status = False

    def receive(self):
        data = self.Ser.read(256)
        print(str(data))
        print(len(data))

    def close(self):
        self.Ser.close()
        

if __name__ == '__main__':
    a = app()
    a.run()
    a.close()



