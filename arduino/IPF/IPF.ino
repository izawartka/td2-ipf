String inputString = "";
bool stringComplete = false;

#define inSwCount 3
int inSwPin[inSwCount] = {5,6,7};
String inSwOn[inSwCount] = {"AK_27:+","AK_25:+","AK_12ab:+"};
String inSwOff[inSwCount] = {"AK_27:-","AK_25:-","AK_12ab:-"};

#define outBtnCount 6
int outBtnPin[outBtnCount] = {8,9,10,11,12,13};
String outBtnMsg[outBtnCount] = {"AK_27:+","AK_25:+","AK_12ab:+","AK_27:-","AK_25:-","AK_12ab:-"};

int outBtnState[outBtnCount];

void setup() {
  Serial.begin(9600);
  inputString.reserve(200);

  for(int i = 0; i < outBtnCount; i++)
  {
    pinMode(outBtnPin[i], INPUT_PULLUP);
  }
}

void loop() {
  if (stringComplete) {
    EventReco();
    inputString = "";
    stringComplete = false;
  }
  else if (Serial.available())
  {
    char inChar = (char)Serial.read();
    if (inChar == '\n') 
    {
      stringComplete = true;
    }
    else inputString += inChar;
  }

  inputReco();
}

void EventReco()
{
  for(int i = 0; i < inSwCount; i++)
  {
    if(inputString == inSwOn[i]) digitalWrite(inSwPin[i], HIGH);
    if(inputString == inSwOff[i]) digitalWrite(inSwPin[i], LOW);
  }
}

void inputReco()
{
  for(int i = 0; i < outBtnCount; i++)
  {
    
    if(digitalRead(outBtnPin[i]) == LOW)
    {
      outBtnState[i]++;
      if(outBtnState[i] == 10)
      {
        Serial.println(outBtnMsg[i]);
      }
    }
    else
    {
      outBtnState[i] = 0;
    }
  }

  delay(1);
}
