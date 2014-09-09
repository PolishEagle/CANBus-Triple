#ifndef TROUBLE_CODES_HEAD
#define TROUBLE_CODES_HEAD

#define CODE_CHAR_SIZE 5 + 1

/**
 * Structs used for trouble code processing
 */
typedef struct TroubleCodeHeadStruct {
    char ** codes;
    short codeCount;
    short insertedCodes;
    short displayIndex;
} TroubleCodeHead;



void initializeCodesStruct(TroubleCodeHead * head)
{
    head->codes = NULL;
    head->codeCount = 0;
    head->insertedCodes = 0;
    head->displayIndex = -1;
}

/***
 * Initialize the trouble code head struct
 */
void initializeCodesStruct(TroubleCodeHead * head, int numCodes)
{
    if (head == NULL)
        return;
    
    head->codes = (char**)malloc(sizeof(char*) * numCodes);
    
    for (int i = 0; i < numCodes; i++)
        head->codes[i] = (char*)malloc(sizeof(char) * (CODE_CHAR_SIZE + 1));
    
    head->codeCount = numCodes;
    head->displayIndex = -1;
    head->insertedCodes = 0;
}


/**
 * Add the code to our structure.
 */
void insertNewCode(TroubleCodeHead * head, char * code)
{
    // if the item doesn't exist lets add it, other wise just free it
    for (int i = 0; i < head->insertedCodes; i++) {
        if (strcmp(code, head->codes[i]) == 0) {
            free(code);
            return;
        } 
    }
    
    // Copy the code to our struct
    strcpy(head->codes[head->insertedCodes++], code);
}


/**
 * Gets the current code
 */
char * getCurrentCode(TroubleCodeHead * head)
{
    if (head == NULL || head->codeCount == 0 || head->displayIndex == -1)
        return NULL;
        
    return head->codes[head->displayIndex];
}
  

/**
 * Increment the page index to the next code
 */
void nextCode(TroubleCodeHead * head)
{
    if (head == NULL || head->codeCount == 0)
        return;
        
    if ((head->displayIndex + 1) >= head->codeCount)
        head->displayIndex = -1;
    else
        head->displayIndex++;
}


/**
 * Decrement the code page index to the previous one.
 */
void previousCode(TroubleCodeHead * head)
{
    if (head == NULL || head->codeCount == 0)
        return;
  
    if ((head->displayIndex - 1) < -1)
        head->displayIndex = head->codeCount - 1;
    else
        head->displayIndex--;
}


void destroyCodes(TroubleCodeHead * head)
{
    for (int i = 0; i < head->codeCount; i++)
    {
        if (head->codes[i] != NULL)
            free(head->codes[i]);
    }
    
    if (head->codes != NULL)
        free(head->codes);
        
    head->codes = NULL;    
    head->codeCount = 0;
    head->insertedCodes = 0;
    head->displayIndex = -1;
}

short getNumCodes(TroubleCodeHead * head)
{
    if (head != NULL)
        return head->codeCount;
    else
        return 0;
}

short getNumInsertedCodes(TroubleCodeHead * head)
{
    if (head != NULL)
        return head->insertedCodes;
    else
        return 0;
}

short getDisplayIndex(TroubleCodeHead * head)
{
    if (head != NULL)
        return head->displayIndex;
    else
        return 0;
}


void processDTC(TroubleCodeHead * codes, byte b1, byte b2)
{
    char * currDTC = (char*)malloc(sizeof(char) * CODE_CHAR_SIZE);
    currDTC[CODE_CHAR_SIZE - 1] = '\0';

    // Get the first character for the trouble code
    switch((b1 >> 6) & 0x03) {
        case 0x00:  currDTC[0] = 'P';  break;  // 00  P
        case 0x01:  currDTC[0] = 'C';  break;  // 01  C
        case 0x02:  currDTC[0] = 'B';  break;  // 10  B
        case 0x03:  currDTC[0] = 'U';  break;  // 11  U
    }
    
    // Get the second character for the trouble code
    sprintf(&currDTC[1], "%d", (b1 >> 4) & 0x03);
    
    // Get the third character for the trouble code
    sprintf(&currDTC[2], "%d", b1 & 0x03);
    
    // Get the fourth character for the trouble code
    sprintf(&currDTC[3], "%d", (b2 >> 4) & 0x0F);
    
    // Get the third character for the trouble code
    sprintf(&currDTC[4], "%d", b2 & 0x0F);
    
    /*
    // Get the second character for the trouble code
    currDTC[1] = (char)(((b1 >> 4) & 0x03) + 48);
    
    // Get the third character for the trouble code
    currDTC[2] = (char)((b1 & 0x03) + 48);
    
    // Get the fourth character for the trouble code
    currDTC[3] = (char)(((b2 >> 4) & 0x0F) + 48);
    
    // Get the third character for the trouble code
    currDTC[4] = (char)((b2 & 0x0F) + 48);
    */
    
    // Save the DTC
    if (currDTC[1] == 48 && currDTC[2] == 48 &&currDTC[3] == 48 && currDTC[4] == 48)
        free(currDTC);
    else
        insertNewCode(codes, currDTC);
}

#endif
