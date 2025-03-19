typedef enum
{
    MODE_ID_RFID_ADD,
    MODE_ID_RFID_FIRST,
    MODE_ID_RFID_SECOND
} MODE_ID_RFID_E;

unsigned char MODE_RFID = MODE_ID_RFID_ADD;

unsigned char id = 0;
unsigned char id_rf = 0;
unsigned char index_t = 0;
unsigned char error_in = 0;

// init keypad
const byte ROWS = 4; // four rows
const byte COLS = 3; // four columns
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {12, 14, 27, 26}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {25, 33, 32}; // connect to the column pinouts of the keypad
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// int addr = 0;
char password[6] = "22222";
char pass_default[6] = "12345";
char mode_changePass[6] = "*001#";
char mode_resetPass[6] = "*002#";
char mode_hardReset[6] = "*003#";
char mode_addRFID[6] = "*101#";
char mode_delRFID[6] = "*102#";
char mode_delAllRFID[6] = "*103#";

char data_input[6]; // variable of data from rfid and keybad
char new_pass1[6]; // variable of comparing changed pass
char new_pass2[6]; // variable of comparing changed pass the second

unsigned char in_num = 0, time_error = 0, isMode = 0;
//init lcd i2c
LiquidCrystal_I2C lcd(0x27, 16, 2);
//init rfid
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];
Servo sg90;

void writeEpprom(char data[]){ //viet du lieu xuong bo nho
    for (unsigned char i = 0; i < 5; i++){
        EEPROM.write(i, data[i]);
    }
    EEPROM.commit();
}

void readEpprom(){
    for (unsigned char i = 0; i < 5; i++){
        password[i] = EEPROM.read(i);
    }
}

void clear_data_input(){ // xoa gia tri nhap vao hien tai
    for (int i = 0; i < 6; i++){
        data_input[i] = '\0';
    }
}

unsigned char isDataBuffer(char data[]){ // Kiem tra buffer da co gia tri chua
    for(unsigned char i = 0; i < 5; i++){
        if(data[i] == '\0') return 0;
    }
    return 1;
}

bool compareData(char data1[], char data2[]){ // Kiem tra 2 cai buffer co giong nhau hay khong
    for (unsigned char i = 0; i < 5; i++){
        if (data1[i] != data2[i]) return false;
    }
    return true;
}

void insertData(char data1[], char data2[]){ // Gan buffer 2 cho buffer 1
    for (unsigned char i = 0; i < 5; i++){
        data1[i] = data2[i];
    }
}

void getData(){ // Nhan buffer tu ban phim
    char key = keypad.getKey(); // Doc gia tri ban phim
    if (key) {
        if (in_num < 5) { // kiem tra so luong ky tu
            data_input[in_num] = key;
            lcd.setCursor(5 + in_num, 1); // dat con tro lcd
            lcd.print(data_input[in_num]);
            lcd.setCursor(5 + in_num, 1);
            lcd.print("*");
            in_num++;

            // neu da du ky tu thi dat lai in_num
            if (in_num == 5) {
                Serial.println(data_input);
                in_num = 0;
            }
        }
    }
}

void checkPass(){ // kiem tra password
    getData();
    if (isDataBuffer(data_input)){
        Serial.print("Input Password: ");
        Serial.println(data_input);
        Serial.print("Stored Password: ");
        Serial.println(password);

        char* modes[] = {mode_changePass, mode_resetPass, mode_addRFID, mode_delRFID, mode_delAllRFID};
        int mode_index[] = {1, 2, 8, 9, 10};
// so sanh cac dieu kien
        for (int i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
            if (compareData(data_input, modes[i])) {
                lcd.clear();
                clear_data_input();
                index_t = mode_index[i];
                return;
            }
        }

        if (compareData(data_input, password)){ // Dung pass
            lcd.clear();
            clear_data_input();
            index_t = 3;//truong hop dung pass
        }

        else if(compareData(data_input, mode_hardReset)){
            lcd.setCursor(0, 0);
            lcd.print("---HardReset---");
            writeEpprom(pass_default);
            insertData(password, pass_default);
            clear_data_input();
            delay(2000);
            lcd.clear();
            index_t = 0;
        }
        
        else{ //xu li loi
            if (time_error == 2){
                clear_data_input();
                lcd.clear();
                index_t = 4;
            }
            if(time_error == 4){
                clear_data_input();
                lcd.clear();
                index_t = 5;
            }
            lcd.setCursor(1, 1);
            lcd.print("WRONG PASSWORD");
            delay(1000);
            clear_data_input();
            time_error++;
            lcd.clear();
        }
    }
}

void openDoor(){
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("---OPENDOOR---");
    unsigned char pos;
    // digitalWrite(BUZZER, HIGH);
    delay(1000);
    // digitalWrite(BUZZER, LOW);
    sg90.write(90);
    ////digitalWrite(LED, HIGH);
    delay(5000);
    //digitalWrite(LED, LOW);
    sg90.write(0);
    lcd.clear();
    index_t = 0;
}

void errored3Times(){
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("WRONG 3 TIME");
    digitalWrite(BUZZER, HIGH);
    Serial.println("buzzer is on");
    delay(3000);
    digitalWrite(BUZZER, LOW);
    lcd.setCursor(1, 1);
    lcd.print("Wait 1 minutes");
    delay(1000);
    lcd.clear();
    unsigned char seconds = 59;

//dem thoi gian cho
    while(seconds > 0){
    // Hien thoi gian con lai
        lcd.setCursor(1, 1);
        lcd.print("Wait ");
        // lcd.print(minutes);
        // lcd.print(" : ");
        lcd.print(seconds < 10 ? "0" : ""); // Thêm số 0 ở trước nếu giây < 10
        lcd.print(seconds);

        // Delay 1 giay
        delay(1000);

        // Cap nhat giay
        // if(seconds == 0){
        //     if (minutes > 0) {
        //         minutes--;
        //         seconds = 59;
        //     }
        // } 
        seconds--; // Giảm giây
    }
    lcd.clear();
    index_t = 0;
}

//neu sai qua 5 lan => khoa cho den khi reset hard pass
void errored5Times(){
    while(1){
        // Serial.println("buzzer is on");
        // digitalWrite(BUZZER, HIGH);
        // delay(500);
        // digitalWrite(BUZZER, LOW);
        lcd.setCursor(0, 4);
        lcd.print("LOCKED");
        if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
        byte rfidTag[4];
        Serial.print("RFID TAG: ");

        for(byte i = 0; i < rfid.uid.size; i++){
            rfidTag[i] = rfid.uid.uidByte[i];
            Serial.print(rfidTag[i], HEX);
        }
        Serial.println();

        if(isAllowedRFID(rfidTag)){
            lcd.clear();
            index_t = 3;
            break;
        }
        Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));

        for(byte i = 0; i < 4; i++){
            nuidPICC[i] = rfid.uid.uidByte[i];
        }

        Serial.println(F("The NUID tag is:"));
        Serial.print(F("In hex: "));
        printHex(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
        Serial.print(F("In dec: "));
        printDec(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
    }
}

void changePass(){ // Thay doi pass
    lcd.setCursor(0, 0);
    lcd.print("- Change Pass -");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("---New Pass---");
//nhap mat khau moi
    while(1){
        getData();
        if(isDataBuffer(data_input)){
            insertData(new_pass1, data_input);
            clear_data_input();
            break;
        }
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("- Pass Again -");
//nhap lai mat khau
    while (1)
    {
        getData();
        if (isDataBuffer(data_input))
        {
            insertData(new_pass2, data_input);
            clear_data_input();
            break;
        }
    }
    delay(1000);
    if (compareData(new_pass1, new_pass2))
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("--- Done ---");
        digitalWrite(BUZZER, HIGH);
        delay(1000);
        digitalWrite(BUZZER, LOW);
        insertData(password, new_pass2);
        writeEpprom(password);
        lcd.clear();
        index_t = 0;
    }
    else
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("- Mismatched -");
        delay(1000);
        lcd.clear();
        index_t = 0;
    }
}

void resetPass(){
    // Serial.println("Pass reset");
    lcd.setCursor(0, 0);
    lcd.print("-- Old Pass --");
    getData();
    if(isDataBuffer(data_input)){
        unsigned char choise = 0;
        if(compareData(data_input, password)){
            lcd.clear();
            clear_data_input();
            while(1){
                lcd.setCursor(0, 0);
                lcd.print("- Reset Pass -");
                char key = keypad.getKey();
                switch(choise){
                    case 0:
                        lcd.setCursor(0, 1);
                        lcd.print(">");
                        lcd.setCursor(2, 1);
                        lcd.print("YES");
                        lcd.setCursor(9, 1);
                        lcd.print(" ");
                        lcd.setCursor(11, 1);
                        lcd.print("NO");
                        break;
                    case 1:
                        lcd.setCursor(0, 1);
                        lcd.print(" ");
                        lcd.setCursor(2, 1);
                        lcd.print("YES");
                        lcd.setCursor(9, 1);
                        lcd.print(">");
                        lcd.setCursor(11, 1);
                        lcd.print("NO");
                        break;
                    default:
                        break;
                }
                if(key == '*'){
                    if(choise == 1){
                        choise = 0;
                    }
                    else{
                        choise++;
                    }
                }
                if(key == '#' && choise == 0){
                    lcd.clear();
                    delay(1000);
                    writeEpprom(pass_default);
                    insertData(password, pass_default);
                    lcd.setCursor(0, 0);
                    lcd.print("- Reset Done -");
                    digitalWrite(BUZZER, HIGH);
                    delay(1000);
                    digitalWrite(BUZZER, LOW);
                    lcd.clear();
                    break;
                }
                if(key == '#' && choise == 1){
                    lcd.clear();
                    break;
                }
            }
            index_t = 0;
        }
        else{
            index_t = 0;
            lcd.clear();
        }
    }
}

////////////////////////RFID/////////////////////////////
unsigned char numberInput(){
    char number[5];
    char count_i = 0;
    while(count_i < 2){
        char key = keypad.getKey();
        if(key && key != '*' && key != '#'){
            delay(100);
            lcd.setCursor(10 + count_i, 1);
            lcd.print(key);
            number[count_i] = key;
            count_i++;
        }
    }
    return (number[0] - '0') * 10 + (number[1] - '0');
}

//print hex rfid
void printHex(byte *buffer, byte bufferSize){
    for (byte i = 0; i < bufferSize; i++){
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

//print dec rfid
void printDec(byte *buffer, byte bufferSize){
    for (byte i = 0; i < bufferSize; i++){
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], DEC);
    }
}

//kiem tra co dung the khong
bool isAllowedRFID(byte tag[]){
    if(tag == NULL) return false;
    int count = 0;
    for(int i = 10; i < 50; i += 4){
        byte storedTag[4];
        Serial.print("EEPROM: ");
        for(int j = 0; j < 4; j++){
            storedTag[j] = EEPROM.read(i + j);
            Serial.print(storedTag[j], HEX);
        }
        Serial.println();

        for(int j = 0; j < 4; j++){
            if(tag[j] == storedTag[j]){
                count++;
            }
        }
        if(count == 4){
            return true;
        }
        count = 0;
    }
    return false;
}

//xu li kiem tra the
void rfidCheck(){
    if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
        byte rfidTag[4];
        Serial.print("RFID TAG: ");
        if (rfid.uid.size > 4) {
            Serial.println(F("UID size is too large."));
            return;
        }
        for(byte i = 0; i < rfid.uid.size; i++){
            rfidTag[i] = rfid.uid.uidByte[i];
            Serial.print(rfidTag[i], HEX);
        }
        Serial.println();

        if(isAllowedRFID(rfidTag)){
            lcd.clear();
            index_t = 3;
        }
        else{
            if(time_error == 2){
                lcd.clear();
                index_t = 4;
            }
            if(time_error == 4){
                lcd.clear();
                index_t = 5;
            }
            Serial.print("Error RFID\n");
            lcd.clear();
            lcd.setCursor(3, 1);
            lcd.print("WRONG RFID");
            time_error++;
            delay(1000);
            lcd.clear();
        }
        Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.println(rfid.PICC_GetTypeName(piccType));

    if( piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&   
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K){

        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return;
    }

        for(byte i = 0; i < 4; i++){
            nuidPICC[i] = rfid.uid.uidByte[i];
        }

        Serial.println(F("The NUID tag is:"));
        Serial.print(F("In hex: "));
        printHex(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
        Serial.print(F("In dec: "));
        printDec(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}

void addRFID(){
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("ADD NEW RFID");
    switch(MODE_RFID){
    case (MODE_ID_RFID_ADD):
    {
        Serial.print("ADD_IN");
        lcd.setCursor(0, 1);
        lcd.print("Input Id: ");
        id_rf = numberInput();
        Serial.println(id_rf);
        if(id_rf == 0){ // ID #0 not allowed, try again!
            lcd.clear();
            lcd.setCursor(3, 1);
            lcd.print("ID ERROR");
        }
        else{
            MODE_RFID = MODE_ID_RFID_FIRST;
        }
        delay(2000);
    }
    break;
    case (MODE_ID_RFID_FIRST):
    {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("   Put RFID    ");
        delay(1000);
        if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
            byte rfidTag[4];
            Serial.print("RFID TAG: ");
            for(byte i = 0; i < rfid.uid.size; i++){
                rfidTag[i] = rfid.uid.uidByte[i];
                Serial.print(rfidTag[i], HEX);
            }
            Serial.println();

            if(isAllowedRFID(rfidTag)){
                lcd.clear();
                lcd.setCursor(1, 1);
                lcd.print("RFID ADDED BF");
                index_t = 0;
                delay(2000);
                lcd.clear();
                MODE_RFID = MODE_ID_RFID_ADD;
            }
            else{
                MODE_RFID = MODE_ID_RFID_SECOND;
            }
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();
        }
    }
    break;
    case MODE_ID_RFID_SECOND:
    {
        lcd.setCursor(0, 1);
        lcd.print("   Put Again    ");
        delay(1000);
        if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
            byte rfidTag[4];
            Serial.print("RFID TAG: ");
            for(byte i = 0; i < rfid.uid.size; i++){
                rfidTag[i] = rfid.uid.uidByte[i];
                Serial.print(rfidTag[i], HEX);
            }
            for(int i = 0; i < 4; i++){
                EEPROM.write(10 + (id_rf - 1) * 4 + i, rfidTag[i]);
                EEPROM.commit();
                Serial.println(EEPROM.read(10 + (id_rf - 1) * 4 + i), HEX);
            }
            Serial.print("OK");
            lcd.setCursor(0, 1);
            lcd.print("Add RFID Done");
            delay(2000);
            index_t = 0;
            Serial.print("ADD_OUT");
            lcd.clear();
            MODE_RFID = MODE_ID_RFID_ADD;
        }
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
    break;
    }
}

void delRFID(){
    char buffDisp[20];
    lcd.setCursor(1, 0);
    lcd.print("  DELETE RFID   ");
    Serial.print("DEL_IN");
    lcd.setCursor(0, 1);
    lcd.print("Input ID: ");
    id_rf = numberInput();
    if(id_rf == 0){ // ID #0 not allowed, try again!
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("ID ERROR");
        delay(2000);
    }
    else{
        for(int i = 0; i < 4; i++){
            EEPROM.write(10 + (id_rf - 1) * 4 + i, '\0');
            EEPROM.commit();
            Serial.println(EEPROM.read(10 + (id_rf - 1) * 4 + i), HEX);
        }
        sprintf(buffDisp, "Clear id:%d Done", id_rf);
        lcd.setCursor(0, 1);
        lcd.print(buffDisp);
        Serial.print("DEL_OUT");
        delay(2000);
        lcd.clear();
        index_t = 0;
    }
}

void delAllRFID(){
    char key = keypad.getKey();
    lcd.setCursor(0, 0);
    lcd.print("CLEAR ALL RFID?");
    if(key == '*'){
        isMode = 0;
    }
    if(key == '#'){
        isMode = 1;
    }
    if(isMode == 0){
        lcd.setCursor(0, 1);
        lcd.print("> Yes      No  ");
    }
    if(isMode == 1){
        lcd.setCursor(0, 1);
        lcd.print("  Yes    > No  ");
    }
    if(key == '0' && isMode == 0){
        for(int i = 10; i < 512; i++){
            EEPROM.write(i, '\0');
            EEPROM.commit();
            Serial.println(EEPROM.read(i), HEX);
        }
        lcd.setCursor(0, 1);
        lcd.print("  Clear done  ");
        delay(2000);
        index_t = 0;
        lcd.clear();
    }
    if(key == '0' && isMode == 1){
        lcd.clear();
        index_t = 0;
    }
}
