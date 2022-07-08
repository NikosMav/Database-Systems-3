#ifndef AM_H_
#define AM_H_

/* Error codes */

#define MAXOPENFILES 20
#define MAXSCANS 20

extern int AM_errno;

//Error Codes
#define AME_OK 0
#define AME_EOF -1
#define AM_FULLMOP -2 //Array of max open files is full
#define AM_WRONGFILE -3 //Tried to open a non b+ tree file
#define AM_OPENSCANS -4 //Still has open scans
#define AM_AL_OPENED -5 //File is already open
#define AM_ATTR_ERROR -6
#define AM_BF_ERROR -7
#define AM_MAX_SCANS_EXCEEDED -8
#define AM_FAILED_INSERT -9
#define AM_FAILED_CLOSE_INDEX_SCAN -10
#define AM_FAILED_DESTROY_INDEX -11
#define AM_FAILED_CLOSE_INDEX -12

#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6


typedef struct fileInfo {
  int fileDesc;
  char* fileName;
  char type1;
  char type2;
  int length1;
  int length2;
  int numOfRecs;
  int numOfPtrs;

} fileInfo;

typedef struct scanInfo {
  char* fileName;
  int AMindex;
  int op;
  void* comparisonKey;
  int currentBlock;
  int entryPos;

} scanInfo;

void AM_Init( void );


int AM_CreateIndex(
  char *fileName, /* όνομα αρχείου */
  char attrType1, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength1, /* μήκος πρώτου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(
  char *fileName /* όνομα αρχείου */
);


int AM_OpenIndex (
  char *fileName /* όνομα αρχείου */
);


int AM_CloseIndex (
  int AMDesc /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
);


int AM_InsertEntry(
  int AMDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  void *value1, /* τιμή του πεδίου-κλειδιού προς εισαγωγή */
  void *value2 /* τιμή του δεύτερου πεδίου της εγγραφής προς εισαγωγή */
);


int AM_OpenIndexScan(
  int fileDesc, /* αριθμός που αντιστοιχεί στο ανοιχτό αρχείο */
  int op, /* τελεστής σύγκρισης */
  void *value /* τιμή του πεδίου-κλειδιού προς σύγκριση */
);


void *AM_FindNextEntry(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


int AM_CloseIndexScan(
  int scanDesc /* αριθμός που αντιστοιχεί στην ανοιχτή σάρωση */
);


void AM_PrintError(
  char *errString /* κείμενο για εκτύπωση */
);

void AM_Close();


#endif /* AM_H_ */
