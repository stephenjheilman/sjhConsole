/*
 * Console — Basic interactive REPL with history and tab completion.
 *
 * This example demonstrates the basic usage of the Console library.
 * It uses the REPL (Read-Eval-Print Loop) API to create an interactive shell.
 * These are the available commands:
 *
 *   version — Print firmware version
 *   heap    — Show free heap memory
 *   restart — Restart the device
 *   echo    — Echo arguments back
 *   led     - turn on RGB LED 
 *   blink   - blink RGB LED
 *   info    - chip information
 *   clear   - clear terminal
 *   prompt  - change prompt
 *
 * Connect with any serial terminal (115200 baud) and type "help" to list
 * available commands. Note that for full functionality (like history and tab completion),
 * you need to use a serial terminal that supports VT100 escape sequences (Arduino IDE does not).
 *
 * Board support: all ESP32 variants using UART or HWCDC (USB-JTAG).
 * Currently, the Console library does not support USB OTG (CDC via TinyUSB / USBSerial).
 *
 * Created by lucasssvaz
 * Update by Stephen J. Heilman 
 */

#include <Arduino.h>
#include <Console.h>

//#define LED_PIN 8 // is the built in led for c3
#define LED_PIN LED_BUILTIN // is the built in led for s3

int toggle_led =0;

const int ledPin = LED_BUILTIN; // RGB LED on ESP32-S3

// Volatile variables used to pass data between ISR and main loop
volatile bool timerFlag = false;
volatile uint32_t interruptCounter = 0;
volatile uint32_t rgb_color = 0;
volatile uint32_t  rgb_brightness = 16;

// Pointer to track the hardware timer configuration
hw_timer_t *timer = NULL; 

// ---------------------------------------------------------------------------
// Command: version
// ---------------------------------------------------------------------------
static int cmd_version(int argc, char **argv) {
  printf("arduino-esp32 %s\n", ESP_ARDUINO_VERSION_STR);
  return 0;
}

// ---------------------------------------------------------------------------
// Command: heap
// ---------------------------------------------------------------------------
static int cmd_heap(int argc, char **argv) {
  printf("Free heap : %lu bytes\n", (unsigned long)ESP.getFreeHeap());
  printf("Min heap  : %lu bytes\n", (unsigned long)ESP.getMinFreeHeap());
  return 0;
}

static int cmd_info(int argc, char **argv) 
{
printf("Version\ncore: ");
printf("%s\n", ESP.getCoreVersion());
printf("Number of cores: %d\n", ESP.getChipCores());
printf("Flash size: %lu\n", (unsigned long)ESP.getFlashChipSize());
printf("Model : %s\n", ESP.getChipModel());
printf("Speed: %lu\n", (unsigned long)ESP.getFlashChipSpeed());
printf("Frequency: %lu Mhz\n", (unsigned long)ESP.getFlashFrequencyMHz());

return 0;

}

static int cmd_clear(int argc, char **argv) {
  Console.clearScreen();
  printf("\n");
  return 0;
}

// ---------------------------------------------------------------------------
// Command: restart
// ---------------------------------------------------------------------------
static int cmd_restart(int argc, char **argv) {
  printf("Restarting in 1 second...\n");
  fflush(stdout);
  delay(1000);
  ESP.restart();
  return 0;
}

// ---------------------------------------------------------------------------
// Command: echo  <message...>
// ---------------------------------------------------------------------------
static int cmd_echo(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    printf("%s", argv[i]);
    if (i < argc - 1) {
      printf(" ");
    }
  }
  printf("\n");
  return 0;
}

static int cmd_prompt(int argc, char **argv) {

  /*
  for (int i = 1; i < argc; i++) {
    printf("%s", argv[i]);
    if (i < argc - 1) {
      printf(" ");
    }
  }
  */
  Console.setPrompt(argv[1]);
  // printf("\n");
  return 0;
}

/*
led red turns on rgbled to red
led green turns on rgbled to green
led blue turns on rgbled to blue
*/
static int cmd_led(int argc, char **argv) {
  String test;
test = argv[1];
if(test == "red")
{
  rgbLedWrite(ledPin, rgb_brightness, 0, 0);
}
else if(test == "blue")
{
  rgbLedWrite(ledPin, 0, 0, rgb_brightness);
}
else if(test == "green")
{
  rgbLedWrite(ledPin, 0, rgb_brightness, 0);
}
else
{
  rgbLedWrite(ledPin, 0, 0, 0);
}

  return 0;
}


// Interrupt Service Routine (ISR) - Must reside in RAM for speed and safety
void IRAM_ATTR onTimer() {

  interruptCounter++;
  timerFlag = true;

//  delay(1000);
  // Print stats securely over serial monitor
  
  Serial.print("Interrupt triggered! Total count: ");
//
  Serial.println(interruptCounter);

}

static int cmd_blink(int argc, char **argv) {
  Serial.print("blink: ");
  Serial.println(argv[1]);
  
  String test;
  String numsec;
  float rate;

  test = argv[1];
  numsec = argv[2];
  Serial.print("argv[1]: ");
  Serial.println(test);
  Serial.print("argv[2]: ");
  Serial.println(numsec);

  rate = numsec.toFloat() * 1000000;
  Serial.print("Rate: ");
  Serial.println( rate);

  if(test == "on")
  {
    Serial.println("In cmd_blink");
    timerEnd(timer);
    timer = timerBegin(1000000);
    if (timer == NULL) {
      Serial.println("Hardware timer initialization failed!");
      while (1); 
    }

    // 2. Bind the custom ISR function to our hardware timer
    timerAttachInterrupt(timer, &onTimer);

    // 3. Configure the alarm threshold
    // Trigger after 1,000,000 ticks (which equals 1,000,000 microseconds = 1 second)
    // Parameters: (timer_pointer, alarm_value, autoreload_bool, loop_count)
    //  timerAlarm(timer, 1000000, true, 0); 
    timerAlarm(timer, rate, true, 0); // 2 sec timer

    // 4. Start the timer execution
    timerStart(timer);
  }
  else
  {
    timerStop(timer);
    rgbLedWrite(ledPin, 0, 0, 0);  //turn off rgb led
  }

return 0;
}

void blinkCode()
{
if(rgb_color == 0)
{
    rgbLedWrite(ledPin, rgb_brightness, 0, 0); 
    rgb_color = 1;
}
else if(rgb_color == 1)
{
    rgbLedWrite(ledPin, 0, rgb_brightness,0);
    rgb_color = 2;
}
else 
{
    rgbLedWrite(ledPin, 0, 0, rgb_brightness);
    rgb_color = 0;

}


}
// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  toggle_led =0;
  delay(1000);

  // Configure Console
  Console.setPrompt("MyEsp32> ");
  Console.setMaxHistory(32);

  // Initialize Console
  if (!Console.begin()) {
    Serial.println("Console init failed");
    return;
  }

  // Add commands
  Console.addCmd("version", "Print firmware version", cmd_version);
  Console.addCmd("heap", "Show free heap memory", cmd_heap);
  Console.addCmd("restart", "Restart the device", cmd_restart);
  Console.addCmd("echo", "Echo arguments back", "<message...>", cmd_echo);
  Console.addCmd("led", "red/green/blue empty is off LED",  cmd_led);
  Console.addCmd("blink", "blink time  LED",  cmd_blink);
  Console.addCmd("info", "chip information",  cmd_info);
  Console.addCmd("clear", "clear terminal",  cmd_clear);
  Console.addCmd("prompt", "change prompt", "<message...>", cmd_prompt);

  // Add built-in help command
  Console.addHelpCmd();

  // Begin Read-Eval-Print Loop
  if (!Console.attachToSerial(true)) {
    Serial.println("REPL start failed");
  }
}

void loop() {
  if(timerFlag == true)
  {
    blinkCode();
  }
/*  
  else
  {
    rgbLedWrite(ledPin, 0, 0, 0);
  }
*/

  timerFlag = false;
  // Loop task is not used in this example, so we delete it to free up resources
//  vTaskDelete(NULL);
}
