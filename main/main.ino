//Included Libraries
#include <IRremote.h> //https://www.arduino.cc/reference/en/libraries/irremote/
#include "DHT.h" //https://github.com/adafruit/DHT-sensor-library & https://github.com/adafruit/Adafruit_Sensor
#include <LiquidCrystal.h> //Built-In 

// Pins
const int wind_speed = A0;
const int uv_level_value = A1;
const int water_level = A2;
const int IR_PIN = 52; //Infrared receiver

#define DHTPIN 53     // Digital pin connected to the DHT sensor
LiquidCrystal lcd(7,8,9,10,11,12); //initializes the RS, Enable, D4, D5, D6, and D7 pins of the LCD

// Variables
int prev_water_level = -1; //Setting it to negative so it doesn't cause any issues when the water sensor is not in water
int max_water_level; //Used for tide and wave height
int min_water_level; //Used for tide and wave height
float tide_height = 0; //Height of the tide from sea level
float wave_height = 0; //Height of the amplitude of the wave

int wind_speed_val = 0; //Wind speed in mph
float uv_perc = 0; //UV Intensity Percentage
float temp = 0; //Air Temperature
float humidity = 0; //Humidity level

#define DHTTYPE DHT11   // DHT 11

int time = 1; //The time is used to simulate the password to use the remote. The password is the last digit of the time being somewhere from 0 to 9
String news = ""; //The string of data that will be displayed on the LCD
int i = 0; //Length of the string
String flag = ""; //The flag that will be displayed on the beach to tell beachgoers the safety status of the beach ranging from double red for extremely dangerous to green for safe
bool changed = false; //This is used to split up the string into two parts for the LCD

// Pad numbers (we only need 0 to 9)
// Each of the pad numbers on the remote are associated with an integer here. This is the hashmap
const unsigned long remote_0 = 3910598400;
const unsigned long remote_1 = 4077715200;
const unsigned long remote_2 = 3877175040;
const unsigned long remote_3 = 2707357440;
const unsigned long remote_4 = 4144561920;
const unsigned long remote_5 = 3810328320;
const unsigned long remote_6 = 2774204160;
const unsigned long remote_7 = 3175284480;
const unsigned long remote_8 = 2907897600;
const unsigned long remote_9 = 3041591040;

//Timers that are used for ensuring the time between LCD displays are consistent
unsigned long currentTime, prevLCD, prevWater, prevData = millis();

IRrecv irrecv(IR_PIN); //Sets the pin for the IR receiver
DHT dht(DHTPIN, DHTTYPE); //

void setup() {
  pinMode(wind_speed, INPUT); //Setting sensors to input
  pinMode(water_level, INPUT);
  pinMode(uv_perc, INPUT);

  irrecv.enableIRIn(); //

  dht.begin(); //

  lcd.begin(16,1); // LCD's number of columns and rows

  Serial.begin(9600); //Starts serial monitor at 9600 bits per second since that is the same rate as the Arduino Mega
}

void loop() {
  currentTime = millis(); //Records time at the start of the loop
  if(currentTime - prevData > 1000){ //If enough time has passed, the data is updated to refresh the LCD
    prevData = currentTime; //The time is updated as well

    wind_speed_val = analogRead(wind_speed); //Raw wind speed value based on the voltage produced from the DC motor
    wind_speed_val = wind_speed_val * .4; //This is converted to a rough estimate of the wind speed in mph based on tests we did blowing on it and estimating the speed of our blowing

    uv_perc = analogRead(uv_level_value); //Raw light intensity value
    uv_perc = uv_perc/11; //This is converted to a percentage based on tests of the highest and lowest light levels being approximately 1000 and 0 respectively

    humidity = dht.readHumidity(); //Humidity is a percentage
    temp = dht.readTemperature(true); //Temperature is in Fahrenheit
  }


  if(currentTime-prevWater > 50){ //This makes sure that the tide and wave height are updated every 50 milliseconds for better calculations
    prevWater = currentTime; //Current time updates the water timer
    float water_level_val = analogRead(water_level); //Raw water level data

    // The tide and wave heights are calculated by finding the approximate peaks and troughs of the waves
    // This is done by recording the previous and current water levels and comparing them to see at what stage in the wave it is at
    
    if (prev_water_level > 0){ //Checks if this the first measurement to ensure there are no errors
      if (water_level_val < prev_water_level){ //If current water level is lower
        if (max_water_level == prev_water_level){ //If we stopped going up from last measurement and are now going down, we can now use the record of the last peak and trough
          wave_height = max_water_level - min_water_level; //Wave height is equivalent to amplitude
          tide_height = (max_water_level + min_water_level)/2; //Tide height is taken using the average of the peak and trough
        }
        min_water_level = water_level_val; //Now the minimum water level is updated because we are sloping down
      }
      else{ //If current water level is higher
        if (min_water_level == prev_water_level){ //If we stopped going down from last measurement and are now going up, we can now use the record of the last peak and trough
          wave_height = max_water_level - min_water_level;
          tide_height = (max_water_level + min_water_level)/2;
        }
        max_water_level = water_level_val; //Now the maximum water level is updated because we are sloping up
      }
    }
    else{
      int min_water_level = water_level_val; //These are both initialized to the current water level to avoid errors
      int max_water_level = water_level_val;
    }
    wave_height = (wave_height)/550 * 500; //Normalizing the data
    tide_height = (tide_height)/700 * 500;
    prev_water_level = water_level_val; //The current water level is now changed to the previous water level for the next cycle

  }
  //      Remote & IR Receiver
  //Check if the IR receiver has received a signal and the correct password, then perform command
  if(pass()){
      delay(400);
      command();
      irrecv.resume();
  }

  if(currentTime - prevLCD > 300){ //Updates the LCD screen every 200 ms
    prevLCD = currentTime; //Current time updates the LCD timer
    if(i > news.length()){ //Loops through every character in the length of the string (this is important after the i variable is increased in the else statement to indicate another character should be added)
      changed = !changed; //Flips between both halves of the string
      i=0; //Resets string length
      if (changed){
        news = " Wind Speed: " + String(wind_speed_val) + " mph" + " UV Value: " + String(uv_perc) + "%" + " Wave height: " + String(wave_height) + " mm";
      }
      else{
        news = " Tide height: " + String(tide_height) + " mm" + " Temperature: " + String(temp) + " F" + " Humidity: " + String(humidity) + "%" + " Flag: " + flag;
      }
      
      lcd.clear(); //Clears LCD screen
      lcd.print(news); //Prints it to the screen
    }
    else{
      lcd.scrollDisplayLeft(); // scroll one position left
      i++; //Increase one more character in LCD screen
    }

  }


}


bool pass(){ //Function used to determine the passcode for the remote control. Again, this is supposed to simulate a code based on the last digit in the time of the day (e.g. the code would be 2 for a time that said 6:42 AM)
    if (irrecv.decode()) {
    // Print the HEX value of the button press
    unsigned long a = irrecv.decodedIRData.decodedRawData;
    switch (irrecv.decodedIRData.decodedRawData) {
      case remote_0:
        // statements
        return time == 0;
      case remote_1:
        // statements
        return time == 1;
      case remote_2:
        // statements
        return time == 2;
      case remote_3:
        // statements
        return time == 3;
      case remote_4:
        // statements
        return time == 4;
      case remote_5:
        // statements
        return time == 5;
      case remote_6:
        // statements
        return time == 6;
      case remote_7:
        // statements
        return time == 7;
      case remote_8:
        // statements
        return time == 8;
      case remote_9:
        // statements
        return time == 9;
      default:
        // statements
        return false;
    }
    }
}

void command(){ //After the code is pressed, the user has half a second (based on the previous code) to input the flag they want and this is the key for it
    if (irrecv.decode()) {
    unsigned long a = irrecv.decodedIRData.decodedRawData;
      switch (irrecv.decodedIRData.decodedRawData) {
        case remote_0:
          // statements
          flag = "Double Red"; //Water closed to the public
          break;
        case remote_1:
          // statements
          flag = "Red"; //High Hazard (High surf and/or strong currents)
          break;
        case remote_2:
          // statements
          flag = "Yellow"; //Medium Hazard (Medium surf and/or currents)
          break;
        case remote_3:
          // statements
          flag = "Green"; //Low Hazard (Calm conditions, exercise caution)
          break;
        case remote_4:
          // statements
          flag = "Purple"; //Dangerous Marine Life (Man o' War, Jellyfish, Stingrays)
          break;
        default:
          // statements
          break;
      }
    }
}