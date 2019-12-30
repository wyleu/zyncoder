import smbus2
import RPi.GPIO as GPIO
from time import sleep
import i2cEncoderLibV2

# First thing we need to have is a python function that will
# handle events as they occur. Notice it has self declared as a parameter
# This is so when it is handled in the GPIO callback it can be passed an
# instance of the object that caused the event

class I2c_RGB(object):
    def __init__(self):
        # Firstly we tell the RaspPi how we want the i/O configured.
        GPIO.setmode(GPIO.BCM)
        # Then we describe the i2C library and the bus we are on.
        self.bus = smbus2.SMBus(1)

        # Now in our world we need to know which pin the interrupt
        # we will use to signal events that will appear on the i2c bus.
        # This needs choosing so that it doesn't clash with screens and such. . .
        INT_pin = 4

        # Then we configure the pin. . .
        GPIO.setup(INT_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

        # and finally talk to the encoder library that our dear friends
        # in Italy so helpfully supplied
        # self.encoder = i2cEncoderLibV2.i2cEncoderLibV2(bus, 0x43)


        # We set up some context ( the perpetual game of programming)
        self.encconfig = (
                i2cEncoderLibV2.INT_DATA |       # We will us 3c2 bit INT's not floats
                i2cEncoderLibV2.WRAP_ENABLE |    # on reaching Cmax + 1 goto Cmin & vice versa
                i2cEncoderLibV2.DIRE_RIGHT |     # rotate tight to increase
                i2cEncoderLibV2.IPUP_ENABLE |    # Enable Interupt PUll up resistors
                i2cEncoderLibV2.RMOD_X1 |        # Read  on A falling edge mode (low res)
                i2cEncoderLibV2.RGB_ENCODER      # We are using RGB_Encoders
        )
        # populate the encoders dict with this context.

        self.detect_encoders()

        # Now we add a GPIO event_detect by telling it
        GPIO.add_event_detect(
            INT_pin,                            #   where to expect the interupt
            GPIO.FALLING,                       #   which edge to wait for...
            callback=self.update_status,        #   the function to call when this all happens . .
            bouncetime=10                       #   ignore noise ...
        )

    def detect_encoders(self):
        """
        Identify valid encoders.
        :return:
        """
        self.encoders = {}

        for encoder in range(1,127):
            # self.encoder = i2cEncoderLibV2.i2cEncoderLibV2(bus, 0x43)
            try:
                poss_encoder = i2cEncoderLibV2.i2cEncoderLibV2(self.bus, encoder)
                poss_encoder.begin(self.encconfig)
                self.encoders[encoder] = poss_encoder
            except OSError:
                pass  # let it pass silently . . .

        print('%d Encoders' % (len(self.encoders,)))
        for count, encoder in enumerate(self.encoders):
            print(count, encoder )

    def config_encoders(self):
        for encoder in self.encoders:
            encoder.begin(self.encconfig)

            encoder.writeRGBCode(0x320000)
            sleep(0.3)

            # Do some higher level setup of other specific information
            encoder.writeCounter(0)
            encoder.writeMax(35)                                   # We will go up to 35
            encoder.writeMin(-20)                                  # And down to -20
            encoder.writeStep(1)                                   # With a step size of 1
            encoder.writeAntibouncingPeriod(8)                     # and a bit of anti bounce
            encoder.writeDoublePushPeriod(50)                      # and make a double push
            encoder.writeGammaRLED(i2cEncoderLibV2.GAMMA_2)        # And tune the LED curves
            encoder.writeGammaGLED(i2cEncoderLibV2.GAMMA_2)
            encoder.writeGammaBLED(i2cEncoderLibV2.GAMMA_2)

            encoder.onChange = self.EncoderChange
            encoder.onButtonPush = self.EncoderPush

            encoder.autoconfigInterrupt()

            encoder.writeRGBCode(0x003200)
            sleep(0.3)

            print ('Board ID code: 0x%X' % (encoder.readIDCode()))
            print ('Board Version: 0x%X' % (encoder.readVersion()))

            encoder.writeRGBCode(0x000000)


    def update_status(self, event):
        # Call the update Status function defined in the
        # i2cEncoderLibV2 library which will put all the values in
        # the python variables that we can then react to.
        print('Hi!', event)
        self.encoder.updateStatus()

    def EncoderChange(self):
        self.encoder.writeLEDG(100)
        print('Changed: %d' % (self.encoder.readCounter32()))
        self.encoder.writeLEDG(0)

    def EncoderPush(self):
        self.encoder.writeLEDB(100)
        print('Encoder Pushed!')
        self.encoder.writeLEDB(0)

# and Sit and wait for events .. . .
# in a loop probably should be constucted
# round the Sm2Bus library with a with . .

if __name__ == '__main__':
    i2c_rgb = I2c_RGB()
    while True:
        pass

