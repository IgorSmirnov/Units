
extern char * LogMessage;
extern char * ApplicationName;
extern char LogName[MAX_PATH + 1];

void AddMessage(void);
void AddLogName(void);
void AddBSTR(wchar_t * String);
void AddChars(char * String);
bool CreateDefLogName(void);
int GetModuleNameByAddr(void * Address, char * FileName, int Length); // Psapi.lib
void AddTimeExtToLogName(char * Last);
HMODULE GetModuleByAddress(void * Address); // Psapi.lib
